#ifndef QMLCANOPENSDOCLIENTCONFIG_H
#define QMLCANOPENSDOCLIENTCONFIG_H

#include <QObject>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenSdoClientConfig : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int cliNum          READ getCliNum          WRITE setCliNum          NOTIFY cliNumChanged)
    Q_PROPERTY (int cobIdCliRequest READ getCobIdCliRequest WRITE setCobIdCliRequest NOTIFY cobIdCliRequestChanged)
    Q_PROPERTY (int cobIdSrvReply   READ getCobIdSrvReply   WRITE setCobIdSrvReply   NOTIFY cobIdSrvReplyChanged)
    Q_PROPERTY (int nodeId          READ getNodeId          WRITE setNodeId          NOTIFY nodeIdChanged)

public:
    explicit QmlCanOpenSdoClientConfig (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getCliNum          (void) const;
    int getCobIdCliRequest (void) const;
    int getCobIdSrvReply   (void) const;
    int getNodeId          (void) const;

public slots:
    void setCliNum          (const int cliNum);
    void setCobIdCliRequest (const int cobIdCliRequest);
    void setCobIdSrvReply   (const int cobIdSrvReply);
    void setNodeId          (const int nodeId);

signals:
    void cliNumChanged          (void);
    void cobIdCliRequestChanged (void);
    void cobIdSrvReplyChanged   (void);
    void nodeIdChanged          (void);

private:
    CanOpenCounter m_cliNum;
    CanOpenCobId m_cobIdCliRequest;
    CanOpenCobId m_cobIdSrvReply;
    CanOpenNodeId m_nodeId;
};

#endif // QMLCANOPENSDOCLIENTCONFIG_H
