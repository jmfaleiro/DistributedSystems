#include <filerequests.hh>
#include <QDir>
#include <QFileInfo>
#include <QFile>
#include <QtCrypto>
#include <QDebug>

FileRequests::FileRequests(const QString& my_name)
{
  me = my_name;
  timerDuration = 1000;
  timer.start(timerDuration);
  downloadTimer.start(timerDuration);
  QObject::connect(&timer, SIGNAL(timeout()),
  		   this, SLOT(processTimeout()));
  
   QObject::connect(&downloadTimer, SIGNAL(timeout()),
  		   this, SLOT(processDownloadTimeout()));
}

void
FileRequests::newDownload(const QString& fileName, const QString&destination, const QByteArray& masterBlock)
{

  if(!pendingDownloads.contains(masterBlock)){
    
    QPair<QString, QString> val(fileName, destination);
    pendingDownloads[masterBlock] = val;
    
    QMap<QString, QVariant> req;
    req["BlockRequest"] = masterBlock;
    qDebug() << masterBlock;
    qDebug() << masterBlock.size();
    emit sendDownloadMsg(req, destination);
  }
}

void
FileRequests::processBlockReply(const QMap<QString, QVariant> & msg)
{
  QByteArray blockReply = msg["BlockReply"].toByteArray();
  QString origin = msg["Origin"].toString();
  QByteArray data = msg["Data"].toByteArray();
  
  qDebug() << "Got something!";
  if (verifyData(blockReply, data)){
    qDebug() << "Verified!";

    // Reply contains the hash meta-list
    if(pendingDownloads.contains(blockReply)){
      
      qDebug() << "FileRequests: got the hash-list";
      if (!invertBlockHashes.contains(blockReply)){
	QList<QByteArray> hashList;
	int size = data.count();
      
	char *buf = data.data();
	int count = 0;
	for(int i = 0; i < size; i += HASH_SIZE){
	
	  QByteArray temp(&buf[i], HASH_SIZE);
	
	  hashList.append(temp);
	  blockHashes[temp] = blockReply;
	  
	  QMap<QString, QVariant> msg;
	  
	  msg["BlockRequest"] = temp;
	  emit sendDownloadMsg(msg, origin);
	  ++count;
	}
	qDebug() << "Expect " << count << " blocks";
	invertBlockHashes[blockReply] = hashList;
      }
    }
  
    // Reply contains a block of data
    else if (blockHashes.contains(blockReply)){
      
      qDebug() << "FileRequests: got a data block!";
      if (!pendingDownloadData.contains(blockReply)){
	
	pendingDownloadData[blockReply] = data;
	QByteArray master = blockHashes[blockReply];
	if(downloadCompleted(master)){
	  
	  qDebug() << "Download completed!";
	  writeFile(master);
	}
	
      }
    }
  }
}

void
FileRequests::writeFile(const QByteArray &hash)
{
  QDir downloadDir;
  if (!downloadDir.exists("Downloads"))
    downloadDir.mkdir("Downloads");

  downloadDir.cd("Downloads");
  QString fileName = pendingDownloads[hash].first;
  QFileInfo fileinf(downloadDir, fileName);
  
  QString bigName = fileinf.absoluteFilePath();
  
  QFile file(bigName);
  
  if (file.exists())
    file.remove();
  
  file.open(QIODevice::WriteOnly);
  
  QByteArray fileData;
  
  QList<QByteArray> hashes = invertBlockHashes[hash];
  for(int i = 0;  i < hashes.count(); ++i)    
    fileData += pendingDownloadData[hashes[i]];
  
      
  file.write(fileData);
  file.flush();
}

void
FileRequests::destroyDownloadData(const QByteArray &hash)
{
  
}

bool
FileRequests::downloadCompleted(const QByteArray &hash)
{
  QList<QByteArray> hashes = invertBlockHashes[hash];
  int size = hashes.count();
  
  for(int i = 0; i < size; ++i){

    if (!pendingDownloadData.contains(hashes[i]))
      return false;
  }
  return true;
}

bool
FileRequests::verifyData(const QByteArray &hash, const QByteArray &data)
{
  QCA::Hash shaHash("sha256");
  
  const char *dataBuf = data.data();
  quint32 size = data.size();
  
  shaHash.update(dataBuf, size);
  
  QByteArray computedHash = shaHash.final().toByteArray();
  
  return computedHash == hash;
}

void
FileRequests::newSearch(const QString& queryString)
{ 
  // Check if we are already searching for this query string.
  if (!pendingSearches.contains(queryString)){
    
    // If not, initiate the search sequence and keep an identifier
    // around (query string itself).
    QMap<QString, QVariant> msg;
    msg["Origin"] = me;
    msg["Search"] = queryString;
    msg["Budget"] = START_BUDGET;
    
    
    pendingSearches[queryString] = START_BUDGET;

    emit broadcastRequest(msg);
  }
}



void
FileRequests::processReply(const QMap<QString, QVariant>&msg)
{
  if (msg.contains("SearchReply") &&
      msg.contains("Dest") &&
      msg.contains("Origin") &&
      msg.contains("HopLimit") &&
      msg.contains("MatchNames") &&
      msg.contains("MatchIDs")){
    qDebug() << "FileRequests: Got search reply for " << msg["SearchReply"].toString();
    processSearchReply(msg);
  }
  
  else if (msg.contains("Dest") &&
	   msg.contains("Origin") &&
	   msg.contains("HopLimit") &&
	   msg.contains("BlockReply") &&
	   msg.contains("Data")){

    processBlockReply(msg);
  }
}

void
FileRequests::processSearchReply(const QMap<QString, QVariant>& msg)
{
  //Check if we have a legitimate reply.
  QString searchReply = msg["SearchReply"].toString();
  if (pendingSearches.contains(searchReply)){
    
    // CAUTION: names and ids might not have the same
    //          size always, make sure you account for that!    
    QList<QVariant> names = msg["MatchNames"].toList();
    QList<QVariant> ids = msg["MatchIDs"].toList();
    
    for(int i = 0; i < names.count() && i < ids.count(); ++i){
      
      QMap<QString, QVariant> temp;
      temp["Name"] = names[i].toString();
      temp["ID"] = ids[i].toByteArray();
      temp["Origin"] = msg["Origin"].toString();
     
      // Keep the responses around and signal the UI.
      searchResponses.insertMulti(searchReply, temp);      
      emit newResponse(searchReply, temp);
      

    }

    // We've exceeded the threshold, stop requesting.
    if (searchResponses.values(searchReply).count() >= NUM_MATCHES)      
      destroyRequest(searchReply);
    
  }
}

void
FileRequests::destroyRequest(const QString& queryString)
{       
  pendingSearches.remove(queryString);
  searchResponses.remove(queryString); 
  qDebug() << "Destroyed search request " << queryString;
}

void
FileRequests::processDownloadTimeout()
{
  downloadTimer.stop();
  
  QList<QByteArray> masters = pendingDownloadData.keys();
  for(int i = 0; i < masters.count(); ++i){

    if (!invertBlockHashes.contains(masters[i])){
	

	QString destination = pendingDownloads[masters[i]].second;
	QMap<QString, QVariant> msg;
	
	msg["BlockRequest"] = masters[i];

	emit sendDownloadMsg(msg, destination);
      }
  }
  
  QList<QByteArray> keys = blockHashes.keys();
  for(int i = 0; i < keys.count(); ++i){
    
    if (!pendingDownloadData.contains(keys[i])){
      
      QString destination = pendingDownloads[blockHashes[keys[i]]].second;
      
      QMap<QString, QVariant> msg;
      
      msg["BlockRequest"] = keys[i];
      emit sendDownloadMsg(msg, destination);      
    }
  }
  
  downloadTimer.start();
}

void
FileRequests::processTimeout()
{
  timer.stop();
  
  QList<QString> keys = pendingSearches.keys();
  for(int i = 0; i < keys.count(); ++i){
    

    quint32 currentBudget = pendingSearches[keys[i]];
    if (currentBudget > MAX_BUDGET){
      
      destroyRequest(keys[i]);
    }

    else{
      QMap<QString, QVariant> msg;      
      msg["Origin"] = me;
      msg["Search"] = keys[i];
      msg["Budget"] = currentBudget * 2;
    
      pendingSearches.insert(keys[i], currentBudget * 2);
    
      emit broadcastRequest(msg);    
    }
  }
  
  timer.start(timerDuration);
}
