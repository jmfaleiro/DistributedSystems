#ifndef FILEREQUESTS_HH
#define FILEREQUESTS_HH

#include <fileconstants.hh>

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QString>
#include <QList>
#include <QHash>
#include <QPair>
#include <QTimer>
#include <QByteArray>

class FileRequests : public QObject
{
  Q_OBJECT
  
public:
 
  FileRequests(const QString& my_name);
 

signals:
  
  void
  broadcastRequest(const QMap<QString, QVariant> & msg);
			
  void
  newResponse(const QString& query, const QMap<QString, QVariant>&repsonse);
  
  void
  sendDownloadMsg(QMap<QString, QVariant> &msg, const QString &destination);									  
  
									  
public slots:
  
  void
  processDownloadTimeout();
  
  void
  processReply(const QMap<QString, QVariant>&msg);

  
  void
  newSearch(const QString& queryString);
  
  
  void
  newDownload(const QString& fileName, const QString&destination, const QByteArray& masterBlock);
  
  
  void
  processTimeout();

  void
  destroyRequest(const QString& queryString);



private:  
  
  void
  processBlockReply(const QMap<QString, QVariant> & msg);
  
  void
  writeFile(const QByteArray &hash);
  
  void
  destroyDownloadData(const QByteArray &hash);

  bool
  downloadCompleted(const QByteArray &hash);

  bool
  verifyData(const QByteArray &hash, const QByteArray &data);
  
  void
  processSearchReply(const QMap<QString, QVariant>& msg);
  
  QHash<QString, QMap<QString, QVariant> > searchResponses;
  QHash<QString, quint32> pendingSearches;  

  QHash<QByteArray, QPair<QString, QString> > pendingDownloads;
  QHash<QByteArray, QByteArray> pendingDownloadData;
  QHash<QByteArray, QByteArray> blockHashes;
  QHash<QByteArray, QList<QByteArray> >invertBlockHashes;
  
  QTimer timer;
  QString me;
  quint32 timerDuration;
  QTimer downloadTimer;

};

#endif // FILEREQUESTS_HH
