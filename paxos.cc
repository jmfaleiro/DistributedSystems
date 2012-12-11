#include <paxos.hh>

// Broadcast a message, first send it to the local node
// and then send it to the router for broadcast.

Paxos::Paxos(QList<QString> given_participants)
{
  assert(participants.count() >= 1);
  me = participants[0];

  participants = given_participants;
  
  proposer = new Proposer((given_participants.count()/2)+1);
  acceptor = new Acceptor();

  // Connect the acceptor.
  connect(this, SIGNAL(phase1Message(const QVariantMap&)),
	  acceptor, SLOT(tryPromise(const QVariantMap&)));
  connect(this, SIGNAL(phase2Message(const QVariantMap&)),
	  acceptor, SLOT(tryAccept(const QVariantMap&)));
  connect(acceptor, SIGNAL(singleReceiver(const QVariantMap &, const QString &)),
	  this, SLOT(sendSingle(const QVariantMap&, const  QString&)));
  
  // Connect the propser.
  connect(this, SIGNAL(promiseMessage(const QVariantMap&)),
	  proposer, SLOT(processPromise(const QVariantMap&)));
  connect(this, SIGNAL(acceptMessage(const QVariantMap&)),
	  proposer, SLOT(processAccept(const QVariantMap&)));
  connect(proposer, SIGNAL(broadcastMessage(const QVariantMap&)),
	  this, SLOT(broadcastMsg(const QVariantMap&)));
  
}

void
Paxos::clientRequest(const QString& value)
{
  QString requestId = newId();
  QVariantMap valueMap;

  valueMap["Id"] = requestId;
  valueMap["Value"] = value;

  pendingRequests.append(valueMap);
  if(pendingRequests.count() == 1){
    
    proposer->phase1(maxSafeRound, pendingRequests[0]);
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
  newMessage(msg);

  for(int i = 0; i < participants.count(); ++i)
    if (participants[i] != me)
      emit sendP2P(msg, participants[i]);
}

// Just received a new message from the router.
void
Paxos::newMessage(const QVariantMap& msg)
{
  PaxosCodes pc = (PaxosCodes) msg["Paxos"].toInt();

  switch(pc){
    
  case PHASE1:
    processPhase1(msg);    
    break;
  case PHASE2:
    emit phase2Message(msg);
    break;
  case COMMIT:
    commit(msg);
    break;
    
  case REJECT:
    emit rejectMessage(msg);
    break;
  case PROMISEVALUE:
    emit promiseMessage(msg);
    break;
  case PROMISENOVALUE:
    emit promiseMessage(msg);
    break;
  case ACCEPT:
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

  if (commits.contains(round)){
    
    QVariantMap response;
    PaxosCodes pc = REJECT;

    response["Paxos"] = (int)pc;
    response["Round"] = round;
    response["Value"] = commits[round];
    response["Proposal"] = msg["Proposal"];
    
    sendSingle(response, origin);
  }
  else    
    emit phase1Message(msg);  
}

void
Paxos::sendSingle(const QVariantMap& msg, const QString& destination)
{
  if (destination == me)
    newMessage(msg);
  else
    emit sendP2P(msg, destination);
}

void
Paxos::proposerTimeoutFailure()
{
  proposer->phase1(maxSafeRound, pendingRequests[0]);
}


void
Paxos::commit(QVariantMap msg)
{
  quint32 round = msg["Round"].toUInt();
  QVariantMap value = msg["Value"].toMap();
  
  commits.insert(round, value);  
}


void
Paxos::laggingRoundNumber(quint32 round, const QVariantMap& value)
{
  assert(round == maxSafeRound);
  ++maxSafeRound;
  
  QString id = value["Id"].toString();
  QString proposedId = pendingRequests[0]["Id"].toString();

  if (!commits.contains(round))
    commits.insert(round, value);  
  
  // Our value has been committed.
  if (id == proposedId)
    pendingRequests.removeFirst();
    
  if (pendingRequests.count() > 0)
    proposer->phase1(maxSafeRound, pendingRequests[0]);  
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
  return temp;
}

ProposalNumber::ProposalNumber(quint64 number, QString name)
{
  number = number;
  name = name;
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
  ProposalNumber temp;
  temp.number = second.number;
  temp.name = second.name;
  return temp;
}
