#ifndef CANOPENPROTOCOLMANAGER_H
#define CANOPENPROTOCOLMANAGER_H

#include <QObject>
#include <QQueue>
#include <QStringBuilder>
#include <QMetaEnum>
#include <QHash>
#include <QTimer>

#include "QtCAN.h"

#include "CanOpenDefs.h"

class QTCAN_CANOPEN_EXPORT CanOpenProtocolManager : public QObject {
    Q_OBJECT

public:
    explicit CanOpenProtocolManager (const bool autoRemoveMsg = true, const bool hooksForDebug = false, QObject * parent = Q_NULLPTR);

    static const bool           BLOCK_CRC_SUPPORT   = true;
    static const CanOpenCounter DEFAULT_BLKSIZE     = 20;
    static const CanOpenCounter MAX_SDO_BUFFER_SIZE = (1024 * 1024); // 1 MiB

    int getSdoTimeout (void) const;

    CanOpenObjDict * getObjDict (void) const;

    CanOpenNodeId getLocalNodeId (void) const;

    CanOpenHeartBeatState getLocalNodeState (void) const;

public slots:
    void start (CanDriver * driver);
    void stop  (void);

    bool isRemoteNodeAlive (const CanOpenNodeId nodeId) const;
    CanOpenHeartBeatState getRemoteNodeState (const CanOpenNodeId nodeId) const;

    void reloadPdoAndSdoConfig (void);

    void forceTransmitPdo (const int pdoNum);

    void setLocalNodeId (const CanOpenNodeId nodeId);

    void setLocalNetworkPosition (const CanOpenNetPosition netPos);

    void setSdoTimeout (const int ms);

    void sendSdoWriteRequest (const CanOpenNodeId nodeId,
                              const CanOpenIndex idx,
                              const CanOpenSubIndex subIdx,
                              const void * srcPtr,
                              const CanOpenDataLen srcLen,
                              const bool useBlockTransfer = false);

    void sendSdoReadRequest (const CanOpenNodeId nodeId,
                             const CanOpenIndex idx,
                             const CanOpenSubIndex subIdx,
                             const bool useBlockTransfer = false);

    void sendNmtChangeRequest (const CanOpenNodeId nodeId, const CanOpenNmtCmdSpecif nmtCommand);

    void sendLssCommand (const CanOpenLssCmd lssCommand, const quint8 argument = 0);

    void broadcastSync      (void);
    void broadcastHeartBeat (void);

signals:
    void started (void);
    void stopped (void);

    void localNodeStateChanged  (const CanOpenHeartBeatState state);
    void remoteNodeStateChanged (const CanOpenNodeId nodeId, const CanOpenHeartBeatState state);
    void remoteNodeAliveChanged (const CanOpenNodeId nodeId, const bool alive);

    void configuredPDOs (const QHash<CanOpenCobId, CanOpenPdoRole> & pdosList);
    void configuredSDOs (const QHash<CanOpenNodeId, CanOpenSdoMode> & sdosList);

    void recvRawMsg (const CanOpenCobId cobId, const QByteArray & data);
    void sentRawMsg (const CanOpenCobId cobId, const QByteArray & data);

    void beforeProcessTransfer (CanOpenSdoTransfer * transfer);
    void afterProcessTransfer  (CanOpenSdoTransfer * transfer);

    void failedToSendSdo  (const QString & reason);

    void recvSdoWriteReply (const CanOpenNodeId nodeId,
                            const CanOpenIndex idx,
                            const CanOpenSubIndex subIdx,
                            const CanOpenSdoAbortCode statusCode,
                            const QByteArray & buffer);

    void recvSdoReadReply  (const CanOpenNodeId nodeId,
                            const CanOpenIndex idx,
                            const CanOpenSubIndex subIdx,
                            const CanOpenSdoAbortCode statusCode,
                            const QByteArray & buffer);

    void recvSdoWriteRequest (const CanOpenNodeId nodeId,
                              const CanOpenIndex idx,
                              const CanOpenSubIndex subIdx,
                              const CanOpenSdoAbortCode statusCode,
                              const QByteArray & buffer);

    void recvSdoReadRequest  (const CanOpenNodeId nodeId,
                              const CanOpenIndex idx,
                              const CanOpenSubIndex subIdx,
                              const CanOpenSdoAbortCode statusCode,
                              const QByteArray & buffer);

protected slots:
    void doInitNode (void);

    void doMsgSend (CanMessage * msg);
    void onMsgRecv (CanMessage * msg);

    void onSdoTimeout (void);
    void onLssTimeout (void);

    void onSyncIntervalChanged      (void);
    void onHeartBeatIntervalChanged (void);
    void onHeartbeatConsumerChanged (void);

    void onDiag (const int level, const QString & description);

    void handleNMT   (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handleSYNC  (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handleHB_NG (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handleSDORX (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handleSDOTX (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handlePDO   (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);
    void handleLSS   (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr);

    void refreshSdoConfig  (void);
    void refreshPdoMapping (void);

    void enqueueSdoTransfer (CanOpenSdoTransfer * transfer);
    void processSdoTransfer (const CanOpenNodeId nodeId);

    void enqueueLssAction (CanOpenLssAction * action);
    void processLssAction (void);

protected:
    void timerEvent (QTimerEvent * event) Q_DECL_FINAL;

private:
    const bool m_autoRemoveMsg;
    const bool m_hooksForDebug;
    bool m_active;
    bool m_nodeGuardToggleBit;
    int m_sdoTimeout;
    CanOpenNodeId m_localNodeId;
    CanOpenHeartBeatState m_localNodeState;
    CanOpenNetPosition m_localNetworkPos;
    QTimer * m_syncTimer;
    QTimer * m_heartBeatTimer;
    CanDriver * m_driver;
    CanOpenObjDict * m_objDict;
    CanOpenLssQueue m_lssQueue;
    QHash<CanOpenCobId, CanOpenPdoConfigCache *> m_pdoConfigCaches;
    QHash<CanOpenNodeId, CanOpenSdoTransferQueue *> m_sdoTransferQueues;
    QHash<CanOpenNodeId, CanOpenHeartbeatConsumer *> m_hbConsumersByNodeId;
};

#endif // CANOPENPROTOCOLMANAGER_H
