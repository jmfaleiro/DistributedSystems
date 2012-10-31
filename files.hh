#ifndef FILES_HH
#define FILES_HH

#include <fileconstants.hh>

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
#include <QUuid>

class FileStore : public QObject
{
  Q_OBJECT

public:

  FileStore();

  bool
  Search(const QString& query, QMap<QString, QVariant> *ret);

  bool
  ReturnBlock(QByteArray index, QByteArray *ptr, QByteArray *indexPtr);    

	  
public slots:

  void
  IndexFiles(const QStringList & files);
  
  

private:
  
  QList<QVariant>
  ConvertBytes(const QList<QByteArray> & hashes);

  
  QList<QVariant>
  ConvertStrings(const QList<QString>&strings);

  
  void
  IndexSingleFile(const QString& file);
  
  
  void
  HashBlocks(QDataStream& s, const QString& fileName, quint32 level);

  
  QByteArray
  Hash(const char *data, int size);

  quint32
  ConvertHash(const QByteArray& arr);

  void
  MapBlock(QByteArray hash,
	   const char *data,
	   int size,
	   const QString& fileName,
	   quint32 level);


  void
  MapMaster(QByteArray hash,
	    quint32 level,
	    const QString& fileName);

  bool
  Match(const QString &query, const QString &key);
  
  QHash<QByteArray, QMap<QString, QVariant> > blockMap;
  QHash<QString, QMap<QString, QVariant> > fileMeta;

};

#endif // FILES_HH
