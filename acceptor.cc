#include <paxos.hh>

/*
  msg["Round"] = round;
  msg["Proposal"] = proposal;
  msg["Phase1"] = 0;
msg["Sender"]

 Acceptor messages: "Proposal"                => propsal number
                     "Round"                   => round number
		     "Accepted"                => If contains key, then has accepted 
		     "Value"                   => value accepted
*/


void
Acceptor::tryPromise(QVariantMap msg)
{

  quint32 round = msg["Round"].toUInt();
  ProposalNumber p = msg["Proposal"].value<ProposalNumber>();
  QString origin = msg["Origin"].toString();
  QVariant reply

  if (commits.contains(round)){
    
    // Reply negatively
  }
  
  else if (p > maxPromise[round]){
    
    maxPromise.insert(round, p);
    // Reply with the appropriate values
  }
  
  emit singleReceiver(reply, origin);
}

void
Acceptor::tryAccept(QVariantMap msg)
{
  quint32 round = msg["Round"].toUInt();
  QString proposal = msg["Proposal"].toString();
  
  if (proposal >= maxPromise[round]){
    accepts.insert(round, proposal);

    
  }
}

