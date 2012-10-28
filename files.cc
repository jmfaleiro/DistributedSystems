#include <QDebug>
#include <QtCrypto>
#include <QFile>
#include <QByteArray>
#include <QPair>
#include <QObject>
#include <QString>
#include <QVariant>


#include <files.hh>

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
  QList<QVariant> fileHash = ConvertBytes(HashBlocks(fileStream));
  
  qDebug() << "Hashed " << inputFile << ". Num levels = " << fileHash.count();

  QMap<QString, QVariant> fileDetails;
  fileDetails["size"] = size;
  fileDetails["hash"] = fileHash;
  
  if (files.contains(inputFile)){
    qDebug() << "File " << inputFile << " already exists.";
    files.remove(inputFile);
  }
  
  files[inputFile] = fileDetails;
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
QList<QByteArray>
FileStore::HashBlocks(QDataStream& s)
{
  char data[2] = {'a', 'b'};
  int numRead;
  
  QList<QByteArray> ret;
  
  QStringList supportedHashes = QCA::Hash::supportedTypes();
  for(int i = 0; i < supportedHashes.count(); ++i)
    qDebug() << supportedHashes[i];
  
  if (!QCA::isSupported("sha256"))
    qDebug() << "sha256 not supported";
  
  QCA::Hash shaHash("sha-256");
  
  QByteArray result;
  QDataStream resultStream(&result, QIODevice::Append);
  qint64 resultSize = 0;

  while((numRead = s.readRawData(data, BLOCK_SIZE)) != -1){
    
    shaHash.update(data, 2);
    
    char* temp = shaHash.final().toByteArray().data();
    resultStream.writeRawData(temp, numRead);
    
    shaHash.clear();
    resultSize += 32;
  }
  
  if (resultSize > 32){
    
    ret << HashBlocks(resultStream);
  }

  ret << result;

  return ret;
}
