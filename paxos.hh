#ifndef PAXOS_HH
#define PAXOS_HH

#include <QObject>
#include <QMap>
#include <QVariant>
#include <QString>
#include <QVariantMap>

class ProposalNumber : public QObject{

public:
  ProposalNumber();
  
  ProposalNumber(quint64 number, QString name);
  
  ProposalNumber(const &ProposalNumber other);
  
  ~ProposalNumber();
  
  friend bool
  operator> (const ProposalNumber & first, const ProposalNumber & second);
  
  friend bool
  operator>= (const ProposalNumber& first, const ProposalNumber &second);

  friend bool
  operator== (const ProposalNumber& first, const ProposalNumber &second);

  friend bool
  operator< (const ProposalNumber& first, const ProposalNumber &second);

  friend bool
  operator<= (const ProposalNumber& first, const ProposalNumber &second);

  static void incr(ProposalNumber & p);

  quint64 number;
  QString name;
  
};

Q_DECLARE_METATYPE(ProposalNumber)

class Proposer : public QObject {

  Q_OBJECT

public:



  void
  propose(QString round, ProposalNumber proposal, QString value);

public slots:
  
  void 
  phase1 (QString value);

  void 
  phase2 (quint32 round, ProposalNumber proposal, QList<QVariantMap> responses);

private:
  
  ProposalNumber prop;

  QMap<ProposalNumber, QVariantMap> clientInfoMap;

  QMap<ProposalNumber, QString> pendingProposals;

  QMap<ProposalNumber, QSet<QString> > uniquePhase1Replies;
  QMap<ProposalNumber, QList<QVariantMap> > phase1Replies;

  QMap<ProposalNumber, QSet<QString> > uniquePhase2Replies;

  QMap<ProposalNumber, int> tombstones;

  // A well defined set of neighbors
  NeighborList neighbors;  
  QString nodeName;  

  QMap<quint32, ProposalNumber> proposalTracker;
  QMap<quint32, QVariant> valueTracker;
  
  quint32 maxRound;
};

class Acceptor : public QObject {
  
  Q_OBJECT

public slots:

  void
  tryPromise(QVariantMap msg);

  void
  tryAccept(QVariantMap msg);

private:

  QMap<quint32, ProposalNumber> maxPromise;
  QMap<quint32, QString> accepts;
  QMap<quint32, QString> commits;
};

class Learner : public QObject { 

  Q_OBJECT
  
public signals:
  
  void 
  newValue(quint32 round, QString value);

public slots:
  void
  recentlyAccepted(QVariantMap msg);

  
private:  
  void
  roundSafe(quint32 round);
  
  QMap<quint32, QMap<QString, QString> > accepts;
};

class Paxos : public QObject {
  
  Q_OBJECT
  
public slots:
  
  void
  broadcastMsg(QVariantMap msg);

  void
  newMessage(QVariantMap msg);

signals:

  void
  gotPhase1Message(QVariant msg);
  
  void
  gotPhase2Message(QVariant msg);
  
  void
  gotPromiseMessage(QVariant msg);
  
  void
  gotAcceptedMessage(QVariant msg);
  
  void
  gotMajorityPromises(quint32 round, ProposalNumber proposal, QVariantMap promises);

private:

  QList<QString> participants;
  
  QMap<quint32, QVariantMap> promises;

  QString me;
  Router router;

  Proposer proposer;
  Acceptor acceptor;
  Learner learner;  
};

#endif
