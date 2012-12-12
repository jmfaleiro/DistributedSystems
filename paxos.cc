#include <paxos.hh>
#include <QDebug>

// Broadcast a message, first send it to the local node
// and then send it to the router for broadcast.

Paxos::Paxos(const QList<QString>& given_participants)
{

  retryTimer.stop();
  assert(given_participants.count() >= 1);
  me = given_participants[0];
  maxSafeRound = 1;

  participants = given_participants;
  
  QString file = "pxs-commits-log-";
  file += me;
  file += ".txt";
  
  my_log = new Log(file);
  QList<QString> old_commands =  my_log->readLog();
  replayLog(old_commands);

  proposer = new Proposer((given_participants.count()/2)+1, me);
  acceptor = new Acceptor(me);
  

  // Phase 1 Message -> Acceptor
  connect(this, SIGNAL(phase1Message(const QVariantMap&)),
	  acceptor, SLOT(tryPromise(const QVariantMap&)));

  // Phase 2 Message -> Acceptor
  connect(this, SIGNAL(phase2Message(const QVariantMap&)),
	  acceptor, SLOT(tryAccept(const QVariantMap&)));

  
  
  // PromiseMessage -> Proposer
  connect(this, SIGNAL(promiseMessage(const QVariantMap&)),
	  proposer, SLOT(processPromise(const QVariantMap&)));

  // Accept Message -> Proposer
  connect(this, SIGNAL(acceptMessage(const QVariantMap&)),
	  proposer, SLOT(processAccept(const QVariantMap&)));
  
  // Reject Message -> Proposer
  connect(this, SIGNAL(rejectMessage(const QVariantMap&)),
	  proposer, SLOT(processFailed(const QVariantMap&)));




  // Proposer -> broadcast message
  connect(proposer, SIGNAL(broadcastMessage(const QVariantMap&)),
	  this, SLOT(broadcastMsg(const QVariantMap&)));

  // Acceptor -> send p2p messages
  connect(acceptor, SIGNAL(singleReceiver(const QVariantMap &, const QString &)),
	  this, SLOT(sendSingle(const QVariantMap&, const  QString&)));




  
  // Catchup instance.
  connect(proposer, SIGNAL(catchupInstance(quint32, const QVariantMap&)),
	  this, SLOT(laggingRoundNumber(quint32, const QVariantMap&)));
  
  // Timeout failures.
  connect(proposer, SIGNAL(proposalTimeout()),
	  this, SLOT(proposerTimeoutFailure()));
  
  connect(&retryTimer, SIGNAL(timeout()),
	  this, SLOT(buzz()));
  
}

void
Paxos::clientRequest(const QString& value)
{
  QString requestId = newId();
  QVariantMap valueMap;

  valueMap["Id"] = requestId;
  valueMap["Value"] = value;

  qDebug() << "Paxos: got new request for value="<<value;
  
  pendingRequests.append(valueMap);

  assert(pendingRequests.count() > 0);
  if(pendingRequests.count() == 1){
    
    qDebug() << "Paxos: dispatching new request, with round="<<maxSafeRound;
    proposer->phase1(getSafeRound(), pendingRequests[0]);
  }
}

QString
Paxos::newId()
{
  QUuid localId = QUuid::createUuid();
  return localId.toString();
}

// Just got a request to broadcast a message.
void
Paxos::broadcastMsg(const QVariantMap& msg)
{  
  for(int i = 0; i < participants.count(); ++i)    
      emit sendP2P(msg, participants[i]);
}

// Just received a new message from the router.
void
Paxos::newMessage(const QVariantMap& msg)
{
  qDebug() << "Paxos: got new message!";
  
  PaxosCodes pc = (PaxosCodes) msg["Paxos"].toInt();

  switch(pc){
    
  case PHASE1:
    qDebug() << "Paxos: got phase1 message";
    processPhase1(msg);    
    break;
  case PHASE2:
    qDebug() << "Paxos: got phase2 message";
    emit phase2Message(msg);
    break;
  case COMMIT:
    qDebug() << "Paxos: got commit message";
    commit(msg);
    break;
    
  case REJECT:
    qDebug() << "Paxos: got reject message";
    emit rejectMessage(msg);
    break;
  case PROMISEVALUE:
    qDebug() << "Paxos: got promise message";
    emit promiseMessage(msg);
    break;
  case PROMISENOVALUE:
    qDebug() << "Paxos: got promise message with no value";
    emit promiseMessage(msg);
    break;
  case ACCEPT:
    qDebug() << "Paxos: got accept message";
    emit acceptMessage(msg);
    break;   
  default:
    break;
  }
}

void
Paxos::processPhase1(const QVariantMap&msg)
{
  quint32 round = msg["Round"].toUInt();
  QString origin = msg["Origin"].toString();
  
  QPair<bool, QVariantMap> isCommitted = checkCommitted(round);
  if (isCommitted.first){
    
    QVariantMap response;
    PaxosCodes pc = REJECT;

    response["Paxos"] = (int)pc;
    response["Round"] = round;
    response["Value"] = isCommitted.second;
    response["Proposal"] = msg["Proposal"];
    
    sendSingle(response, origin);
  }
  else    
    emit phase1Message(msg);  
}

void
Paxos::sendSingle(const QVariantMap& msg, const QString& destination)
{
  emit sendP2P(msg, destination);  
}

void
Paxos::proposerTimeoutFailure()
{
  int sleepTime = qrand() % 10;  
  sleepTime *= 1000;
  sleepTime += 1000;
  retryTimer.start(sleepTime);  
}

void
Paxos::buzz()
{
  retryTimer.stop();
  qDebug() << "Paxos: Retrying request with round="<<maxSafeRound;
  proposer->phase1(maxSafeRound, pendingRequests[0]);
}

void
Paxos::commit(QVariantMap msg)
{
  quint32 round = msg["Round"].toUInt();
  QVariantMap value = msg["Value"].toMap();
  
  storeCommit(round, value);
}


void
Paxos::laggingRoundNumber(quint32 round, const QVariantMap& value)
{
  qDebug() << "In lagging round number";
  assert(round == getSafeRound());

  incrementSafeRound();
  
  QString id = value["Id"].toString();
  QString proposedId = pendingRequests[0]["Id"].toString();

  storeCommit(round, value);
  
  // Our value has been committed.
  if (id == proposedId)
    pendingRequests.removeFirst();
    
  if (pendingRequests.count() > 0){
    qDebug() << "Paxos: dispatching new request with round="<<maxSafeRound;
    proposer->phase1(getSafeRound(), pendingRequests[0]);  
  }
}



void
Paxos::incrementSafeRound()
{  
  ++maxSafeRound;
}

quint32
Paxos::getSafeRound()
{
  return maxSafeRound;
}

void
Paxos::storeCommit(quint32 round, const QVariantMap& value)
{
  if (!commits.contains(round)){
    
    QString toLog;
    QTextStream stream(&toLog);
        
    stream << "commit" << ":" << round << ":" << value["Id"].toString() << ":" << value["Value"].toString() << ":\n";

    my_log->log(toLog);
    
    emit newValue(round, value["Value"].toString());
    commits.insert(round, value);
  }
}

void
Paxos::replayLog(const QList<QString> &log)
{
  quint32 round;
  QVariantMap value;

  bool quit = false;

  for(int i = 0; i < log.count(); ++i){
    
    QStringList split = log[i].split(":");
    
    if (split[0] == "commit" && split[4] == ""){

      round = split[1].toUInt();

      value.insert("Id", split[2]);
      value.insert("Value", split[3]);
      
      assert(!commits.contains(round));
      
      commits.insert(round, value);
      qDebug() << "Replay log: found commit#" << round << ", value=" << value;
    }
    
    else      
      assert(false);
    
    if (quit)
      break;
  }
}

QPair<bool, QVariantMap>
Paxos::checkCommitted(quint32 round)
{
  QVariantMap value;
  bool isCommitted = false;

  if (commits.contains(round)){
    
    isCommitted = true;
    value = commits[round];
  }
  
  QPair<bool, QVariantMap> ret(isCommitted, value);
  return ret;
}


ProposalNumber::ProposalNumber()
{
  number = 0;
  name = "";
}

ProposalNumber::ProposalNumber(const ProposalNumber& other)
{
  number = other.number;
  name = other.name;
}

ProposalNumber::~ProposalNumber()
{

}

ProposalNumber ProposalNumber::incr(const ProposalNumber &p)
{
  ProposalNumber temp;
  
  temp.number = p.number + 1;
  temp.name = p.name;
  qDebug()<< "incr number="<<temp.number<<" "<<temp.name;
  return temp;
}

ProposalNumber::ProposalNumber(quint64 given_number, QString given_name)
{
  number = given_number;
  name = given_name;
}

bool
ProposalNumber::operator< (const ProposalNumber & second) const
{
  return (number < second.number) || ((number == second.number) &&
				      (name < second.name));
}

bool
ProposalNumber::operator<= (const ProposalNumber &second) const
{
  return  ((number < second.number) || ((number == second.number) && (name < second.name))) || ((number == second.number) && (name == second.name));
}

bool
ProposalNumber::operator== (const ProposalNumber &second) const
{
  return ((number == second.number) && (name == second.name));
}

bool
ProposalNumber::operator> (const ProposalNumber &second) const
{
  return (number > second.number) || ((number == second.number) && (name > second.name));
}

bool 
ProposalNumber::operator>= (const ProposalNumber &second) const
{
  return  ((number > second.number) || ((number == second.number) && (name > second.name))) || ((number == second.number) && (name == second.name));
}

ProposalNumber
ProposalNumber::operator= (const ProposalNumber &second)
{
  number = second.number;
  name = second.name;
  return *this;
}

Log::Log(const QString &given_fileName)
{
  fileName = given_fileName;
}

void
Log::log(const QString& tolog)
{
  std::ofstream logFile;
  logFile.open(fileName.toStdString().c_str(), std::ios::out | std::ios::app);
  logFile << tolog.toStdString();
  logFile.close();
}


QList<QString>
Log::readLog() 
{
  QList<QString> ret;
  std::string temp;
  std::ifstream logFile;
  
  logFile.open(fileName.toStdString().c_str());

  while(std::getline(logFile, temp))    
    ret.append(QString(temp.c_str()));

  return ret;
}
