

#include "router.hh"
#include "helper.hh"

#include "main.hh"

#define HOP_LIMIT 10

Router::Router(NetSocket *socket)
{
  sock = socket;
}



bool
Router::processRumor(const QVariantMap& rumor, 
		     const QHostAddress& sender,
		     const quint16 port)
{

  QString origin = rumor["Origin"].toString();
  bool ret = !routingTable.contains(origin);
    
  routingTable[rumor["Origin"].toString()] = QPair<QHostAddress, quint16>(sender, port);
  return ret;
  
}


void
Router::sendMessage(const QString& message,const QString& destination)
{
  QVariantMap messageMap;
  
  messageMap["Dest"] = destination;
  messageMap["ChatText"] = message;
  messageMap["HopLimit"] = HOP_LIMIT;
  messageMap["Origin"] = me;
  

  QByteArray arr = Helper::SerializeMap(messageMap);
  
  QPair<QHostAddress, quint16> dest = routingTable[destination];
  sock->writeDatagram(arr, dest.first, dest.second);
}



void 
Router::receiveMessage(QVariantMap& msg)
{
  
  QString destination = msg["Dest"].toString();
  int hopLimit;
  
  if (destination == me){
    
    emit privateMessage(msg["ChatText"].toString(), msg["Origin"].toString());
  }
  
  else if (hopLimit > 0){
    
    hopLimit = msg["HopLimit"].toInt() - 1;

    if (hopLimit != 0){
      
      msg["HopLimit"] = hopLimit;
      QPair<QHostAddress, quint16> dest = routingTable[destination];
      QByteArray arr = Helper::SerializeMap(msg);
      sock->writeDatagram(arr, dest.first, dest.second);
    }
  }
}



