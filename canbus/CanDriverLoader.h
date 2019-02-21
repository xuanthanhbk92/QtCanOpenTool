#ifndef CANDRIVERLOADER_H
#define CANDRIVERLOADER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QMap>

#include "QtCAN.h"

class QTCAN_BASE_EXPORT CanDriverLoader : public QObject {
    Q_OBJECT

public:
    static bool                     useStaticDriverPlugins (void);
    static QStringList              driversAvailable       (void);
    static QList<CanDriverOption *> driverOptionsRequired  (const QString & driverName);
    static CanDriver *              loadDriver             (const QString & driverName, QObject * parent = Q_NULLPTR);

private:
    static const QMap<QString, CanDriverPlugin *> & getDriversHash (void);
};

#endif // CANDRIVERLOADER_H
