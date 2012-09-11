
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QTextStream>
#include <QtGlobal>
#include "main.hh"


// BEGIN: TextEntryWidget

TextEntryWidget::TextEntryWidget(QWidget * parent) : QTextEdit(parent)  
{
  
}

void TextEntryWidget::keyPressEvent(QKeyEvent *e)
{
  if (e->key() == Qt::Key_Enter || e->key() == Qt::Key_Return){
    emit returnPressed();
  }
  else{
    QTextEdit::keyPressEvent(e);
  }
}

// END: TextEntryWidget


// Begin: ChatDialog

ChatDialog::ChatDialog()
{
	setWindowTitle("Peerster");

	// Read-only text box where we display messages from everyone.
	// This widget expands both horizontally and vertically.
	textview = new QTextEdit(this);
	textview->setReadOnly(true);

	// Small text-entry box the user can enter messages.
	// This widget normally expands only horizontally,
	// leaving extra vertical space for the textview widget.
	//
	// You might change this into a read/write QTextEdit,
	// so that the user can easily enter multi-line messages.
	textline = new TextEntryWidget(this);
	textline->setFocus();

	
	peerAdder = new QLineEdit(this);
	

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(peerAdder);
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);
	

	
	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));


	connect(peerAdder, SIGNAL(returnPressed()),
		this, SLOT(gotAddPeer()));
	
}

void ChatDialog::gotAddPeer()
{
  QString temp = peerAdder->text();
  peerAdder->clear();
  qDebug() << "ChatDialog::gotAddPeer -- received request to add " << temp;
  emit this->addPeer(temp); 
}

void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...
  qDebug() << "FIX: send message to other peers: " << textline->toPlainText();
  //textview->append(textline->toPlainText());

  emit this->sendMessage (textline->toPlainText());

	// Clear the textline to get ready for the next input message.
	textline->clear();

	
}

void ChatDialog::gotNewMessage(const QString& s)
{
  qDebug() << "ChatDialog::gotNewMessage -- ok, at least I get called" << '\n';
  textview->append(s);
}

// End: ChatDialog

//Begin: NetSocket

NetSocket::NetSocket()
{
	// Pick a range of four UDP ports to try to allocate by default,
	// computed based on my Unix user ID.
	// This makes it trivial for up to four Peerster instances per user
	// to find each other on the same host,
	// barring UDP port conflicts with other applications
	// (which are quite possible).
	// We use the range from 32768 to 49151 for this purpose.
	myPortMin = 32768 + (getuid() % 4096)*4;
	myPortMax = myPortMin + 3;
	
	localhost = new QHostAddress(QHostAddress::LocalHost);
	vectorClock = new QVariantMap();
	
	messageIdCounter = 1;
	
	pendingLookups = new QMap<QString, QList<quint16> >();

	neighborsVisited = new QSet<quint32>();
	messages = new QMap<QString, QList<QByteArray> >();
	rumorTimer = new QTimer();

	anythingHot = false;
	antiEntropyTimer = new QTimer();
	antiEntropyTimer->setSingleShot(false);
	antiEntropyTimer->start(10000);
	
	

	EMPTY_BYTE_ARRAY = new QByteArray();
	QObject::connect(this, SIGNAL(readyRead()),
			 this, SLOT(readData()));

	QObject::connect(this, SIGNAL(startRumorTimer(int)),
			 rumorTimer, SLOT(start(int)));

	QObject::connect(rumorTimer, SIGNAL(timeout()),
			 this, SLOT(processTimeout()));
	
	
	
	QObject::connect(antiEntropyTimer, SIGNAL(timeout()),
			 this, SLOT(processAntiEntropyTimeout()));
			 
	
}


// After receiving a time-out from the antientropy timer, send 
// my vector clock to a random neighbor.
void NetSocket::processAntiEntropyTimeout()
{
  if (neighbors->count() > 0){
    
    int index = qrand() % (neighbors->count());
  
  
    sendStatusMessage((*neighbors)[index].first, (*neighbors)[index].second);
  }
}



bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
  quint16 qMyPortMin = (quint16) myPortMin;
  quint16 qMyPortMax = (quint16) myPortMax;

  // Find and register all the neighbors. For the time being they are just those on the same
  // host on different ports.
  for (quint16 p = qMyPortMin; p <= qMyPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;

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

			qDebug() << "something";
			
			neighbors = new QList<QPair<QHostAddress, quint16> >();
			QPair<QHostAddress, quint16>* ahead;
			QPair<QHostAddress, quint16>* behind;
			
			me = new QPair<QHostAddress, quint16>(*localhost, p);	
			/*
			if (p == qMyPortMin){
			  
			  ahead = new QPair<QHostAddress, quint16>(*localhost, p + 1);
			  neighbors->append(*ahead);
		
			  
			}

			else if (p == qMyPortMin + 1){

			  ahead = new QPair<QHostAddress, quint16>(*localhost, p + 1);
			  behind = new QPair<QHostAddress, quint16>(*localhost, p - 1);
			  
			  neighbors->append(*ahead);
			  neighbors->append(*behind);




			}

			else if (p == qMyPortMin + 2){

			  ahead = new QPair<QHostAddress, quint16>(*localhost, p + 1);
			  behind = new QPair<QHostAddress, quint16>(*localhost, p - 1);

			  neighbors->append(*ahead);
			  neighbors->append(*behind);
			   



			}

			else if (p == qMyPortMin + 3){

			  behind = new QPair<QHostAddress, quint16>(*localhost, p - 1);

			  neighbors->append(*behind);


			}
			*/
			

			/*
			for (quint16 q = qMyPortMin; q <= qMyPortMax; q++) {
			  
			  if (p != q)			    
			    neighbors->append(*localhost, q);
			  			  
			}
			*/

			

			QStringList args = QCoreApplication::arguments();
			
			int max = args.count();
			for(int i = 1; i < max; ++i){
			  
			  qDebug() << args[i];			  
			  addHost(args[i]);
			}
			
			QTextStream *stream = new QTextStream(&myNameString);
			
			(*stream) << QHostInfo::localHostName() << ":" << p << qrand();
			
			myNameVariant = new QVariant(myNameString);
			
			stream->~QTextStream();
			qDebug() << "Finished intialization!!!";
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}


// Received a message from the dialog.
// Construct the message as a QVariantMap and 
// call the function to handle received rumors.
void NetSocket::gotSendMessage(const QString &s)
{

  QVariantMap *udpBodyAsMap = new QVariantMap();

    
  // Put the values in the map.
  (*udpBodyAsMap)["ChatText"] = s;
  (*udpBodyAsMap)["SeqNo"] = messageIdCounter++;
  (*udpBodyAsMap)["Origin"] = myNameString;
  

  
  QByteArray *arr = new QByteArray();

  QDataStream *stream = new QDataStream(arr, QIODevice::Append);
  (*stream) << (*udpBodyAsMap);
  
  newRumor(*udpBodyAsMap, QHostAddress::LocalHost, 0);

}


// Send status message to the given address:port combination.
// Reads the current state of the vector clock.
void NetSocket::sendStatusMessage(QHostAddress address, quint16 port)
{
  QVariantMap *udpBodyAsMap = new QVariantMap();
  
  qDebug() << "NetSocker::sendStatusMessage " << *vectorClock;
  (*udpBodyAsMap)["Want"] = *vectorClock;

  QByteArray *arr = new QByteArray();
  
  QDataStream *s = new QDataStream(arr, QIODevice::Append);
  (*s) << (*udpBodyAsMap);
    
  this->writeDatagram(*arr, address, port);
  qDebug() << "NetSocket:sendStatusMessage -- finished sending status!";
  
}

// We might have received a new rumor.
// a) If it is from the dialog, then it is definitely new.
//
// b) If it is from the network, then it might not be new,
//    we have to account for this by checking the expected 
//    value from the vector clock with the sequence number
//    in the message.
//
// c) If we have a new message, start rumor mongering.
// 
// d) Only this function manipulates the vector clock and messages.
//
// e) Only this function sets anythingHot to true.
void NetSocket::newRumor(const QVariantMap& readmessage,const  QHostAddress& senderAddress,const quint16& port)
{

  rumorTimer->stop();
  QString origin = readmessage["Origin"].toString();
  

  quint32 expected = (vectorClock->contains(origin))? (*vectorClock)[origin].toUInt() : 1;
    
  if (vectorClock->contains(origin))
    expected = (*vectorClock)[origin].toUInt();
  else
    expected = 1;
  
  
  if (expected == readmessage["SeqNo"].toUInt()){
        
    hotMessage = readmessage;
    cleanUpVisited();
    

    QByteArray *arr = new QByteArray();
    QDataStream *s = new QDataStream(arr, QIODevice::Append);
    (*s) << hotMessage;

    qDebug() << "NetSocket::newRumor -- yay, in-order message!!!";
      
    emit receivedMessage ((readmessage["ChatText"]).toString());    	

    (*vectorClock)[origin] = expected + 1;
	
    if (expected == 1){
      QList<QByteArray> *temp = new QList<QByteArray>();
      (*messages)[origin] = *temp;
      (*messages)[origin].append(*EMPTY_BYTE_ARRAY);
    }
    (*messages)[origin].append(*arr);

    if (origin != myNameString){
      sendStatusMessage(senderAddress, port);
    }
  
    // Make sure we don't send the message back to the origin:
    //QStringList temp = origin.split(":");
    
    //excludeNeighbor(temp[1].toUInt());

    anythingHot = true;
    
    int index;
    if  ((index = randomNeighbor()) >= 0){
    
      qDebug() << "NetSocket::newRumor -- random number = " << index;

    
      this->writeDatagram(*arr, (*neighbors)[index].first, (*neighbors)[index].second);   

	          
      emit startRumorTimer(2000);
    }

    
  }
}


// Some utility methods to keep track of which neighbors we have already sent 
// messages to for the same hotmessage. If we change hotmessages, then we have to clean up!!!
// WE ONLY CLEAN UP IN newRumor!!!
void NetSocket::excludeNeighbor(quint32 port)
{
  for (int i = 0; i < neighbors->count(); ++i){
    
    if ((*neighbors)[i].second == port){
      neighborsVisited->insert(i);
      return;
    }

  }
}

void NetSocket::cleanUpVisited()
{
    neighborsVisited->clear();
}

int NetSocket::randomNeighbor()
{
  
  if (neighbors->count() == 0){
    return -1;
  }
  if (neighbors->count() == neighborsVisited->count()){
    return -1;
  }
  
  int index;
  while (neighborsVisited->contains(index = (qrand() % (neighbors->count()))))    
    ;

  neighborsVisited->insert(index);
  
  return index;  
}

// If none of the elements are bigger, the string is empty.
QString NetSocket::tryFindFirstBigger(const QVariantMap& map1, const QVariantMap& map2, int *wanted)
{
  QList<QString> keys = map1.keys();
  for(int i = 0; i < keys.count(); ++i){
    
    if (map2.contains(keys[i])){
      
      qDebug() << "NetSocket::tryFindFirstBigger -- first if ok";
      // both have the key, but map1's is higher: Success!!!
      if (map1[keys[i]].toUInt() > map2[keys[i]].toUInt()){
	

	*wanted = map2[keys[i]].toUInt();

	qDebug() << "NetSocket::tryFindFirstBigger -- inner if ok";
	return keys[i];

      }
      
      else
	continue;
    } 


    // map2 does not have the key: Success!!!
    else{
     
      *wanted = 1;
      return keys[i];

    }

  }
    
  return "";
}

void NetSocket::addHost(const QString& s)
{
  qDebug() << "NetSocket::addHost -- at least I got called";
  QPair<QHostAddress, quint16> peer;
  
  QStringList parts = s.split(":");
  if (parts.count() != 2){
    qDebug() << "NetSocket::AddHost didn't receive the right format, expected \"(ipaddr|hostname):port\"";
    return;
  }

  bool correct;
  quint32 port = parts[1].toUInt(&correct);
  if (!correct){
    
    qDebug() << "NetSocket::AddHost received invalid port";
    return;
  }

  qDebug() << "NetSocket::addHost -- got a well formed addresss!!!";
  QHostAddress* addr = new QHostAddress();

  // Success, we parsed the ip address and we're done!
  // Make sure that we don't add the same host:port combination twice!!!
  if (addr->setAddress(parts[0])){
    
    if (parts[0] != myIP){
      for(int i = 0; i < neighbors->count(); ++i){
      
	// Check if we've already added this host before.
	if (((*neighbors)[i].first == *addr) && ((*neighbors)[i].second == port)){
	
	  qDebug() << "NetSocket::addHost -- already added " << addr->toString() << ":" << port;
	  addr->~QHostAddress();
	  return;	  
	}
      }

      peer = *(new QPair<QHostAddress, quint16>(*addr, (quint16)port));
      (*neighbors).append(peer);
    }

  }
  
  // The ip address wasn't properly parsed, maybe it's a hostname?
  else{


    if (pendingLookups->contains(parts[0])){
      
      if ((*pendingLookups)[parts[0]].indexOf(port) >= 0){

	return;
      }

      else{
	
	(*pendingLookups)[parts[0]].append(port);
	return;
      }

    }

    else {
      
      (*pendingLookups)[parts[0]] = *(new QList<quint16>());
      (*pendingLookups)[parts[0]].append(port);

      QHostInfo::lookupHost(parts[0], this, SLOT(lookedUpHost(const QHostInfo&)));
      return;
    }
    
  }  
  
}

bool NetSocket::checkIfWellFormedIP(const QString& addr)
{
  
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

// Taken from qt documentation on how to use QHostInfo to 
// lookup a host's ip address.
void NetSocket::lookedUpHost(const QHostInfo& host)
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
	    
	    for(int i = 0; i < (*pendingLookups)[host.hostName()].count(); ++i){
	    
	      QPair<QHostAddress, quint16> *peer = new QPair<QHostAddress, quint16>(curr, 
										    (*pendingLookups)[host.hostName()][i]);
	      neighbors->append(*peer);
	    }
	  
	  }
	}

      }
    }

  }
  pendingLookups->remove(host.hostName());
}

// Called when we receive a new status message.
// 
// Handles *both* messages due to rumor mongering and anti-entropy.
//
// a) If we have nothing hot, then it's safe to rumor monger
//    with just the host that sent us the status message.
//
// b) If anythingHot is true, we have to make sure that we also accommodate for
//    rumormongering with all neighbors.

void NetSocket::newStatus(const QVariantMap& message,
			   const QHostAddress& senderAddress, 
			   const quint16& port)
{
  rumorTimer->stop();
  QString ans;
  int required;

  
  
  qDebug() << "NetSocket::newStatus " << message["Want"];

  // Our vector is bigger!!!
  if ((ans = tryFindFirstBigger(*vectorClock, message["Want"].toMap(), &required)) != ""){
    
    qDebug() << "NetSocket::newStatus -- our vector is bigger!!!";
    qDebug() << "NetSocket:: newStatus -- neighbor wants " << ans << ":" << required;
    
    this->writeDatagram((*messages)[ans][required], senderAddress, port);
    qDebug() << "NetSocket::newStatus -- wrote required message!!!";
    qDebug() << '\n';
    emit startRumorTimer(2000);   
  }

  // Her's is bigger :(
  else if ((ans = tryFindFirstBigger(message["Want"].toMap(), *vectorClock, &required)) != ""){
    
    qDebug() << "NetSocket::newStatus -- her's is bigger!!!";
    QVariantMap *temp = new QVariantMap();
    (*temp)["Want"] = *vectorClock;
    QByteArray *arr = new QByteArray();
    QDataStream *stream = new QDataStream(arr, QIODevice::Append);
    
    (*stream) << (*temp);    
    this->writeDatagram(*arr, senderAddress, port);    
    qDebug() << "NetSocket::newStatus -- wrote our status!!!";
    qDebug() << '\n';
  }  
  
  // Tie: propagate hot message
  else if (anythingHot){
    
    
     qDebug() << "NetSocket::newStatus -- tie!!!";
    // Flip a coin
    if (qrand() % 2){
      qDebug() << "NetSocket::newStatus -- got heads! try to find next neighbor";
      int index;
      if ((index = randomNeighbor()) >= 0){
      
	qDebug() << "NetSocket::newStatus -- send to next neighbor!!!";
	QByteArray *arr = new QByteArray();
	
	QDataStream *stream = new QDataStream(arr, QIODevice::Append);
	(*stream) << hotMessage;


	this->writeDatagram(*arr, (*neighbors)[index].first, (*neighbors)[index].second);
	qDebug() << "NetSocket::newStatus -- sent message!!!";
	emit startRumorTimer(2000);
      }
      else{
	qDebug() << "NetSocket::newStatus -- no more neighbors!!!";
	anythingHot = false;
      }

    }
    else {
      qDebug() << "NetSocket::newStatus -- got tails! done!!!";
      qDebug() << '\n';
      anythingHot = false; 
    }
  }
}
  

// Rumormongering timeout.
void NetSocket::processTimeout()
{
  int index;
  rumorTimer->stop();
  if (anythingHot){

    
    if ((index = randomNeighbor()) >= 0){
  
      QByteArray *arr = new QByteArray();

      QDataStream *stream = new QDataStream(arr, QIODevice::Append);
      (*stream) << hotMessage;
	

      this->writeDatagram(*arr, (*neighbors)[index].first, (*neighbors)[index].second);
      emit startRumorTimer(2000);
    }

    else{
      anythingHot = false;
    }
  }
}

// Read data from the network and redirect the message for analysis to 
// either newRumor or newStatus.

void NetSocket::readData()
{
  
  
  const qint64 size = this->pendingDatagramSize();
  if (size == -1){
    qDebug() << "NetSocket::readData() -- Error signalled for new data, but nothing present";
    qDebug() << '\n';
    return;
  }
  char *data  = new char[size+1];
  QHostAddress *senderAddress = new QHostAddress();
  quint16 port = 0;
  
  if (size != this->readDatagram(data, size, senderAddress, &port)){
    qDebug() << "NetSocket::readData() -- Error reading data from socket. Sizes don't match!!!";
  }

  qDebug() << "NetSocket::readData() -- just received a datagram!!!";

  bool seenBefore;
  for(int i = 0; i < neighbors->count(); ++i){
    
    if((*neighbors)[i].first.toString() == (*senderAddress).toString() && 
       (*neighbors)[i].second == port){
      
      qDebug() << "NetSocket::readData() -- seen this host before!!!";
      seenBefore = true;
      break;
    }      
  }

  if (!seenBefore){
    
    QPair<QHostAddress, quint16> *peer = new QPair<QHostAddress, quint16>(*senderAddress, port);
    neighbors->append(*peer);    
  }
  

  QByteArray *arr = new QByteArray(data, size);
  QVariantMap *items = new QVariantMap();
  
  QDataStream *stream = new QDataStream(*arr);
  (*stream) >> (*items);
  
  qDebug() << "Checking if message is rumor or status..."; //Debug Message to make sure we receive the right stuff!!!


  // Rumor message:
  if (items->contains("ChatText") && 
      items->contains("Origin") &&
      items->contains("SeqNo") &&
      !(items->contains("Want"))){
    
    qDebug() << "Rumor!!!";
    newRumor(*items, *senderAddress, port);
  }

  // Status message:
  else if (items->contains("Want") && !(items->contains("ChatText") ||
					items->contains("Origin") ||
					items->contains("SeqNo"))){

    qDebug() << "Status!!!";
    newStatus(*items, *senderAddress, port);
    
  }
	   
  qDebug() << '\n';
}

// END: NetSocket

int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();


	

	
	// Create a UDP network socket
	NetSocket sock;
	if (!sock.bind())
		exit(1);

	QObject::connect(&dialog, SIGNAL(sendMessage(const QString&)),
			 &sock, SLOT(gotSendMessage(const QString&)));


	QObject::connect(&sock, SIGNAL(receivedMessage(const QString&)),
			 &dialog, SLOT(gotNewMessage(const QString&)));


	QObject::connect(&dialog, SIGNAL(addPeer(const QString&)),
			 &sock, SLOT(addHost(const QString&)));

			 

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

