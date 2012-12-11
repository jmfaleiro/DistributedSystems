#ifndef DISPATCHER_HH
#define DISPATCHER_HH

#include <QObject>
#include <QHash>
#include <QUuid>
#include <QVariant>
#include <QMap>
#include <QString>

#include <files.hh>

class NetSocket;

class Dispatcher : public QObject
{
  Q_OBJECT
  
public:
  Dispatcher(FileStore* fs, NetSocket *netsocket);
  
  

public slots:
  void
  processRequest(const QMap<QString, QVariant> &request);


  /*
  void
  finishRequest(QUuid &requestId, QMap<QString, QVariant> &response);
  */

signals:
  void
  reply(const QMap<QString, QVariant> &ret, const QString& dest);
							  
  void
  sendNeighbor(const QMap<QString, QVariant> &ret, quint32 index);
  
private:
  //QHash<QUuid, QMap<QString, QVariant> > pendingRequests;

  void
  distributeBudget(quint32 b, const QMap<QString, QVariant> &request);

  
  bool
  isBlockRequest(const QMap<QString, QVariant> &request);

  bool 
  isSearchRequest(const QMap<QString, QVariant> & request);

  
  
  FileStore *m_fs;
  NetSocket *m_netsocket;
};

#endif // DISPATCHER_HH
