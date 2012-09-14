#ifndef NEIGHBORS_HH
#define NEIGHBORS_HH

#include <QObject>
#include <QMap>
#include <QList>
#include <QPair>
#include <QHostAddress>
#include <QHostInfo>
#include <QString>
#include <QStringList>

class NeighborList : public QObject
{
  Q_OBJECT

public:
  NeighborList();
  QPair<QHostAddress, quint16> randomNeighbor();
  void addNeighbor(const QHostAddress& addr, quint16 port);


public slots:
  void addHost(const QString& s);
  void lookedUpHost(const QHostInfo& info);
  
private:
  bool checkIfWellFormedIP(const QString& addr);
  
  QMap<QString, QList<quint16> > pendingLookups;
  QList<QPair<QHostAddress, quint16> > neighbors;
  QString myIP;
};

#endif
