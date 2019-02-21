
#include "AbstractObdPreparator.h"

#include "CanOpenDefs.h"
#include "QmlCanOpenDataLayer.h"
#include "QmlCanOpenVarEntry.h"
#include "QmlCanOpenArrayEntry.h"
#include "QmlCanOpenRecordEntry.h"
#include "QmlCanOpenSubEntry.h"
#include "QmlCanOpenEntryRepeater.h"
#include "QmlCanOpenSdoClientConfig.h"
#include "QmlCanOpenSdoServerConfig.h"
#include "QmlCanOpenPdoReceive.h"
#include "QmlCanOpenPdoTransmit.h"
#include "QmlCanOpenPdoMappedVar.h"
#include "QmlCanOpenValueModifier.h"

void CanOpenObdPreparator::registerCanOpenQmlTypes (void) {
    static bool registered = false;
    if (!registered) {
        const char * uri = "QtCAN.CANopen"; // @uri QtCAN.CANopen
        const int    maj = 2;
        const int    min = 0;

        qmlRegisterUncreatableType<CanOpenDataTypes>      (uri, maj, min, "DataTypes",   "!!!");
        qmlRegisterUncreatableType<CanOpenObjTypes>       (uri, maj, min, "ObjTypes",    "!!!");
        qmlRegisterUncreatableType<CanOpenAccessModes>    (uri, maj, min, "AccessModes", "!!!");

        qmlRegisterUncreatableType<AbstractObdPreparator> (uri, maj, min, "AbstractObdPreparator", "!!!");

        qmlRegisterType<QmlCanOpenPdoMappedVar>           (uri, maj, min, "PdoMappedVar");

        qmlRegisterType<QmlCanOpenSubEntry>               (uri, maj, min, "SubEntry");

        qmlRegisterType<QmlCanOpenDataLayer>              (uri, maj, min, "DataLayer");

        qmlRegisterType<QmlCanOpenVarEntry>               (uri, maj, min, "VarEntry");
        qmlRegisterType<QmlCanOpenArrayEntry>             (uri, maj, min, "ArrayEntry");
        qmlRegisterType<QmlCanOpenRecordEntry>            (uri, maj, min, "RecordEntry");

        qmlRegisterType<QmlCanOpenValueModifier>          (uri, maj, min, "ValueModifier");

        qmlRegisterType<QmlCanOpenEntryRepeater>          (uri, maj, min, "EntryRepeater");

        qmlRegisterType<QmlCanOpenSdoClientConfig>        (uri, maj, min, "SdoClient");
        qmlRegisterType<QmlCanOpenSdoServerConfig>        (uri, maj, min, "SdoServer");

        qmlRegisterType<QmlCanOpenPdoReceive>             (uri, maj, min, "PdoReceive");
        qmlRegisterType<QmlCanOpenPdoTransmit>            (uri, maj, min, "PdoTransmit");

        registered = true;
    }
}

void CanOpenObdPreparator::exposeQmlProperty (const QString & name, const QVariant & value) {
    m_qmlEngine.rootContext ()->setContextProperty (name, value);
}

bool CanOpenObdPreparator::loadFromQmlFile (const QString & url, CanOpenObjDict * obd) {
    bool ret = false;
    registerCanOpenQmlTypes ();
    QQmlComponent compo (&m_qmlEngine, QUrl (url), QQmlComponent::PreferSynchronous);
    if (compo.isReady ()) {
        if (QObject * obj = compo.create ()) {
            if (AbstractObdPreparator * gen = qobject_cast<AbstractObdPreparator *> (obj)) {
                connect (gen, &AbstractObdPreparator::diag, this, &CanOpenObdPreparator::diag);
                gen->prepareOBD (obd);
                ret = true;
            }
        }
    }
    else {
        foreach (const QQmlError & qmlError, compo.errors ()) {
            qCritical () << qmlError;
        }
    }
    return ret;
}
