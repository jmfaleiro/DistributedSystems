#ifndef FILES_HH
#define FILES_HH

#define BLOCK_SIZE 8192 
#define HASH_SIZE 32

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPair>
#include <QByteArray>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QMap>

class FileStore : public QObject
{
  Q_OBJECT

public:

  FileStore();

public slots:

  void
  IndexFiles(const QStringList & files);

private:
  
  QList<QVariant>
  ConvertBytes(const QList<QByteArray> & hashes);
  
  void
  IndexSingleFile(const QString& file);
  
  
  void
  HashBlocks(QDataStream& s, const QString& fileName);

  
  QByteArray
  Hash(const char *data, int size);

  quint32
  ConvertHash(const QByteArray& arr);

  void
  MapBlock(quint32 hash,
	   const char *data,
	   int size,
	   const QString& fileName);


    
  
  QHash<quint32, QMap<QString, QVariant> > files;

};

#endif // FILES_HH
