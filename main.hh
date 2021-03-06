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
#include <QHostInfo>
#include <QListWidget>
#include <QHash>
#include <QCloseEvent>
#include <QVBoxLayout>
#include <QPushButton>
#include <QAbstractButton>
#include <QFileDialog>
#include <QStringList>
#include <QTabWidget>

#include <paxos.hh>
#include <files.hh>
#include <filerequests.hh>
#include <neighbors.hh>


class Router;
class Dispatcher;



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


class DownloadBox : public QDialog
{
  Q_OBJECT

public:
  DownloadBox(const QString& query);
  ~DownloadBox();

signals:

    void
    download(const QString& fileName, const QString&destination, const QByteArray& masterBlock);
											       
    void
    close(const QString &query);


    void
    destroyRequest(const QString &query);
					
  					
  
public slots:
  
  void
  newResult(const QMap<QString, QVariant>& msg);

  void
  gotDoubleClick(QListWidgetItem *item);
  
  void
  closeEvent(QCloseEvent *e);
  
  
  
private:
  
  QVBoxLayout *layout;
  QListWidget *results;

  
  QString m_search;

};

class PaxosDialog: public QDialog
{
  Q_OBJECT

public:
  PaxosDialog(Router *r, const QList<QString>& participants);

signals:
  void
  newRequest(const QString &value);

public slots:
  void
  newValue(quint32 round, const QString &value);
  void
  gotReturnPressed();
  
private:
  
  Paxos *paxos;
  QTextEdit *valueDisplay;  
  QLineEdit *valueAdder;
};

class FileDialog : public QDialog
{
  Q_OBJECT
  
public:
  FileDialog(FileRequests *fr);

public slots:
  
  void
  newSearch();
  
  void
  closeBox(const QString &query);

  void
  newSearchResult(const QString &query, const QMap<QString, QVariant> &response);
  
  void
  fileButtonClicked();
  
  void
  filesSelected(const QStringList & files);



signals:
  
  void
  indexFiles(const QStringList& ans);					


  
  void
  destroyRequest(const QString &queryString);
  
  void
  newRequest(const QString &queryString);

private:
  
  
  QFileDialog *fileMenu;
  QPushButton *fileButton;
  FileRequests *m_fr;
  QLineEdit *text;
  QHash<QString, DownloadBox *> activeRequests;
};


class PrivateChatDialog : public QDialog
{
  Q_OBJECT
  
  public:
  PrivateChatDialog(const QString& destination);
  ~PrivateChatDialog();
				
					      
  public slots:
  
  void
  externalMessageReceived(const QString& msg);
  
  void 
  internalMessageReceived();
  

  void
  closeEvent(QCloseEvent *e);


  

signals:
 
  void
  sendMessage(const QString& msg, const QString& dest);
  
  void 
  closed(const QString & destination);

private:

  QString m_destination;
  QLineEdit *textentry;
  QTextEdit *textview;
  QVBoxLayout *layout;


};



class ChatDialog : public QDialog
{
	Q_OBJECT

public:
	ChatDialog(Router *r);

public slots:
  void gotReturnPressed();
  void gotNewMessage(const QString& s);
  void gotAddPeer();
  void addOrigin(const QString& org);
  void newPrivateMessage(const QString& message, const QString& from);
  void openEmptyPrivateChat(QListWidgetItem* item);
  void destroyPrivateWindow(const QString & from);

 


signals:
  void sendMessage (const QString& s);
  void addPeer (const QString& s);
  void sendPrivateMessage(const QString& msg, const QString& dest);
  void indexFiles(const QStringList& files);
  
private:
	QTextEdit *textview;
	TextEntryWidget *textline;
  QLineEdit *peerAdder;
  QListWidget *origins;
  
  Router *router;
  QHash<QString, PrivateChatDialog*> privateChats;
 

};


class NetSocket : public QUdpSocket
{
	Q_OBJECT

public:
	NetSocket();

	// Bind this socket to a Peerster-specific default port.
  bool bind(QList<QString> &nodes);
  
        quint32 numNeighbors();
        
        FileRequests *fileRequests;
	Router *router;
public slots:
  // This function communicates a new message from the dialog.
  void gotSendMessage(const QString &s);
  
  // This function reads data from the network. It is connected to 
  // QUdpSocket's readyRead signal.
  void readData();


  // This function processes a time-out generated by the timer
  // for antientropy.
  void processAntiEntropyTimeout();
  
  void processFiles (const QStringList& files);

  void routeRumorTimeout();

  void newRumor();
  void addHost(const QString& s);
  
  void sendNeighbor(const QMap<QString, QVariant>& msg, quint32 index);

  void broadcastMessage(const QMap<QString, QVariant>& msg);

signals:
  // This signal is connected to a display method in the dialog to 
  // display new messages received over the network.
  void receivedMessage(const QString& data);

  // This signal is starts the rumor timer while rumormongering.
  void startRumorTimer(int msec);
  
  void startRouteRumorTimer(int msec);

  void toDispatcher(const QMap<QString, QVariant>& msg);
  //void processFiles(const QStringList & files);

private:
  // We call this function when the node receives a new rumor from either the dialog or the network.
  // Only this method updates the vectorClock, messages.
  // Only this method sets anythingHot to true.


  // We call this function when we receive a new status message.
  // Both anti-entropy and rumormongering are handled. 
  // This function uses the "anythingHot" flag to rumormonger if it is true.
  // This function may set "anythingHot" to false if it stops rumormongering.
  void newStatus(const QVariantMap& message,
			    const QHostAddress& senderAddress, 
			    const quint16& port);


  // This function serlializes the current state of the vectorClock 
  // and sends to address:port.
  void sendStatusMessage(QHostAddress address, quint16 port);  


  // This function is used to compare two vectors.
  //
  // It only checks to see if map1 contains an index whose value
  // is greater than map2's corresponding index.
  //
  // If we find an index, required is set to its value.
  int tryFindFirstBigger(const QVariantMap& map1,const QVariantMap& map2, QString *required);
  

  // randomNeighbor: Finds a random neighbor "statefully".
  // 
  // Internally contains a set of neighbors already chosen. Choses only from
  // the set of those not yet chosen.
  //
  // excludeNeighbor: explicitly add a neighbor to the set of chosen ports. This
  // may be used when we want to exclude a neighbor of rumormongering.
  //
  // cleanUpVisited: empty the set and start over.
  int randomNeighbor();
  

  
  bool checkVector(const QVariantMap& vect);
  void addUnknownOrigins(const QVariantMap &message);

  bool expectedRumor(const QVariantMap& rumor, QString* origin, quint32* expected);
  bool updateVector(const QVariantMap& rumor, bool routeMessage);



  QVariantMap EMPTY_VARIANT_MAP;
  
  int myPortMin, myPortMax;
  //  QHostAddress localhost(QHostAddress::LocalHost);

  QVariant myNameVariant;
  QString myNameString;
  
  QString myIP;
  quint16 myPort;
  
  QTimer rumorTimer;
  QTimer antiEntropyTimer;
  QTimer routeRumorTimer;
  QMap<QString, QList<QVariantMap> > messages;
  
  QVariantMap hotMessage;

  bool anythingHot;
  
  NeighborList neighborList;
  
  QVariantMap vectorClock;
  quint32 messageIdCounter;
  
  bool noForward;
  
  FileStore fs;
  Dispatcher *dispatcher;
  
  
};

#endif // PEERSTER_MAIN_HH
