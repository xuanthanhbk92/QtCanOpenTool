
#include "CanDriverLoader.h"
#include "CanDriver.h"

#include <QDir>
#include <QStringBuilder>
#include <QPluginLoader>
#include <QCoreApplication>

bool CanDriverLoader::useStaticDriverPlugins (void) {
#ifdef QTCAN_STATIC_DRIVERS
    return true;
#else
    return false;
#endif
}

QStringList CanDriverLoader::driversAvailable (void) {
    return getDriversHash ().keys ();
}

QList<CanDriverOption *> CanDriverLoader::driverOptionsRequired (const QString & driverName) {
    QList<CanDriverOption *> ret;
    CanDriverPlugin * driverPlugin = getDriversHash ().value (driverName, Q_NULLPTR);
    if (driverPlugin != Q_NULLPTR) {
        ret = driverPlugin->optionsRequired ();
    }
    return ret;
}

CanDriver * CanDriverLoader::loadDriver (const QString & driverName, QObject * parent) {
    CanDriver * ret = Q_NULLPTR;
    CanDriverPlugin * driverPlugin = getDriversHash ().value (driverName, Q_NULLPTR);
    if (driverPlugin != Q_NULLPTR) {
        ret = driverPlugin->createDriverInstance (parent);
    }
    return ret;
}

const QMap<QString, CanDriverPlugin *> & CanDriverLoader::getDriversHash (void) {
    static bool driversRegistered = false;
    static QMap<QString, CanDriverPlugin *> ret;
    if (!useStaticDriverPlugins ()) {
        if (!driversRegistered) {
            INFO << "Registering dynamic drivers...";
            QDir driversDir (QCoreApplication::applicationDirPath () % "/drivers");
            INFO << QCoreApplication::applicationDirPath () % "/drivers";
            if (driversDir.exists ()) {
#if defined (Q_OS_LINUX)
                static const QString pluginPattern = QStringLiteral ("*QtCAN-driver-*.so");
#elif defined (Q_OS_WIN)
                static const QString pluginPattern = QStringLiteral ("*QtCAN-driver-*.dll");
#endif
                const QFileInfoList list = driversDir.entryInfoList ((QStringList () << pluginPattern),
                                                                     (QDir::Files | QDir::NoSymLinks | QDir::NoDotAndDotDot),
                                                                     (QDir::IgnoreCase | QDir::Name));
                foreach (const QFileInfo & fileInfo, list) {
                    DUMP << "File found in driver directory :" << fileInfo.fileName ();
                    const QString driverPath = fileInfo.filePath ();
                    QPluginLoader loader (driverPath);
                    if (loader.load ()) {
                        CanDriverPlugin * driverPlugin = qobject_cast<CanDriverPlugin *> (loader.instance ());
                        if (driverPlugin != Q_NULLPTR) {
                            const QString driverName = driverPlugin->getDriverName ();
                            if (!driverName.isEmpty ()) {
                                if (!ret.contains (driverName)) {
                                    ret.insert (driverName, driverPlugin);
                                }
                            }
                        }
                    }
                    else {
                        WARN << "Can't load plugin :" << loader.errorString ();
                    }
                }
            }
            else {
                WARN << "Not driver directory found in same dir as application !";
            }
            driversRegistered = true;
            INFO << "Registering dynamic drivers : OK.";
        }
    }
    return ret;
}
