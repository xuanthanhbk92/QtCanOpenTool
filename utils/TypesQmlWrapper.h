#ifndef TYPESQMLWRAPPER_H
#define TYPESQMLWRAPPER_H

#include <QObject>
#include <QtGlobal>

#include "QQmlEnumClassHelper.h"

// NOTE : at the moment the QML engine has no way to support 64 bit ints

#define NO_64_BIT_IN_QML

#ifdef NO_64_BIT_IN_QML
#   define QmlBiggestInt qint32
#else
#   define QmlBiggestInt qint64
#endif

#define QTCAN_UTILS_EXPORT
class QTCAN_UTILS_EXPORT VarTypes {
    Q_GADGET

public:
    enum Enum {
        Unknown = -1,
        Int8,
        UInt8,
        Int16,
        UInt16,
        Int32,
        UInt32,
#ifndef NO_64_BIT_IN_QML
        Int64,
        UInt64,
#endif
    };
    QML_EXPORT_ENUM (Enum)
};
typedef VarTypes::Enum VarType;

#endif // TYPESQMLWRAPPER_H
