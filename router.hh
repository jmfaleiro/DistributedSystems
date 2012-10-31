#ifndef ROUTER_HH
#define ROUTER_HH

class NetSocket;

#include <QObject>
#include <QHash>
#include <QString>
#include <QHostAddress>
#include <QTimer>
#include <QMap>
#include <QVariantMap>

class Router : public QObject
{
  Q_OBJECT

public:
  Router(NetSocket *ns, bool nf);
  QString me;

void 
processRumor(const QVariantMap& rumor, 
     	       const QHostAddress& sender,
     	       const quint16 port);

public slots:

void
sendMessage(const QString& message, const QString& destination);

void
sendMap(QMap<QString, QVariant> &mesg,
	const QString &destination);

void 
receiveMessage(QVariantMap& msg);
  
signals:

void 
privateMessage(const QString&message, const QString &origin);

void 
newOrigin(const QString& origin);

void
toFileRequests(const QMap<QString, QVariant> &msg);

void
blockRequest(const QMap<QString, QVariant>&msg);
  
private:
  QHash<QString, QPair<QHostAddress, quint16> > routingTable;
  QHash<QString, quint32> currHighest;
  NetSocket *sock;
  QTimer timer;
  bool noForward;

};

#endif
