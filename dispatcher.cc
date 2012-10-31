#include <dispatcher.hh>
#include <main.hh>


Dispatcher::Dispatcher(FileStore *fs, NetSocket *netsocket)
{
  m_fs = fs;
  m_netsocket = netsocket;
}

void
Dispatcher::processRequest(const QMap<QString, QVariant> &request)
{

  //QUuid requestId = QUuid::createUuid();
  //pendingRequests[requestId] = request;
  QMap<QString, QVariant> ret;
  
  if (isBlockRequest(request)){
    
    QByteArray retBlock;
    QByteArray retIndex;
    QByteArray index = request["BlockRequest"].toByteArray();

    if (m_fs->ReturnBlock(index, &retBlock, &retIndex)){
      
      qDebug() << "Dispatcher:Got block request from found!";
      ret["BlockReply"] = retIndex;
      ret["Data"] = retBlock;

      emit reply(ret, request["Origin"].toString());
    }
    else {
      qDebug() << "Dispatcher: Couldn't find block!";
    }
    
  }
  else if (isSearchRequest(request)){
    
    quint32 budget = request["Budget"].toUInt();
    
    if (budget > 0){
      
      qDebug() << "Dispatcher:Got search request";
      if(m_fs->Search(request["Search"].toString(), &ret)){
	qDebug() << ret;
	qDebug() << "Dispatcher:Found match!";
	emit reply(ret, request["Origin"].toString());
      }      
      
      //distributeBudget(budget, request);
    }     
  }
}

void
Dispatcher::distributeBudget(quint32 b, const QMap<QString, QVariant> &request)
{
  QList<quint32> budgets;

  quint32 numNeighbors = m_netsocket->numNeighbors();

  for(int i = 0; i < numNeighbors; ++i)
    budgets.append(0);
  
  for(int i = 0; b > 0;--b, ++i)    
    budgets[i%numNeighbors] += 1;
  
  
  for(int i = 0; i < numNeighbors; ++i){
    
    QMap<QString, QVariant> toSend;
    toSend["Origin"] = request["Origin"].toString();
    toSend["Search"] = request["Search"].toString();
    toSend["Budget"] = budgets[i];
    emit sendNeighbor(toSend, i);
  }    
}

bool
Dispatcher::isBlockRequest(const QMap<QString, QVariant> &request)
{
  return
    request.contains("Dest") &&
    request.contains("Origin") &&
    request.contains("HopLimit") &&
    request.contains("BlockRequest");
}

bool 
Dispatcher::isSearchRequest(const QMap<QString, QVariant> & request)
{
  return 
    request.contains("Origin") &&
    request.contains("Search") &&
    request.contains("Budget");
}
