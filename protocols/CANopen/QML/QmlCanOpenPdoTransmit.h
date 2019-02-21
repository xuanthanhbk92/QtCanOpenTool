#ifndef QMLCANOPENPDOTRANSMIT_H
#define QMLCANOPENPDOTRANSMIT_H

#include <QObject>
#include <QVector>
#include <QQmlListProperty>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"
#include "QmlCanOpenPdoMappedVar.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenPdoTransmit : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int pdoNum           READ getPdoNum           WRITE setPdoNum           NOTIFY pdoNumChanged)
    Q_PROPERTY (int cobId            READ getCobId            WRITE setCobId            NOTIFY cobIdChanged)
    Q_PROPERTY (int transmissionType READ getTransmissionType WRITE setTransmissionType NOTIFY transmissionTypeChanged)
    Q_PROPERTY (int inhibitTime      READ getInhibitTime      WRITE setInhibitTime      NOTIFY inhibitTimeChanged)
    Q_PROPERTY (int eventTimer       READ getEventTimer       WRITE setEventTimer       NOTIFY eventTimerChanged)
    Q_PROPERTY (QQmlListProperty<QmlCanOpenPdoMappedVar> mappedVars READ getMappedVars)
    Q_CLASSINFO ("DefaultProperty", "mappedVars")

public:
    explicit QmlCanOpenPdoTransmit (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getPdoNum           (void) const;
    int getCobId            (void) const;
    int getTransmissionType (void) const;
    int getInhibitTime      (void) const;
    int getEventTimer       (void) const;

    QQmlListProperty<QmlCanOpenPdoMappedVar> getMappedVars (void);

public slots:
    void setPdoNum           (const int pdoNum);
    void setCobId            (const int cobId);
    void setTransmissionType (const int transmissionType);
    void setInhibitTime      (const int inhibitTime);
    void setEventTimer       (const int eventTimer);

signals:
    void pdoNumChanged           (void);
    void cobIdChanged            (void);
    void transmissionTypeChanged (void);
    void inhibitTimeChanged      (void);
    void eventTimerChanged       (void);

private:
    CanOpenCounter m_pdoNum;
    CanOpenCobId m_cobId;
    quint8 m_transmissionType;
    quint16 m_inhibitTime;
    quint16 m_eventTimer;
    QmlListWrapper<QmlCanOpenPdoMappedVar> m_mappedVars;
};

#endif // QMLCANOPENPDOTRANSMIT_H
