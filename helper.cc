#include "helper.hh"


QByteArray Helper::SerializeMap(const QVariantMap& m)
{
  QByteArray arr;
  
  QDataStream s(&arr, QIODevice::Append);
  s << m;

  return arr;
}
