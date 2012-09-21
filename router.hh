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
  Router(NetSocket *ns);
  QString me;

bool
processRumor(const QVariantMap& rumor, 
	       const QHostAddress& sender,
	       const quint16 port);

public slots:

void
sendMessage(const QString& message, const QString& destination);

void 
receiveMessage(QVariantMap& msg);
  
signals:

void 
privateMessage(const QString&message, const QString &origin);


private:
  QHash<QString, QPair<QHostAddress, quint16> > routingTable;
  NetSocket *sock;
  QTimer timer;

};

#endif
