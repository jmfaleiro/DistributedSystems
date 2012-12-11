
#include <paxos.hh>
#include <QDebug>

Proposer::Proposer(quint32 given_majority, QString my_name)
{
  state = IDLE;
  majority = given_majority;

  curProposal.number = 1;
  curProposal.name = my_name;  
  
  connect(&roundTimer, SIGNAL(timeout()),
	  this, SLOT(processTimeout()));
  
  curProposal = ProposalNumber::incr(curProposal);
  qDebug() << "Proposer:Initialized proposal="<<curProposal.number<<" "<<curProposal.name<<" majority="<<majority;
}

void
Proposer::phase1(quint32 round, QVariantMap value)
{  
  // Generate a new, globally unique proposal number, 
  // which doubles for a request identifier.
  curProposal = ProposalNumber::incr(curProposal);
  
  // Clean up the state from the previous try.
  uniquePhase1Replies.clear();
  uniquePhase2Replies.clear();
  phase1Replies.clear();  
  
  curValue = value;
  curRound = round;
  
  QVariantMap msg;
  PaxosCodes pc = PHASE1;
  
  QVariant proposalVar;
  proposalVar.setValue(curProposal);
  
  // Marshall the message.
  msg["Paxos"] = (int)pc;
  msg["Round"] = round;
  msg.insert("Proposal", proposalVar);

  qDebug() << "Proposer: phase1: round="<<round<<", proposal="<<curProposal.number<<" "<<curProposal.name;
  
  // Set the state for accepting messages from acceptors, 
  // broadcast the proposal and start the timeout timer.
  state = PHASE1;  
  emit broadcastMessage(msg);
  roundTimer.start(200);
  assert(roundTimer.isActive());
}

void
Proposer::processTimeout()
{
  qDebug() << "Proposer: Got timeout!";
  // If we got a timeout, it doesn't matter in which phase of Paxos 
  // we were in. We quit and signal that we didn't succeed.  
  roundTimer.stop();
  state = IDLE;
  emit proposalTimeout();
}

// This method processes a response from an acceptor which says that the
// round has already completed. We may have already processed such a message
// before, so make sure that it corresponds to the current proposal.
// It can only be processed when we are in state phase1. Avoid the case
// where we might see it in a later state. It's ok because the round will
// not succeed anyway. 
void
Proposer::processFailed(const QVariantMap& response)
{
  ProposalNumber proposal= response["Proposal"].value<ProposalNumber>();
  
  if (state == PHASE1){
    
    if (curProposal == proposal){
    
      roundTimer.stop();
      state = IDLE;
  
      quint32 round = response["Round"].toUInt();
      QVariantMap value = response["Value"].toMap();
    
      qDebug() << "Proposer: got paxos reject, sending to paxos";
      emit catchupInstance(round, value);
    }
  }
}


// Process a response to a phase1 message. We have to be in state phase1 to 
// deal with such messages. 
void
Proposer::processPromise(const QVariantMap& response)
{
  if (state == PHASE1){
    
    ProposalNumber promised = response["Proposal"].value<ProposalNumber>();

    // Check if the response is to the request we're currently processing.
    if (promised == curProposal){

      // Idempotence: Make sure we don't process a promise from the same origin more
      //              than once. 
      QString origin = response["Origin"].toString();      
      if (!uniquePhase1Replies.contains(origin)){
	
	qDebug() << "Proposer: got new promise!";
	uniquePhase1Replies.insert(origin);
	phase1Replies.append(response);
	
	// If we have a majority of responses: 
	//   1) Stop the timer.
	//   2) Stop processing more messages.
	//   3) Move to phase2.
	if(uniquePhase1Replies.count() >=  majority){
	  
	  roundTimer.stop();
	  state = PROCESSING;
	  qDebug() << "Proposer: promise count > majority="<<majority;
	  phase2();
	}
      }      
    }
  }
}

void
Proposer::processAccept(const QVariantMap& response)
{
  if(state == PHASE2){
    
    ProposalNumber proposal = response["Proposal"].value<ProposalNumber>();
    
    // Check if the response is to the request we're currently processing.
    if (curProposal == proposal){
      
      // Idempotence check: ensure that you don't count replies twice. 
      QString origin = response["Origin"].toString();
      if(!uniquePhase2Replies.contains(origin)){
	
	uniquePhase2Replies.insert(origin);
	
	// If we have enough replies, commit!
	if(uniquePhase2Replies.count() == majority){
	  
	  roundTimer.stop();
	  state = PROCESSING;
	  broadcastCommit(proposal);
	}
      }      
    }
  }
}

void
Proposer::broadcastCommit(ProposalNumber p)
{  
  QVariantMap msg;
  PaxosCodes pc = COMMIT;

  msg["Paxos"] = (int)pc;
  msg["Value"] = acceptValue;
  msg["Round"] = curRound;
  
  qDebug() << "Committing round:"<<curRound;

  emit broadcastMessage(msg);  
  emit catchupInstance(curRound, acceptValue);
}

// We reach this method when we have dealt with a majority of 
// promises from our neighbors.
void
Proposer::phase2()
{
  // Check if any of the acceptors have already accepted a value before.
  // If yes, then make sure that you don't report 
  QPair<ProposalNumber, QVariantMap> value = checkAccepted();

  // Value of the highest accepted proposal for this round.
  acceptValue = value.second;
  if(value.first.number == 0)
    acceptValue = curValue;

  QVariantMap msg;
  PaxosCodes pc = PHASE2;
  
  QVariant proposalVar;
  proposalVar.setValue(curProposal);

  msg["Paxos"] = (int)pc;
  msg["Round"] = curRound;  
  msg["Value"] = acceptValue;
  msg.insert("Proposal", proposalVar);
  // msg["Proposal"] = proposalVar;

  state = PHASE2;
  emit broadcastMessage(msg);
  roundTimer.start(200);
  assert(roundTimer.isActive());
}    
  
QPair<ProposalNumber, QVariantMap> 
Proposer::checkAccepted()
{
  int i;
  ProposalNumber p;
  p.number = 0;
  p.name = "";
  QVariantMap value;

  for(i = 0; i < phase1Replies.count(); ++i){
    
    QVariantMap response = phase1Replies[i];
    PaxosCodes msgCode = (PaxosCodes)response["Paxos"].toInt();
    
    if (msgCode == PROMISEVALUE){

      ProposalNumber rProposal = response["AcceptedProp"].value<ProposalNumber>();
      if(p < rProposal){
	
	p = rProposal;
	value = response["Value"].toMap();
      }
    }
  }

  QPair<ProposalNumber, QVariantMap> ret;
  ret.first = p;
  ret.second = value;
  return ret;
}
