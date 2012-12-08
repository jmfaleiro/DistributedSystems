
#include <paxos.hh>

void
Proposer::phase1(quint32 round, QString value, QString client)
{  
  ProposalNumber::increment(prop);
  
  QVariantMap msg;
  msg["Paxos"] = 0;
  msg["Receiver"] = "Acceptor";
  msg["Round"] = round;
  msg["Phase1"] = 0;
  msg["ProposalNumber"] = prop; 

  QVariantMap clientInfo;
  clientInfo["Client"] = client;
  clientInfo["Round"] = round;
  clientInfo["Value"] = value;
  
  clientInfoMap[prop] = clientInfo;

  broadcastMsg(msg);
}

void
Proposer::respondClient(bool good)
{
  QVariantMap reply;
  
  QVariantMap clientInfo = clientInfoMap[prop];
  
  reply["Paxos"] = 0;
  reply["Round"] = response["Round"].toUInt();
  reply["Value"] = clientInfo["Value"].toString();
  reply["Done"] = false;
  
  emit singleReceiver(reply, clientInfo["Client"].toString());
}

void
Proposer::killProposal(ProposalNumber p)
{
  
}

void
Proposer::processFailed(QVariantMap response)
{
  ProposalNumber p = response["Proposal"].value<ProposalNumber>();
  killProposal(p);
  respondClient(false);  
}

void
Proposer::processPromise(QVariantMap response)
{
  ProposalNumber promised = response["Promise"].value<ProposalNumber>();
  QString origin = response["Origin"].toString();

  if (!tombstones.contains(promised) && clientInfoMap.contains(promised)){
  
      QSet<QString> replies = uniquePhase1Replies[promised];

      if (!replies.contains(origin)){    
    
	replies.insert(origin);
	phase1Replies[promised].append(response);
    
	if(replies.count() == neighbors.getAllNeighbors().count()){
      
	  // start the phase2
	  phase2(promised);
	}
      }
  }    
}

void
Proposer::processAccept(QVariantMap response)
{
  ProposalNumber proposal = response["Proposal"].value<ProposalNumber>();
  QString origin = response["Origin"].toString();

  if(!tombstones.contains(proposal) && pendingProposals.contains(proposal)){
    
    QSet<QString> replies = uniquePhase2Replies[proposal];
    if(!replies.contains(origin)){
      
      replies.insert(origin);
      if(replies.count() == neighbors.getAllNeighbors().count()){
	
	broadcastCommit(proposal);
      }
    }
  }
}

void
Proposer::broadcastCommit(ProposalNumber p)
{
  QVariantMap clientInfo = clientInfoMap[p];
  quint32 round = clientInfo["Round"].toUInt();
  QString value = pendingProposals[p];

  QVariantMap msg;
  msg["Paxos"] = 0;
  msg["Commit"] = 0;
  msg["Value"] = value;
  msg["Round"] = round;

  emit broadcastMessage(msg);
  
  killProposal(p);
}

void
Proposer::phase2(ProposalNumber p)
{
  QList<QVariantMap> responses = phase1Replies[p];
  
  QPair<ProposalNumber, QString> value = checkAccepted(responses);
  
  quint32 round = clientInfoMap[p]["Round"].toUInt();

  QString acceptValue = value.second;
  if(value.first == 0)
    acceptValue = pendingProposals[p];
    
  QVariantMap msg;
  msg["Paxos"] = 0;
  msg["Round"] = round;
  msg["Value"] = acceptValue;
  msg["Proposal"] = p;
  msg["Phase2"] = 0;
  msg["Receiver"] = "Acceptor";

  emit broadcastMessage(msg);
}    
  
QPair<ProposalNumber, QString> 
Proposer::checkAccepted(QList<QVariantMap> responses)
{
  
  ProposalNumber p;
  p.number = 0;
  p.name = "";
  QString value;

  for(i = 0; i < responses.count(); ++i){
    
    QVariantMap response = responses[i];
    
    if (response.contains("Value")){

      ProposalNumber rProposal = response["Proposal"].value<ProposalNumber>();
      if(p < rProposal){
	
	p = rProposal;
	value = response["Value"].toString();
      }
    }
  }

  QPair<ProposalNumber, QString> ret (p, value);
  return ret;
}
