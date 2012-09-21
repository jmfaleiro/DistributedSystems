#include "neighbors.hh"

NeighborList::NeighborList()
{
  QHostInfo myInformation = QHostInfo::fromName(QHostInfo::localHostName());
  QList<QHostAddress> myAddresses = myInformation.addresses();
  
  for(int i = 0; i < myAddresses.count(); ++i){
    
    
    if (myAddresses[i].toString() != "127.0.0.1"){
      
      if (checkIfWellFormedIP(myAddresses[i].toString())){
	myIP = myAddresses[i].toString();
	qDebug() << "found my address!!!";
	qDebug() << myIP;
	
      }
    }
  }
  

}

QPair<QHostAddress, quint16> NeighborList::randomNeighbor()
{
  return neighbors[qrand() % neighbors.count()];
}

void NeighborList::addNeighbor(const QHostAddress& addr, quint16 port)
{
  int len = neighbors.count();
  bool seen = false;
  for (int i = 0; i < len; ++i){
    
    if (neighbors[i].first == addr &&
	neighbors[i].second == port)
      seen = true;
      
  }

  if (!seen){
    neighbors.append(QPair<QHostAddress, quint16>(addr, port));
  }
}

void NeighborList::addHost(const QString& s)
{
    qDebug() << "NetSocket::addHost -- at least I got called";

  
  QStringList parts = s.split(":");
  if (parts.count() != 2){
    qDebug() << "NetSocket::AddHost didn't receive the right format, expected \"(ipaddr|hostname):port\"" << s;
    return;
  }

  bool correct;
  quint32 port = parts[1].toUInt(&correct);
  if (!correct){
    
    qDebug() << "NetSocket::AddHost received invalid port";
    return;
  }

  qDebug() << "NetSocket::addHost -- got a well formed addresss!!!";
  QHostAddress addr;

  // Success, we parsed the ip address and we're done!
  // Make sure that we don't add the same host:port combination twice!!!
  if (addr.setAddress(parts[0])){
    
    if (parts[0] != myIP){
      for(int i = 0; i < neighbors.count(); ++i){
      
	// Check if we've already added this host before.
	if (((neighbors[i]).first == addr) && ((neighbors[i]).second == port)){
	
	  qDebug() << "NetSocket::addHost -- already added " << addr.toString() << ":" << port;

	  return;	  
	}
      }
      
      QPair<QHostAddress, quint16> peer(addr, (quint16)port);
      neighbors.append(peer);
    }

  }
  
  // The ip address wasn't properly parsed, maybe it's a hostname?
  else{


    if (pendingLookups.contains(parts[0])){
      
      if ((pendingLookups[parts[0]]).indexOf(port) >= 0){

	return;
      }

      else{
	
	pendingLookups[parts[0]].append(port);
	return;
      }

    }

    else {
      
      //pendingLookups[parts[0]] = *(new QList<quint16>());
      pendingLookups[parts[0]].append(port);
      qDebug() << "aha! this works!";
      QHostInfo::lookupHost(parts[0], this, SLOT(lookedUpHost(const QHostInfo&)));
      return;
    }
    
  }  

}

void NeighborList::lookedUpHost(const QHostInfo& host)
{
  if (host.error() != QHostInfo::NoError) {
    qDebug() << "NetSocket::lookedUpHost -- lookup failed for " << host.hostName();

  }

  else {
    
    if (!host.addresses().isEmpty()){
      
      for(int i = 0; i < host.addresses().count(); ++i){
	
	QHostAddress curr = host.addresses()[i];
	
	if (checkIfWellFormedIP(curr.toString())){
	  
	  if (myIP != curr.toString() && "127.0.0.1" != curr.toString()){
	    
	    for(int i = 0; i < pendingLookups[host.hostName()].count(); ++i){

	      

	      QPair<QHostAddress, quint16> peer(curr,pendingLookups[host.hostName()][i]);

	      qDebug() << "Added Neighbor " << host.hostName() << " " << peer;
	      neighbors.append(peer);
	    }
	  
	  }
	  break;
	}

      }
    }

  }
  pendingLookups.remove(host.hostName());
  qDebug() << pendingLookups;
}


bool NeighborList::checkIfWellFormedIP(const QString& addr)
{
  return true;
  
  QStringList parts = addr.split('.');
  
  bool ret = false;
  if (parts.count() == 4){
    
    for(int i = 0; i < 4; ++i){
      
      parts[i].toInt(&ret);
      
      if (!ret){
	qDebug() << "The IP address " << addr << " is not well formed!!!";
	return ret;
      }
    }

  }
  
  qDebug() << "The IP address " << addr << " is well formed!!!";
  return ret;
	
}
