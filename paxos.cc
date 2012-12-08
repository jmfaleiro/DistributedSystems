#include <paxos.hh>
// Broadcast a message, first send it to the local node
// and then send it to the router for broadcast.

void
Paxos::broadcastMsg(QVariantMap msg)
{  
  // Proposer's phase 1 message
  if (msg.contains("Phase1"))
    acceptor.tryPromise(msg);
  
  // Proposer's phase 2 message
  else if (msg.contains("Phase2"))
    acceptor.tryAccept(msg);

  // Acceptor's accepted message
  else if (msg.contains("Accepted"))
    learner.recentlyAccepted(msg);  

  for(int i = 0; i < participants.count(); ++i)
    router.sendMap(msg, participants[i]);
}


// Just received a new message from the router.
void
Paxos::newMessage(QVariantMap msg)
{
  if (msg.contains("Phase1"))
    acceptor.tryPromise(msg);
    
  else if (msg.contains("Phase 2"))
    acceptor.tryAccept(msg);
    
  else if (msg.contains("Accepted"))
    learner.recentlyAccepted(msg);
  
  else if (msg.contains("Promise"))
    newPromise(msg);
}

void
Paxos::sendSingle(QVariantMap msg, QString destination)
{
  if (destination == me)
    newMessage(msg);
  else
    router.sendMap(msg, destination);
}

void
Paxos::newPromise(QVariantMap msg)
{
  quint32 round = msg["Round"].toUInt();
  QString origin = msg["Origin"].toString();
  ProposalNumber proposal = msg["Proposal"].value<ProposalNumber>();
  quint32 promise = msg["Promise"].toUInt();
  
  QVariantMap roundPromises;
  QVariantMap individualPromise;
  
  if (promises.contains(round))
    roundPromises = promises[round];

  // Make sure that we have the latest value, if any.
  if (roundPromises.contains(proposal))
    individualPromise = roundPromises[proposal];

  individualPromise.insert(origin, msg);
  
  if(individualPromise.count() >= majority){
    
    roundPromises.remove(proposal);
    QList<QVariantMap> toProposer = individualPromise.values();    
    emit gotMajorityPromises(round, proposal, toProposer);
  }
  else {
    
    roundPromises.insert(proposal, individualPromise);
    promises.insert(round, roundPromises);
  }
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

ProposalNumber::increment(ProposalNumber &p)
{
  ++(p.number);
}

ProposalNumber::ProposalNumber(quint32 number, QString name)
{
  number = number;
  name = name;
}

bool
ProposalNumber::operator< (const ProposalNumber &first, const ProposalNumber & second)
{
  return (first.number < second.number) || ((first.number == second.number) && 
						(first.name < second.name));
}

bool
ProposalNumber::operator<= (const ProposalNumber &first, const ProposalNumber &second)
{
  return (first < second) || ((first.number == second.number) &&
			       (first.name == second.name));
}

bool
ProposalNumber::operator== (const ProposalNumber &first, const ProposalNumber &second)
{
  return (!(first < second)) && (first <= second);
}

bool
ProposalNUmber::operator> (const ProposalNumber &first, const ProposalNumber &second)
{
  return !(first <= second);
}

bool 
ProposalNumber::operator>= (const ProposalNumber &first, const ProposalNumber &second)
{
  return !(first < second);
}
