#include <paxos.hh>
#include <QDebug>

Acceptor::Acceptor()
{

}

void
Acceptor::tryPromise(const QVariantMap& msg)
{

  quint32 round = msg["Round"].toUInt();
  ProposalNumber p = msg["Proposal"].value<ProposalNumber>();
  QString origin = msg["Origin"].toString();
  
  qDebug() << "Acceptor: try promise for round =" << round << ", proposal ="<< p.number << " " << p.name;
  
  QVariantMap reply;
  
  if (p > maximumPromise(round)){

    // Reply with the appropriate values
    ProposalNumber lastPromise = maxPromise[round];
    insertNewPromise(round, p);
    PaxosCodes pc;

    QVariant proposalVar;
    proposalVar.setValue(p);
    
    reply["Round"] = round;
    reply["Proposal"] = proposalVar;

    QPair<ProposalNumber, QVariantMap> earlierAccept = getLastAccept(round);
    
    if (earlierAccept.first.number != 0){
      
      pc = PROMISEVALUE;
      reply["Value"] = earlierAccept.second;
      reply["AcceptedProp"] = QVariant::fromValue(earlierAccept.first);
    }
    else {
      
      pc = PROMISENOVALUE;      
    }  
    
    reply["Paxos"] = pc;
  }
  
  qDebug() << "Acceptor: promised!";
  emit singleReceiver(reply, origin);
}

void
Acceptor::tryAccept(const QVariantMap& msg)
{  
  quint32 round = msg["Round"].toUInt();
  QVariantMap value = msg["Value"].toMap();
  ProposalNumber proposal = msg["Proposal"].value<ProposalNumber>();

  qDebug() << "Acceptor: try accept for round =" << round << ", proposal ="<< proposal.number << " " << proposal.name;  

  QString origin = msg["Origin"].toString();
  
  if (proposal >= maximumPromise(round)){
    
    insertAccept(round, proposal, value);

    QVariantMap reply;
    PaxosCodes pc = ACCEPT;
    
    QVariant proposalVar;
    proposalVar.setValue(proposal);
    
    reply["Paxos"] = (int)pc;
    reply["Round"] = round;
    reply["Value"] = value;
    reply["Proposal"] = proposalVar;
    
    qDebug() << "Acceptor: Accepted!";
    
    emit singleReceiver(reply, origin);
  }
}

// Functions to abstract the underlying data-layer. For now, 
// we're using in-memory data structures. 

QPair<ProposalNumber, QVariantMap>
Acceptor::getLastAccept(quint32 round)
{
  ProposalNumber p(0, "");
  QVariantMap value;
  if (acceptValues.contains(round)){
    
    assert(acceptProposals.contains(round));

    value = acceptValues[round];
    p = acceptProposals[round];
  }
  
  QPair<ProposalNumber, QVariantMap> ret(p, value);
  return ret;
}

ProposalNumber
Acceptor::maximumPromise(quint32 round)
{
  return maxPromise[round];
}

void
Acceptor::insertNewPromise(quint32 round, const ProposalNumber& p)
{
  maxPromise.insert(round, p);
}

void
Acceptor::insertAccept(quint32 round, const ProposalNumber&p, const QVariantMap& value)
{
  acceptProposals.insert(round, p);
  acceptValues.insert(round, value);
}
