#include <QDebug>
#include <QtCrypto>
#include <QFile>
#include <QByteArray>
#include <QPair>
#include <QObject>
#include <QString>
#include <QVariant>


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
  
  qint64 size = file.size();
  QDataStream fileStream(&file);
  HashBlocks(fileStream, inputFile);

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
FileStore::HashBlocks(QDataStream& s, const QString& fileName)
{  
  char data[BLOCK_SIZE];
  int numRead;
  
  QByteArray result;
  QDataStream resultStream(&result, QIODevice::Append);
  qint64 resultSize = 0;

  while((numRead = s.readRawData(data, BLOCK_SIZE)) != -1){
    
    QByteArray hashArray = Hash(data, numRead);
    quint32 hash = ConvertHash(hashArray);
    MapBlock(hash, data, numRead, fileName);
    
    resultStream.writeRawData(hashArray.data(), BLOCK_SIZE);
    resultSize += HASH_SIZE;
    
    if (numRead < BLOCK_SIZE)
      break;
  }
  
  if (resultSize > BLOCK_SIZE){
    
    QDataStream resultROStream(&result, QIODevice::ReadOnly);
    HashBlocks(resultROStream, fileName);
  }  
  
  // We need to add one more master block.
  else {
    
    QByteArray hashArray = Hash(result.data(), resultSize);
    quint32 hash = ConvertHash(hashArray);
    MapBlock(hash, result.data(), resultSize, fileName);    
  }
}

QByteArray
FileStore::Hash(const char *data, int size)
{
  QCA::Hash shaHash("sha256");
  
  shaHash.update(data, size);
  return shaHash.final().toByteArray();
}

quint32
FileStore::ConvertHash(const QByteArray& arr)
{
  bool ok;
  QString hashAsString = QCA::arrayToHex(arr);
  quint32 hash = hashAsString.toUInt(&ok, 16);
  
  if (!ok){
    qDebug() << "Couldn't convert hash to uint";
    *((int *)NULL) = 1;      
  }
  
  return hash;
}

void
FileStore::MapBlock(quint32 hash,
		    const char *data,
		    int size,
		    const QString& fileName)
{
  QByteArray block(data, size);
  
  QMap<QString, QVariant> val;  
  
  val["name"] = fileName;
  val["block"] = block;
  
  files[hash] = val;
}

/*
QByteArray
FileStore::ReturnBlock(quint32 index)
{
  QList<QMap<QString, QVariant> > values = files.values();
  
  for(int i = 0; i < values.count(); ++i){
    
    QList<QVariant> fileHash = values[i]["hash"].toList();
    
    for(int j = 0; j < fileHash.count(); ++j){
      
      
    }
    
  }
}

// If this function is called, then its'
// safe read 4 bytes.
quint32
FileStore::ParseInt(char *data)
{
  
}
*/
