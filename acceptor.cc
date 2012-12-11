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
  
  if (p > maxPromise[round]){

    // Reply with the appropriate values
    ProposalNumber lastPromise = maxPromise[round];
    maxPromise.insert(round, p);
    PaxosCodes pc;

    QVariant proposalVar;
    proposalVar.setValue(p);
    
    reply["Round"] = round;
    reply["Proposal"] = proposalVar;

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
  
  if (proposal >= maxPromise[round]){
    
    acceptProposals.insert(round, proposal);
    acceptValues.insert(round, value);

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

