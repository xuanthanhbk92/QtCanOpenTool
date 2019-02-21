
#include "CanDriverQmlWrapper.h"

#include "CanMessage.h"
#include "CanDriver.h"
#include "CanDriverLoader.h"
#include "QtCAN.h"
#include <QStringBuilder>
#include <QDebug>
#include <qqml.h>
#include "CanDriver_jsonTcp.h"
#include "candriver_serial.h"
CanDriverWrapper::CanDriverWrapper (QObject * parent)
    : QObject        (parent)
    , m_driverLoaded (false)
    , m_canDriver    (Q_NULLPTR)
{
    m_tcpJsonDriver = new CanDriverPlugin_jsonTcp();
    CanDriverPlugin_Serial * m_serial = new CanDriverPlugin_Serial();
    m_DriverMap.insert("TcpJson",m_tcpJsonDriver);
    m_DriverMap.insert("Serial",m_serial);
    m_driversList = m_DriverMap.keys();
}

CanDriverWrapper::~CanDriverWrapper (void) {
    if (m_canDriver) {
        INFO << "Stopping driver...";
        m_canDriver->stop ();
        INFO << "Stopped driver.";
    }
}

void CanDriverWrapper::registerQmlTypes (QQmlEngine * engine) {
    static const int       maj = 2;
    static const int       min = 0;
    static const char    * uri = "QtCAN.Utils"; // @uri QtCAN.Utils
    static const QString   msg = "!!!";

    qmlRegisterType<CanDriverWrapper> (uri, maj, min, "CanDriverWrapper");
    qmlRegisterType<CanDriverOption>  (uri, maj, min, "CanDriverOption");

    if (engine != Q_NULLPTR) {
        engine->addImportPath ("qrc:///imports");
    }
    else {
        qWarning () << "You didn't pass a QML engine to the register function,"
                    << "some features (mostly plain QML components) won't work !";
    }
}

bool CanDriverWrapper::getDriverLoaded (void) const {
    return m_driverLoaded;
}

QString CanDriverWrapper::getDriverName (void) const {
    return m_driverName;
}

QVariantList CanDriverWrapper::getDriverOptions (void) const {
    return m_driverOptions;
}

QStringList CanDriverWrapper::getDriversList (void) const {
    return m_driversList;
}

CanDriver * CanDriverWrapper::getDriverObject (void) const {
    return m_canDriver;
}

void CanDriverWrapper::selectDriver (const QString & name) {
    m_driverName.clear ();
    m_driverOptions.clear ();
    if (!name.isEmpty ()) {
        if (m_driversList.contains (name)) {
            m_driverName = name;
            const QList<CanDriverOption *> tmp = driverOptionRequire (m_driverName);
            m_driverOptions.clear ();
            m_driverOptions.reserve (tmp.count ());
            foreach (CanDriverOption * opt, tmp) {
                m_driverOptions.append (QVariant::fromValue (opt));
            }
        }
    }
    emit driverNameChanged ();
    emit driverOptionsChanged ();
}

void CanDriverWrapper::loadDriver (const QVariantMap & options) {
    if (m_canDriver != Q_NULLPTR) {
        m_canDriver->stop ();
        DUMP << "Driver unloaded :" << m_canDriver;
        disconnect (m_canDriver, Q_NULLPTR, this,        Q_NULLPTR);
        disconnect (this,        Q_NULLPTR, m_canDriver, Q_NULLPTR);
        m_canDriver->deleteLater ();
        m_canDriver = Q_NULLPTR;
        m_driverLoaded = false;
    }
    if (m_driversList.contains (m_driverName)) {
        DUMP << "Driver options :" << options;
        m_canDriver = loadDriver (m_driverName);
        if (m_canDriver != Q_NULLPTR) {
            connect (m_canDriver, &CanDriver::diag, this, &CanDriverWrapper::onDiag);
            connect (m_canDriver, &CanDriver::recv, this, &CanDriverWrapper::onCanMsgRecv);
            m_driverLoaded = m_canDriver->init (options);
            DUMP << "Driver loaded :" << m_canDriver;
        }
    }
    emit driverLoadedChanged ();
}

void CanDriverWrapper::sendCanMsg (CanMessage * message) {
    if (message != Q_NULLPTR &&m_canDriver != Q_NULLPTR) {
        if (m_canDriver->send (message)) {
            emit canMsgSent (message);
        }
        delete message;
    }
}

void CanDriverWrapper::onCanMsgRecv (CanMessage * message) {
    if (message != Q_NULLPTR && m_canDriver != Q_NULLPTR) {
        emit canMsgRecv (message);
    }
}

void CanDriverWrapper::onDiag (int level, const QString & description) {
    switch (CanDriver::DiagnosticLevel (level)) {
        case CanDriver::Trace: {
            emit diagLogRequested ("TRACE", description);
            break;
        }
        case CanDriver::Debug: {
            emit diagLogRequested ("DEBUG", description);
            break;
        }
        case CanDriver::Information: {
            emit diagLogRequested ("INFO", description);
            break;
        }
        case CanDriver::Warning: {
            emit diagLogRequested ("WARNING", description);
            break;
        }
        case CanDriver::Error: {
            emit diagLogRequested ("ERROR", description);
            break;
        }
    }
}

QList<CanDriverOption *> CanDriverWrapper::driverOptionRequire(QString driverName)
{
    QList<CanDriverOption *> ret;
    CanDriverPlugin * driverPlugin = m_DriverMap.value (driverName, Q_NULLPTR);
    if (driverPlugin != Q_NULLPTR) {
        ret = driverPlugin->optionsRequired ();
    }
    return ret;
}

CanDriver *CanDriverWrapper::loadDriver(QString driverName)
{
    CanDriver * ret = Q_NULLPTR;
    CanDriverPlugin * driverPlugin = m_DriverMap.value (driverName, Q_NULLPTR);
    if (driverPlugin != Q_NULLPTR) {
        ret = driverPlugin->createDriverInstance (this);
    }
    return ret;
}
