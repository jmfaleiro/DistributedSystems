#include <paxos.hh>

Acceptor::Acceptor()
{

}

void
Acceptor::tryPromise(const QVariantMap& msg)
{

  quint32 round = msg["Round"].toUInt();
  ProposalNumber p = msg["Proposal"].value<ProposalNumber>();
  QString origin = msg["Origin"].toString();
  
  QVariantMap reply;
  
  if (p > maxPromise[round]){

    // Reply with the appropriate values
    ProposalNumber lastPromise = maxPromise[round];
    maxPromise.insert(round, p);
    PaxosCodes pc;
    reply["Round"] = round;
    reply["Proposal"] = QVariant::fromValue(p);

    if (acceptValues.contains(round)){
      
      pc = PROMISEVALUE;
      
      reply["Value"] = acceptValues[round];
      reply["AcceptedProp"] = QVariant::fromValue(acceptProposals[round]);
    }
    
    else {
      
      pc = PROMISENOVALUE;      
    }  
    
    reply["Paxos"] = pc;
  }
  
  emit singleReceiver(reply, origin);
}

void
Acceptor::tryAccept(const QVariantMap& msg)
{
  quint32 round = msg["Round"].toUInt();
  QVariantMap value = msg["Value"].toMap();
  ProposalNumber proposal = msg["Proposal"].value<ProposalNumber>();

  QString origin = msg["Origin"].toString();
  
  if (proposal >= maxPromise[round]){
    
    acceptProposals.insert(round, proposal);
    acceptValues.insert(round, value);

    QVariantMap reply;
    PaxosCodes pc = ACCEPT;
    
    reply["Paxos"] = (int)pc;
    reply["Round"] = round;
    reply["Value"] = value;
    reply["Proposal"] = QVariant::fromValue(proposal);
    
    emit singleReceiver(reply, origin);
  }
}

