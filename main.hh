#ifndef PEERSTER_MAIN_HH
#define PEERSTER_MAIN_HH

#include <QDialog>
#include <QTextEdit>
#include <QLineEdit>
#include <QUdpSocket>
#include <QKeyEvent>
#include <QDataStream>
#include <QByteArray>
#include <QVariant>



class MessageSender : public QObject
{

  Q_OBJECT

public: 
  MessageSender();

public slots:
  void gotSendMessage (const QString &s);
  void gotReceivedMessage (const QByteArray &arr);
  
signals:
  void sendMessage(const QByteArray &arr);
  void receivedMessage (const QString &s);

};



class TextEntryWidget : public QTextEdit
{
  Q_OBJECT



public:
  TextEntryWidget(QWidget *parent);

protected:
  void keyPressEvent ( QKeyEvent* e);

signals:
  void returnPressed();

};

class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog();

public slots:
	void gotReturnPressed();
  void gotNewMessage(const QString& s);

signals:
  void sendMessage (const QString& s);


private:
	QTextEdit *textview;
	TextEntryWidget *textline;

};

class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a Peerster-specific default port.
	bool bind();

public slots:
  void gotSendMessage(const QByteArray &datagram);
  void readData();

signals:
  void newData(const QByteArray& data);

private:
	int myPortMin, myPortMax;
  QHostAddress * me;
};


#endif // PEERSTER_MAIN_HH
