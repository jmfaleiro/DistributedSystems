#ifndef FILES_HH
#define FILES_HH

#define BLOCK_SIZE 8192 

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
  
  QList<QByteArray>
  HashBlocks(QDataStream& s);
  
  QHash<QString, QMap<QString, QVariant> > files;
};

#endif // FILES_HH
