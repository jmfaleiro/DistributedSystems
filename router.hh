#ifndef ROUTER_HH
#define ROUTER_HH

#include <QObject>
#include <QHash>
#include <QString>
#include <QHostAddress>

class Router : public QObject
{
  Q_OBJECT

public:
  Router();


void 
processRumor(const QVariantMap& rumor, 
	       const QVariantMap& vectorClock,
	       const QHostAddress& sender,
	       const quint16 port);

  
private:
QHash<QString, QPair<QHostAddress, quint16> > routingTable;
};

#endif
