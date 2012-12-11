

#include "router.hh"
#include "helper.hh"

#include "main.hh"

#define HOP_LIMIT 10

Router::Router(NetSocket *socket, bool nf)
{
  sock = socket;
  noForward = nf;
}

void
Router::processRumor(const QVariantMap& rumor, 
		     const QHostAddress& sender,
		     const quint16 port)
{


    //qDebug() << rumor;
  QString origin = rumor["Origin"].toString();
  quint32 seqNo = rumor["SeqNo"].toInt();

  if (origin != me){
  
    bool neworg = !routingTable.contains(origin);

  
    if (neworg || currHighest[origin] < seqNo){
      currHighest[origin] = seqNo;
      routingTable[origin] = QPair<QHostAddress, quint16>(sender, port);
    }
    
    else if (rumor.contains("LastIP") && rumor.contains("LastPort")){

      
      if (seqNo == currHighest[origin]){
	
	
	QHostAddress holeIP(rumor["LastIP"].toInt());
	quint16 holePort = rumor["LastPort"].toInt();
	routingTable[origin] = QPair<QHostAddress, quint16>(holeIP, holePort);
      }
    }
	     
    
    if(neworg){
      emit newOrigin(origin);  
    }
  }
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

  if (routingTable.contains(destination)){
    
    QPair<QHostAddress, quint16> dest = routingTable[destination];
    //  qDebug() << "Sending: " << message;
    //qDebug() << dest.first << ":" << dest.second;
    sock->writeDatagram(arr, dest.first, dest.second);
  }
}



void
Router::sendMap(const QMap<QString, QVariant>& msg, const QString& destination)
{
  QMap<QString, QVariant> real_msg = msg;
  
  real_msg["Dest"] = destination;
  real_msg["HopLimit"] = HOP_LIMIT;
  real_msg["Origin"] = me;  
  
  QByteArray arr = Helper::SerializeMap(real_msg);
  if (routingTable.contains(destination)){
    
    QPair<QHostAddress, quint16> dest = routingTable[destination];
    sock->writeDatagram(arr, dest.first, dest.second);
  }

  //qDebug() << msg;
}

void 
Router::receiveMessage(QVariantMap& msg)
{
  //qDebug() << "Received: " << msg;
  
  QString destination = msg["Dest"].toString();
  int hopLimit = msg["HopLimit"].toInt();
  

  if (destination == me){
    
    if (msg.contains("ChatText"))
      emit privateMessage(msg["ChatText"].toString(), msg["Origin"].toString());
    else if (msg.contains("BlockRequest")){
      //qDebug() << "Got block request, sending to dispatcher";
      emit blockRequest(msg);
    }
    else if ((msg.contains("BlockReply") && msg.contains("Data")) ||
	     (msg.contains("SearchReply") && msg.contains("MatchNames") && msg.contains("MatchIDs"))){
      //qDebug() << "Got reply, sending to filerequests";
      emit toFileRequests(msg);
    }
    else if (msg.contains("Paxos")){
      qDebug() << "Got paxos message, sending to paxos";
      emit toPaxos(msg);
    }
  }
  
  else if (!noForward && hopLimit > 0){
    
    if (--hopLimit != 0){
      
      msg["HopLimit"] = hopLimit;
      QPair<QHostAddress, quint16> dest = routingTable[destination];
      QByteArray arr = Helper::SerializeMap(msg);
      sock->writeDatagram(arr, dest.first, dest.second);
    }
  }
}



