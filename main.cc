
#include <unistd.h>

#include <QVBoxLayout>
#include <QApplication>
#include <QDebug>
#include <QKeyEvent>


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

//Begin: MessageSender

MessageSender::MessageSender(){

  
}

void MessageSender::gotSendMessage (const QString &s)
{
  QVariantMap *udpBodyAsMap = new QVariantMap();
  
  QVariant *chatTextValue = new QVariant(s);
  QString *chatTextKey = new QString("ChatText");

  (*udpBodyAsMap)[*chatTextKey] = *chatTextValue;
  

  // For types of QMap/QVariantMap, QDataStream first writes the
  // number of items, followed by key, value pairs.

  QByteArray *arr = new QByteArray();
  QDataStream *serializer = new QDataStream(arr, QIODevice::Append);

  (*serializer) << (*udpBodyAsMap);

  
    
  emit MessageSender::sendMessage(*arr);
  
  udpBodyAsMap->~QMap();
  chatTextValue->~QVariant();
  chatTextKey->~QString();
  serializer->~QDataStream();
  arr->~QByteArray();

}

void MessageSender::gotReceivedMessage (const QByteArray& arr)
{
  QDataStream *stream = new QDataStream(arr);
  
  QVariantMap *items = new QVariantMap();
  
  (*stream) >> (*items);
  
  QString *temp = new QString("ChatText");
  
  qDebug() << ((*items)[*temp]).toString() << '\n'; //Debug Message to make sure we receive the right stuff!!!

  emit receivedMessage (((*items)[*temp]).toString());
}

//End: MessageSender


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

	// Lay out the widgets to appear in the main window.
	// For Qt widget and layout concepts see:
	// http://doc.qt.nokia.com/4.7-snapshot/widgets-and-layouts.html
	QVBoxLayout *layout = new QVBoxLayout();
	layout->addWidget(textview);
	layout->addWidget(textline);
	setLayout(layout);
	

	
	// Register a callback on the textline's returnPressed signal
	// so that we can send the message entered by the user.
	connect(textline, SIGNAL(returnPressed()),
		this, SLOT(gotReturnPressed()));


	
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
	
	me = new QHostAddress(QHostAddress::LocalHost);
}

bool NetSocket::bind()
{
	// Try to bind to each of the range myPortMin..myPortMax in turn.
	for (int p = myPortMin; p <= myPortMax; p++) {
		if (QUdpSocket::bind(p)) {
			qDebug() << "bound to UDP port " << p;
			return true;
		}
	}

	qDebug() << "Oops, no ports in my default range " << myPortMin
		<< "-" << myPortMax << " available";
	return false;
}

void NetSocket::gotSendMessage(const QByteArray &datagram)
{

  
  for(int p = myPortMin; p <= myPortMax; p++){
    qDebug() << "NetSocket::gotSendMessage -- Sending to " << p << '\n';
    qint64 size = writeDatagram(datagram, *me, (quint16) p);
    if (size == -1){
      qDebug() << "NetSocket::gotSendMessage -- Unable to send message to " 
	       << p << '\n';
    }
  }
}

void NetSocket::readData()
{
  const qint64 size = this->pendingDatagramSize();
  if (size == -1){
    qDebug() << "NetSocket::readData() -- Error signalled for new data, but nothing present" << '\n';
    return;
  }
  char *data  = new char[size+1];
  //QHostAddress *senderAddress = new QHostAddress();
  //quint16 port = 0;
  if (size != this->readDatagram(data, size, 0, 0)){
    qDebug() << "NetSocket::readData() -- Error reading data from socket. Sizes don't match!!!" << '\n';
  }


  // Warning: Haven't implemented checksum checking code yet!!!

  QByteArray *arr = new QByteArray(data, size);
  
  qDebug() << "NetSocket::readData() -- just received a datagram!!!" << '\n';

  emit newData(*arr);
    
}

// END: NetSocket

int main(int argc, char **argv)
{
	// Initialize Qt toolkit
	QApplication app(argc,argv);

	// Create an initial chat dialog window
	ChatDialog dialog;
	dialog.show();

	MessageSender *sender = new MessageSender();
	

	
	// Create a UDP network socket
	NetSocket sock;
	if (!sock.bind())
		exit(1);

	QObject::connect(&dialog, SIGNAL(sendMessage(const QString&)),
			 sender, SLOT(gotSendMessage(const QString&)));


	QObject::connect(sender, SIGNAL(sendMessage(const QByteArray&)),
			 &sock, SLOT(gotSendMessage(const QByteArray&)));

	QObject::connect(&sock, SIGNAL(readyRead()),
			 &sock, SLOT(readData()));

	QObject::connect(&sock, SIGNAL(newData(const QByteArray&)),
			 sender, SLOT(gotReceivedMessage(const QByteArray &)));

	QObject::connect(sender, SIGNAL(receivedMessage(const QString&)),
			 &dialog, SLOT(gotNewMessage(const QString&)));

	// Enter the Qt main loop; everything else is event driven
	return app.exec();
}

