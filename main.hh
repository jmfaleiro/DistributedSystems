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
#include <QPair>
#include <QList>
#include <QTimer>


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
  void gotSendMessage(const QString &s);
  void readData();
  void processTimeout();
  void processAntiEntropyTimeout();

signals:
  void receivedMessage(const QString& data);
  void startRumorTimer(int msec);
  


private:
  void newRumor(const QVariantMap& message,const QHostAddress& senderAddress,const quint16& port);
  void newStatus(const QVariantMap& message,
			    const QHostAddress& senderAddress, 
			    const quint16& port);

  void sendStatusMessage(QHostAddress address, quint16 port);  

  //  void serializeMap(const QVariantMap& m, QByteArray *arr);

  void deserializeMap(const QByteArray& arr, QVariantMap* m);

  QString tryFindFirstBigger(const QVariantMap& map1,const QVariantMap& map2, int *required);
  
  int randomNeighbor();
  void cleanUpVisited();
  void excludeNeighbor(quint32 port);

  
  QByteArray *EMPTY_BYTE_ARRAY;
  
  int myPortMin, myPortMax;
  QHostAddress *localhost;
  QPair<QHostAddress, quint16> *me;
  QVariant *myNameVariant;
  QString myNameString;

  QTimer *rumorTimer;
  QTimer *antiEntropyTimer;
  QMap<QString, QList<QByteArray> > *messages;
  
  QVariantMap hotMessage;
  QSet<quint32> *neighborsVisited;
  bool anythingHot;
  

  QList<QPair<QHostAddress, quint16> > *neighbors;
  QVariantMap *vectorClock;
  quint32 messageIdCounter;
  
};

#endif // PEERSTER_MAIN_HH
