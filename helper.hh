#ifndef HELPER_HH
#define HELPER_HH

#include <QVariantMap>
#include <QByteArray>
#include <QString>

class Helper 
{
public:
  static QByteArray SerializeMap(const QVariantMap& m);
};


#endif // HELPER_HH
