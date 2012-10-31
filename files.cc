#include <QDebug>
#include <QtCrypto>
#include <QFile>
#include <QByteArray>
#include <QPair>
#include <QObject>
#include <QString>
#include <QVariant>
#include <QFileInfo>

#include <files.hh>

/*
 * We have to take a lot of care while hashing because 
 * the data() member of the ByteArray returns a \0 terminated
 * string. The padding should *not* affect the result of 
 * the way we do hashing. 
 * 
 * We have to always specify the number of bytes we want to hash
 * and the number of bytes we read from the data buffer into
 * the stream.
 */

FileStore::FileStore()
{

}

/*
 * Given a list of files returned by the user, index
 * their information.
 */
void 
FileStore::IndexFiles(const QStringList& inputFiles)
{
  for(int i = 0; i < inputFiles.count(); ++i)
    IndexSingleFile(inputFiles[i]);
}

/*
 * Index the information of a single file. 
 */
void
FileStore::IndexSingleFile(const QString& inputFile)
{
  
  QFile file(inputFile);
  if (!file.open(QIODevice::ReadOnly)){
    
    qDebug() << "File store: file " << inputFile << " not found.";
    return;
  }
  
  
  QDataStream fileStream(&file);
  HashBlocks(fileStream, inputFile, 0);

  /*
  QList<QVariant> fileHash = ConvertBytes(HashFinalBlock(HashBlocks(fileStream)));
  
  qDebug() << "Hashed " << inputFile << ". Num levels = " << fileHash.count();

  QMap<QString, QVariant> fileDetails;
  fileDetails["size"] = size;
  fileDetails["hash"] = fileHash;
  
  if (files.contains(inputFile)){
    qDebug() << "File " << inputFile << " already exists.";
    files.remove(inputFile);
  }
  
  files[inputFile] = fileDetails;
  */
}

QList<QVariant>
FileStore::ConvertStrings(const QList<QString>&strings)
{
  QList<QVariant> ret;
  int i;
  for(i = 0; i < strings.count(); ++i){
    
    QVariant stringAsVariant(strings[i]);
    ret << stringAsVariant;
  }
  return ret;
}

QList<QVariant>
FileStore::ConvertBytes(const QList<QByteArray>&bytes)
{
  QList<QVariant> ret;
  int i;
  for(i = 0; i < bytes.count(); ++i){
    
    QVariant bytesAsVariant(bytes[i]);
    ret << bytesAsVariant;
  }  
  return ret;
}


/*
 * Recursively hash the blocks of a file along
 *  with the meta list.
 */
void
FileStore::HashBlocks(QDataStream& s, const QString& fileName, quint32 level)
{  
  char data[BLOCK_SIZE];
  int numRead;
  
  QByteArray result;
  // QDataStream resultStream(&result, QIODevice::Append);  

  int count = 0;
  while((numRead = s.readRawData(data, BLOCK_SIZE)) != -1){
    
    QByteArray hash = Hash(data, numRead);
    MapBlock(hash, data, numRead, fileName, level);
    
    result += hash;
    ++count;
    if (numRead < BLOCK_SIZE)
      break;
    
  }
  
  qDebug() << "Hashed " << count << " blocks";
  qDebug() << "num hashes = " << (result.size() / 32);
  if (result.size() % 32 != 0){
    
    qDebug() << "Hash result is not a multiple of 32";
    *((int *)NULL) = 1;             
  }
  
  if (result.size() > BLOCK_SIZE){
    
    QDataStream resultROStream(&result, QIODevice::ReadOnly);
    HashBlocks(resultROStream, fileName, level+1);
  }  
  
  // We need to add one more master block.
  else {
    
    QByteArray hash = Hash(result.data(), result.size());
    MapBlock(hash, result.data(), result.size(), fileName, level+1);    
    MapMaster(hash, level+1, fileName);
    qDebug() << hash << " done";
    qDebug() << hash.size();
  }
}


QByteArray
FileStore::Hash(const char *data, int size)
{
  QCA::Hash shaHash("sha256");
  
  shaHash.update(data, size);
  QByteArray temp = shaHash.final().toByteArray();
  if (temp.size() != HASH_SIZE){
    qDebug() << "Unexpected hash size";
    *((int *)NULL) = 1;        
  }
  return temp;
  
}

quint32
FileStore::ConvertHash(const QByteArray& arr)
{
  bool ok;
  QString hashAsString = QCA::arrayToHex(arr);
  qDebug() << hashAsString;
  quint32 hash = hashAsString.toUInt(&ok, 16);
  
  if (!ok){
    qDebug() << "Couldn't convert hash to uint";
    *((int *)NULL) = 1;      
  }
  
  return hash;
}

void
FileStore::MapMaster(QByteArray hash,
		     quint32 level,
		     const QString& fileName)
{
  QFileInfo fileInfo(fileName);
  QMap<QString, QVariant> val;
  
  val["fullname"] = fileName;
  val["level"] = level;
  val["block"] = hash;

  fileMeta.insertMulti(fileInfo.fileName(), val);
}

void
FileStore::MapBlock(QByteArray hash,
		    const char *data,
		    int size,
		    const QString& fileName,
		    quint32 level)
{
  QByteArray block(data, size);
  QFileInfo fileInfo(fileName);
  QMap<QString, QVariant> val;  
  
  val["fullname"] = fileName;
  val["filename"] = fileInfo.fileName();
  val["block"] = block;  
  val["level"] = level;  
  blockMap[hash] = val;  
}



bool
FileStore::Search(const QString& query, QMap<QString, QVariant> *ret)
{
  qDebug() << "FileStore: got search request for " << query;
  bool isPresent = false;
  QList<QString> keys = fileMeta.keys();
  
  QList<QString> retFiles;
  QList<QByteArray> retBlocks;

  for(int i = 0; i < keys.count(); ++i){
    
    if(Match(query, keys[i])){
      isPresent = true;
      QList<QMap<QString, QVariant> > vals = fileMeta.values(keys[i]);
      
      qDebug() << "FileStore: found something";
      
      for(int j = 0; j < vals.count(); ++j){
	
	retFiles.append(keys[i]);
	retBlocks.append(vals[j]["block"].toByteArray());
      }
    }
      
  }
  
  (*ret)["MatchNames"] = ConvertStrings(retFiles);
  (*ret)["MatchIDs"] = ConvertBytes(retBlocks);
  (*ret)["SearchReply"] = query;

  return isPresent;
}

bool
FileStore::Match(const QString& query, const QString& key)
{
  QStringList keyWords = query.split(" ", QString::SkipEmptyParts);
  
  for(int i = 0; i < keyWords.count(); ++i)
    if (key.contains(keyWords[i]))
      return true;


  return false;
}

bool
FileStore::ReturnBlock(QByteArray index, QByteArray *ptr, QByteArray *indexPtr)
{
  if (!blockMap.contains(index)){
    return false;
  }
  else{
    
    QMap<QString, QVariant> value = blockMap[index];
    *ptr = value["block"].toByteArray();
    *indexPtr = index;
    return true;
  }
}


