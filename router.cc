#include <router.hh>

Router::Router()
{

}


void 
Router::processRumor(const QVariantMap& rumor, 
		     const QVariantMap& vectorClock,
		     const QHostAddress& sender,
		     const quint16 port)
{
  if (vectorClock.contains(rumor["Origin"].toString())){

    if (vectorClock[rumor["Origin"].toString()].toUInt() < 
	rumor["SeqNo"].toUInt()){
      
      routingTable[rumor["Origin"].toString()] = QPair<QHostAddress, quint16> (sender, port);
      
      qDebug() << "Router::processRumor -- Added " << rumor["Origin"].toString() << sender << port;
    }
  }
  
  else{

    routingTable[rumor["Origin"].toString()] = QPair<QHostAddress, quint16> (sender, port);
    qDebug() << "Router::processRumor -- Added " << rumor["Origin"].toString() << sender << port;
  }
  
}





