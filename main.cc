
#include <unistd.h>


#include <QApplication>
#include <QDebug>
#include <QKeyEvent>
#include <QTextStream>
#include <QtGlobal>
#include <QTime>
#include <QDateTime>
#include <QUuid>
#include <QListWidget>

#include "main.hh"
#include "router.hh"
#include "helper.hh"


// BEGIN: PrivateChatDialog

PrivateChatDialog::PrivateChatDialog(const QString& destination)
{

  m_destination = destination;
  setWindowTitle(destination);


  textview = new QTextEdit(this);
  textview->setReadOnly(true);

  textentry = new QLineEdit(this);
  textentry->setFocus();

  layout = new QVBoxLayout();
  layout->addWidget(textview);
  layout->addWidget(textentry);

  connect(textentry, SIGNAL(returnPressed()),
	  this, SLOT(internalMessageReceived()));
  
  setLayout(layout);
}

PrivateChatDialog::~PrivateChatDialog()
{
  textview->~QTextEdit();
  textentry->~QLineEdit();
  layout->~QVBoxLayout();  
}

void
PrivateChatDialog::internalMessageReceived()
{
  QString msg = textentry->text();
  textview->append(msg);
  emit sendMessage(msg, m_destination);
  textentry->clear();
}


void
PrivateChatDialog::externalMessageReceived(const QString& msg)
{
  textview->append(msg);
}


void
PrivateChatDialog::closeEvent(QCloseEvent *e)
{
  
  emit closed(m_destination);
}

// END: PrivateChatDialog


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

ChatDialog::ChatDialog(Router *r)
{
  router = r;
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
	QHBoxLayout *layout = new QHBoxLayout();
	
	QVBoxLayout *innerLayout = new QVBoxLayout();
	innerLayout->addWidget(peerAdder);
	innerLayout->addWidget(textview);
	innerLayout->addWidget(textline);

	origins = new QListWidget();

	layout->addLayout(innerLayout);
	layout->addWidget(origins);
	
	setLayout(layout);
	

	
	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(router, SIGNAL(newOrigin(const QString&)),
		this, SLOT(addOrigin(const QString&)));
	
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));


	connect(peerAdder, SIGNAL(returnPressed()),
		this, SLOT(gotAddPeer()));
	
	connect(origins, SIGNAL(itemDoubleClicked(QListWidgetItem*)),
		this, SLOT(openEmptyPrivateChat(QListWidgetItem*)));
	
}


void 
ChatDialog::addOrigin(const QString& origin)
{
  origins->addItem(origin);
}

void ChatDialog::gotAddPeer()
{
  QString temp = peerAdder->text();
  peerAdder->clear();
  ////qDebug() << "ChatDialog::gotAddPeer -- received request to add " << temp;
  emit this->addPeer(temp); 
}

void ChatDialog::gotReturnPressed()
{
	// Initially, just echo the string locally.
	// Insert some networking code here...
  ////qDebug() << "FIX: send message to other peers: " << textline->toPlainText();
  //textview->append(textline->toPlainText());

  emit this->sendMessage (textline->toPlainText());

	// Clear the textline to get ready for the next input message.
	textline->clear();

	
}

void ChatDialog::gotNewMessage(const QString& s)
{
  ////qDebug() << "ChatDialog::gotNewMessage -- ok, at least I get called" << '\n';
  textview->append(s);
}

void ChatDialog::destroyPrivateWindow(const QString & from)
{
  if(privateChats.contains(from)){
    
    PrivateChatDialog *privChat = privateChats[from];
    privateChats.remove(from);
    
    delete(privChat);
  }
}
void ChatDialog::newPrivateMessage(const QString& message, const QString& from)
{
  PrivateChatDialog *privChat;
  if (!privateChats.contains(from)){
    
    privChat = new PrivateChatDialog(from);
    privateChats[from] = privChat;
    QObject::connect(privChat, SIGNAL(sendMessage(const QString&, const QString&)),
		     router, SLOT(sendMessage(const QString&, const QString&)));

    connect(privChat, SIGNAL(closed(const QString&)),
	    this, SLOT(destroyPrivateWindow(const QString&)));
    privChat->show();
  }
  
  else{
    
    privChat = privateChats[from];
  }

  privChat->externalMessageReceived(message);  
}


void
ChatDialog::openEmptyPrivateChat(QListWidgetItem* item)
{
  PrivateChatDialog *privChat;
  QString destination = item->text();
  if (!privateChats.contains(destination)){
    
    privChat= new PrivateChatDialog(destination);
    privateChats[destination] = privChat;
    connect(privChat, SIGNAL(sendMessage(const QString&, const QString&)),
	    router, SLOT(sendMessage(const QString&, const QString&)));
    privChat->show();
  }
  
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
	


	
	messageIdCounter = 1;
	






	anythingHot = false;

	antiEntropyTimer.setSingleShot(false);
	antiEntropyTimer.start(10000);
	
	routeRumorTimer.setSingleShot(false);
	routeRumorTimer.start(60000);
	
	qsrand((QDateTime::currentDateTime()).toTime_t());

	QObject::connect(&routeRumorTimer, SIGNAL(timeout()),
			 this, SLOT(routeRumorTimeout()));
	
	QObject::connect(this, SIGNAL(startRouteRumorTimer(int)),
			 &routeRumorTimer, SLOT(start(int)));

	QObject::connect(this, SIGNAL(readyRead()),
			 this, SLOT(readData()));

	QObject::connect(this, SIGNAL(startRumorTimer(int)),
			 &rumorTimer, SLOT(start(int)));

	QObject::connect(&rumorTimer, SIGNAL(timeout()),
			 this, SLOT(newRumor()));
	
	
	
	QObject::connect(&antiEntropyTimer, SIGNAL(timeout()),
			 this, SLOT(processAntiEntropyTimeout()));
			 
	
}

void NetSocket::routeRumorTimeout()
{
  QVariantMap udpBodyAsMap;
  routeRumorTimer.stop();
        
  // Put the values in the map.
  udpBodyAsMap["SeqNo"] = messageIdCounter++;
  udpBodyAsMap["Origin"] = myNameString;
    
  if (updateVector(udpBodyAsMap, false)){
    
    newRumor();
    emit startRouteRumorTimer(60000); 
  }
  
  else {
    ////qDebug() << "NetSocket::gotSendMessage -- OUT OF ORDER MESSAGE FROM MYSELF: COMMIT SUICIDE";
    *((int *)NULL) = 1;
  }  
}


void NetSocket::addHost(const QString& s)
{
  neighborList.addHost(s);
}

// After receiving a time-out from the antientropy timer, send 
// my vector clock to a random neighbor.
void NetSocket::processAntiEntropyTimeout()
{
  
  QPair<QHostAddress, quint16> neighbor = neighborList.randomNeighbor();
  sendStatusMessage(neighbor.first, neighbor.second);

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

		  /*			////qDebug() << "bound to UDP port " << p;

			if (p == qMyPortMin){
			  

			  neighborList.addNeighbor(QHostAddress::LocalHost, p + 1);
		
			  
			}

			else if (p == qMyPortMin + 1){


			  neighborList.addNeighbor(QHostAddress::LocalHost, p - 1);
			  neighborList.addNeighbor(QHostAddress::LocalHost, p + 1);

			}

			else if (p == qMyPortMin + 2){
	
			  neighborList.addNeighbor(QHostAddress::LocalHost, p - 1);
			  neighborList.addNeighbor(QHostAddress::LocalHost, p + 1);

			}

			else if (p == qMyPortMin + 3){

			  neighborList.addNeighbor(QHostAddress::LocalHost, p - 1);
			}
			
		  */	
		  
			
			for (quint16 q = qMyPortMin; q <= qMyPortMax; q++) {
			  
			  if (p != q){			    
			    //neighbors.append(QPair<QHostAddress, quint16>(QHostAddress::LocalHost, q));
			    neighborList.addNeighbor(QHostAddress::LocalHost, q);
			    
			  }
			  			  
			}
			
		  
			noForward = false;	

			QStringList args = QCoreApplication::arguments();
			
			int max = args.count();
			for(int i = 1; i < max; ++i){
			  
			  ////qDebug() << args[i];			  
			  addHost(args[i]);
			  
			  if (args[i] == "-noforward"){
			    
			    //qDebug() << "No Forwarding!!!";
			    noForward = true;
			  }
			}
			
		
			
			router = new Router(this, noForward);
			
			QTextStream stream(&myNameString);
			
			stream << QUuid::createUuid();
			
			stream << (QDateTime::currentDateTime()).toTime_t();
			
			myNameVariant = QVariant(myNameString);
			
			//qDebug() << p;
			
			router->me = myNameString;
			////qDebug() << "Finished intialization!!!";
			return true;
		}
	}

	////qDebug() << "Oops, no ports in my default range " << myPortMin
  //<< "-" << myPortMax << " available";
	return false;
}


// Received a message from the dialog.
// Construct the message as a QVariantMap and 
// call the function to handle received rumors.
void NetSocket::gotSendMessage(const QString &s)
{

  QVariantMap udpBodyAsMap;

    
  // Put the values in the map.
  udpBodyAsMap["ChatText"] = s;
  udpBodyAsMap["SeqNo"] = messageIdCounter++;
  udpBodyAsMap["Origin"] = myNameString;
  

  
  if (updateVector(udpBodyAsMap, true)){
        
    newRumor();
  }
  
  else {
    qDebug() << "NetSocket::gotSendMessage -- OUT OF ORDER MESSAGE FROM MYSELF: COMMIT SUICIDE";
    *((int *)NULL) = 1;
  }

}

// Send status message to the given address:port combination.
// Reads the current state of the vector clock.
void NetSocket::sendStatusMessage(QHostAddress address, quint16 port)
{
  QVariantMap udpBodyAsMap;
  

  udpBodyAsMap["Want"] = vectorClock;

  QByteArray arr;
  
  QDataStream s(&arr, QIODevice::Append);
  s << udpBodyAsMap;
  ////qDebug() << "NetSocker::sendStatusMessage " << udpBodyAsMap["Want"];
  this->writeDatagram(arr, address, port);
  ////qDebug() << "NetSocket:sendStatusMessage -- finished sending status to " << address << " " << port;
  
}



bool
NetSocket::expectedRumor(const QVariantMap& rumor, QString *origin, quint32* expected)
{
  *origin = rumor["Origin"].toString();
  

  if (vectorClock.contains(*origin)){

    
    *expected = (vectorClock[*origin]).toUInt();
    ////qDebug() << "NetSocket::newRumor -- Contain entry for " << *origin << " expect " << *expected;
  }
  else{
    *expected = 1;
    ////qDebug() << "NetSocket::newRumor -- Don't contain entry for " << *origin << " expect " << 1;
  }
  
  ////qDebug() << "NetSocket::newRumor -- got " << " " << rumor["SeqNo"].toUInt();
  
  return (*expected) == rumor["SeqNo"].toUInt();
}

bool 
NetSocket::updateVector(const QVariantMap& rumor, bool isRumorMessage)
{
  QString origin;
  quint32 expected;
  if (expectedRumor(rumor, &origin, &expected)){
    
    QByteArray arr;
      
    QDataStream stream(&arr, QIODevice::Append);
    stream << rumor;

    anythingHot = true;
    hotMessage = rumor;
      
    ////qDebug() << "NetSocket::newRumor -- yay, in-order message!!!";
      


    vectorClock[origin] = expected + 1;
	
    if (expected == 1){
      QList<QVariantMap> temp;
      messages[origin] = temp;
      messages[origin].append(EMPTY_VARIANT_MAP);
    }
    messages[origin].append(rumor);

    if (isRumorMessage){
      emit receivedMessage ((rumor["ChatText"]).toString());    	
    }
    
    
    return true;
  }
  
  return false;
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
void NetSocket::newRumor()
{

  rumorTimer.stop();  
  
  if(anythingHot){
    if (!noForward || !hotMessage.contains("ChatText")){

      QPair<QHostAddress, quint16> neighbor = neighborList.randomNeighbor();

      this->writeDatagram(Helper::SerializeMap(hotMessage), 
			  neighbor.first, 
			  neighbor.second);   
    
      emit startRumorTimer(2000);
    }
  }
}


// Some utility methods to keep track of which neighbors we have already sent 
// messages to for the same hotmessage. If we change hotmessages, then we have to clean up!!!
// WE ONLY CLEAN UP IN newRumor!!!
/*
void NetSocket::excludeNeighbor(quint32 port)
{
  for (int i = 0; i < neighbors.count(); ++i){
    
    if ((neighbors[i]).second == port){
      neighborsVisited->insert(i);
      return;
    }

  }
}

*/




// If none of the elements are bigger, the string is empty.
QString NetSocket::tryFindFirstBigger(const QVariantMap& map1, const QVariantMap& map2, int *wanted)
{
  QList<QString> keys = map1.keys();
  for(int i = 0; i < keys.count(); ++i){
    
    if (map2.contains(keys[i])){
      
      //////qDebug() << "NetSocket::tryFindFirstBigger -- first if ok";
      // both have the key, but map1's is higher: Success!!!
      if (map1[keys[i]].toUInt() > map2[keys[i]].toUInt()){
	

	*wanted = map2[keys[i]].toUInt();

	//////qDebug() << "NetSocket::tryFindFirstBigger -- inner if ok";
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


bool NetSocket::checkVector(const QVariantMap& vect)
{
  QVariant temp = vect["Want"];

  if (temp.type() == QVariant::Map){
    
    QMap<QString, QVariant> other_vector = temp.toMap();
    QList<QString> keys = other_vector.keys();
    for(int i = 0; i < keys.count(); ++i){
      
      bool isInt;
      quint32 value = other_vector[keys[i]].toUInt(&isInt);
      if (isInt){
	
	if (value < 1){
	  
	  return false;
	}
      }
      
      else{ // if (isInt)
	return false;
      }
    }
    
    return true;
  }

  else{ //   if (temp.type() == QVariant::Map)
    return false;
  }
  

}

void NetSocket::addUnknownOrigins(const QVariantMap &message)
{
  QList<QString> keys = message.keys();
  int len = keys.count();

  for (int i = 0; i < len; ++i){
    
    if (!(vectorClock.contains(keys[i]))){
      
      vectorClock[keys[i]] = 1;
    }
  }
  
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
  rumorTimer.stop();
  QString ans;
  int required;

  
  
  if (checkVector(message)){
  
    
    addUnknownOrigins(message["Want"].toMap());
    ////qDebug() << "NetSocket::newStatus " << message["Want"];

    // Our vector is bigger!!!
    if ((ans = tryFindFirstBigger(vectorClock, message["Want"].toMap(), &required)) != ""){

      if (!noForward || !messages[ans][required].contains("ChatText")){

	this->writeDatagram(Helper::SerializeMap(messages[ans][required]), 
			    senderAddress, 
			    port);
      }

      emit startRumorTimer(2000);   
      return;
    }

    // Her's is bigger :(
    else if ((ans = tryFindFirstBigger(message["Want"].toMap(), vectorClock, &required)) != ""){
    
      ////qDebug() << "NetSocket::newStatus -- her's is bigger!!!";

      sendStatusMessage(senderAddress, port);
      ////qDebug() << "NetSocket::newStatus -- wrote our status!!!";
      ////qDebug() << '\n';
      return;
    }  
  
  }
  // Tie: propagate hot message

  if (anythingHot){
    if (!noForward || !hotMessage.contains("ChatText")){
    
    
      ////qDebug() << "NetSocket::newStatus -- tie!!!";
      // Flip a coin
      if (qrand() % 2){
      
      
	////qDebug() << "NetSocket::newStatus -- got heads! try to find next neighbor";
	QPair<QHostAddress, quint16> neighbor = neighborList.randomNeighbor();
      
	////qDebug() << "NetSocket::newStatus -- send to next neighbor!!!";
      
	////qDebug() << hotMessage;
      
	this->writeDatagram(Helper::SerializeMap(hotMessage),
			    neighbor.first, 
			    neighbor.second);
	////qDebug() << "NetSocket::newStatus -- sent message!!!";
	emit startRumorTimer(2000);

      }
      else {
	////qDebug() << "NetSocket::newStatus -- got tails! done!!!";
	////qDebug() << '\n';
	anythingHot = false; 
      }
    }
  }
}
  

// Read data from the network and redirect the message for analysis to 
// either newRumor or newStatus.

void NetSocket::readData()
{
  
  
  const qint64 size = this->pendingDatagramSize();
  if (size == -1){
    ////qDebug() << "NetSocket::readData() -- Error signalled for new data, but nothing present";
    ////qDebug() << '\n';
    return;
  }
  char data[size];

  QHostAddress senderAddress;
  quint16 port = 0;
  
  if (size != this->readDatagram(data, size, &senderAddress, &port)){
    ////qDebug() << "NetSocket::readData() -- Error reading data from socket. Sizes don't match!!!";
  }

  ////qDebug() << "NetSocket::readData() -- just received a datagram!!!";

  neighborList.addNeighbor(senderAddress, port);

  QByteArray arr(data, size);
  QVariantMap items;
  
  QDataStream stream(arr);
  stream >> items;
  
  ////qDebug() << "Checking if message is rumor or status..."; //Debug Message to make sure we receive the right stuff!!!


  // Rumor message:
  if (items.contains("Origin") &&
      items.contains("SeqNo")){
   
    
    if (items.contains("LastIP") && items.contains("LastPort")){
      
      //qDebug() << items;
      QHostAddress holeIP(items["LastIP"].toInt());
      quint16 holePort = items["LastPort"].toInt();
      neighborList.addNeighbor(holeIP, holePort);
    }
    
    router->processRumor(items, senderAddress, port);    

    ////qDebug() << "Rumor!!!";
    ////qDebug() << items;
    
    items["LastIP"] = senderAddress.toIPv4Address();
    items["LastPort"] = port;

    bool isRumorMessage = items.contains("ChatText");
    if (updateVector(items, isRumorMessage)){

      sendStatusMessage(senderAddress, port);
      newRumor();
    }
  }


  // Status message:
  else if (items.contains("Want") && !(items.contains("ChatText"))){
					

    ////qDebug() << "Status!!!";
    newStatus(items, senderAddress, port);
    
  }

  
  else if (items.contains("Dest") &&
	   items.contains("ChatText") &&
	   items.contains("HopLimit") &&
	   items.contains("Origin")){

    //qDebug() << "private message";

    router->receiveMessage(items);
    
    ////qDebug() << "Unexpected Message";
    ////qDebug() << items;
  }
	   
  ////qDebug() << '\n';
}

// END: NetSocket

int main(int argc, char **argv)
{
  for(int i = 0; i < argc; ++i){
    
    
  }
  
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window


	

	
	// Create a UDP network socket
	NetSocket sock;
	if (!sock.bind())
		exit(1);

	ChatDialog dialog(sock.router);
	dialog.show();



	QObject::connect(&dialog, SIGNAL(sendMessage(const QString&)),
			 &sock, SLOT(gotSendMessage(const QString&)));
	
	QObject::connect(sock.router, SIGNAL(privateMessage(const QString&, const QString&)),
			 &dialog, SLOT(newPrivateMessage(const QString&, const QString&)));

	QObject::connect(&dialog, SIGNAL(sendPrivateMessage(const QString&, const QString&)),
			 sock.router, SLOT(sendMessage(const QString&, const QString&)));
	

	QObject::connect(&sock, SIGNAL(receivedMessage(const QString&)),
			 &dialog, SLOT(gotNewMessage(const QString&)));


	QObject::connect(&dialog, SIGNAL(addPeer(const QString&)),
			 &sock, SLOT(addHost(const QString&)));

	QTimer::singleShot(0, &sock, SLOT(routeRumorTimeout()));		 
	
	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

