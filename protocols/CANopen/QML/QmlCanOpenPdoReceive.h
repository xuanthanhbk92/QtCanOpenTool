#ifndef QMLCANOPENPDORECEIVE_H
#define QMLCANOPENPDORECEIVE_H

#include <QObject>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"
#include "QmlCanOpenPdoMappedVar.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenPdoReceive : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int pdoNum        READ getPdoNum        WRITE setPdoNum        NOTIFY pdoNumChanged)
    Q_PROPERTY (int cobId         READ getCobId         WRITE setCobId         NOTIFY cobIdChanged)
    Q_PROPERTY (int receptionType READ getReceptionType WRITE setReceptionType NOTIFY receptionTypeChanged)
    Q_PROPERTY (QQmlListProperty<QmlCanOpenPdoMappedVar> mappedVars READ getMappedVars)
    Q_CLASSINFO ("DefaultProperty", "mappedVars")

public:
    explicit QmlCanOpenPdoReceive (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getPdoNum        (void) const;
    int getCobId         (void) const;
    int getReceptionType (void) const;

    QQmlListProperty<QmlCanOpenPdoMappedVar> getMappedVars (void);

public slots:
    void setPdoNum        (const int pdoNum);
    void setCobId         (const int cobId);
    void setReceptionType (const int receptionType);

signals:
    void pdoNumChanged        (void);
    void cobIdChanged         (void);
    void receptionTypeChanged (void);

private:
    CanOpenCounter m_pdoNum;
    CanOpenCobId m_cobId;
    quint8 m_receptionType;
    QmlListWrapper<QmlCanOpenPdoMappedVar> m_mappedVars;
};

#endif // QMLCANOPENPDORECEIVE_H
