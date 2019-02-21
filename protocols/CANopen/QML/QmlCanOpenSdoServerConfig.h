#ifndef QMLCANOPENSDOSERVERCONFIG_H
#define QMLCANOPENSDOSERVERCONFIG_H

#include <QObject>

#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenSdoServerConfig : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int srvNum          READ getSrvNum          WRITE setSrvNum          NOTIFY srvNumChanged)
    Q_PROPERTY (int cobIdCliRequest READ getCobIdCliRequest WRITE setCobIdCliRequest NOTIFY cobIdCliRequestChanged)
    Q_PROPERTY (int cobIdSrvReply   READ getCobIdSrvReply   WRITE setCobIdSrvReply   NOTIFY cobIdSrvReplyChanged)

public:
    explicit QmlCanOpenSdoServerConfig (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getSrvNum          (void) const;
    int getCobIdCliRequest (void) const;
    int getCobIdSrvReply   (void) const;

public slots:
    void setSrvNum          (const int srvNum);
    void setCobIdCliRequest (const int cobIdCliRequest);
    void setCobIdSrvReply   (const int cobIdSrvReply);

signals:
    void srvNumChanged          (void);
    void cobIdCliRequestChanged (void);
    void cobIdSrvReplyChanged   (void);

private:
    CanOpenCounter m_srvNum;
    CanOpenCobId m_cobIdCliRequest;
    CanOpenCobId m_cobIdSrvReply;
};

#endif // QMLCANOPENSDOSERVERCONFIG_H
