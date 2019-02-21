
#include "CanOpenProtocolManager.h"

#include "CanOpenObjDict.h"
#include "CanOpenSubEntry.h"
#include "CanOpenEntry.h"
#include "CanMessage.h"
#include "CanDriver.h"

#include <QDebug>
#include <QElapsedTimer>
#include <QTimerEvent>

static quint16 computeCRC16CCITT (const char data [], const quint len) {
    static const quint16 POLY_CCITT = 0x1021;
    static const quint16 INIT_CCITT = 0x0000;
    static bool isTabInit = false;
    static quint16 crcTab [256];
    if (!isTabInit) {
        for (quint bytePos = 0; bytePos < 256; ++bytePos) {
            quint16 crc = 0;
            quint16 c   = quint16 (quint16 (bytePos) << 8);
            for (quint bitPos = 0; bitPos < 8; ++bitPos) {
                crc = quint16 (((crc ^ c) & 0x8000) ? ((crc << 1) ^ POLY_CCITT) : (crc << 1));
                c = quint16 (c << 1);
            }
            crcTab [bytePos] = crc;
        }
        isTabInit = true;
    }
    quint16 ret = INIT_CCITT;
    for (quint pos = 0u; pos < len; ++pos) {
        const char byte = data [pos];
        const quint16 base = ret;
        const quint16 shortByte = (0x00ff & quint16 (byte));
        const quint16 tmp = ((base >> 8) ^ shortByte);
        ret = quint16 ((base << 8) ^ crcTab [tmp]);
    }
    return ret;
}

CanOpenProtocolManager::CanOpenProtocolManager (const bool autoRemoveMsg, const bool hooksForDebug, QObject * parent) : QObject (parent)
  , m_autoRemoveMsg      (autoRemoveMsg)
  , m_hooksForDebug      (hooksForDebug)
  , m_active             (false)
  , m_nodeGuardToggleBit (false)
  , m_sdoTimeout         (1000)
  , m_localNodeId        (0x00)
  , m_localNodeState     (CanOpenHeartBeatStates::Initializing)
  , m_localNetworkPos    (CanOpenNetPositions::Slave)
  , m_driver             (Q_NULLPTR)
{
    m_objDict = new CanOpenObjDict (this);
    m_syncTimer = new QTimer (this);
    m_syncTimer->setTimerType (Qt::PreciseTimer);
    m_heartBeatTimer = new QTimer (this);
    m_heartBeatTimer->setTimerType (Qt::PreciseTimer);
    connect (m_syncTimer, &QTimer::timeout, this, &CanOpenProtocolManager::broadcastSync);
    connect (m_heartBeatTimer, &QTimer::timeout, this, &CanOpenProtocolManager::broadcastHeartBeat);
    connect (m_objDict->getSubEntry (0x1006, 0x00), &CanOpenSubEntry::dataChanged, this, &CanOpenProtocolManager::onSyncIntervalChanged);
    connect (m_objDict->getSubEntry (0x1017, 0x00), &CanOpenSubEntry::dataChanged, this, &CanOpenProtocolManager::onHeartBeatIntervalChanged);
    for (CanOpenSubIndex subIdx = 1; subIdx <= 127; ++subIdx) {
        if (CanOpenHeartbeatConsumer * consumer = new CanOpenHeartbeatConsumer (CanOpenNodeId (subIdx))) {
            m_hbConsumersByNodeId.insert (consumer->nodeId, consumer);
        }
        if (CanOpenSubEntry * subEntry = m_objDict->getSubEntry (0x1016, subIdx)) {
            connect (subEntry, &CanOpenSubEntry::dataChanged, this, &CanOpenProtocolManager::onHeartbeatConsumerChanged);
        }
    }
}

int CanOpenProtocolManager::getSdoTimeout (void) const {
    return m_sdoTimeout;
}

CanOpenObjDict * CanOpenProtocolManager::getObjDict (void) const {
    return m_objDict;
}

CanOpenNodeId CanOpenProtocolManager::getLocalNodeId (void) const {
    return m_localNodeId;
}

CanOpenHeartBeatState CanOpenProtocolManager::getLocalNodeState (void) const {
    return m_localNodeState;
}

void CanOpenProtocolManager::start (CanDriver * driver) {
    if (!m_active) {
        m_driver = driver;
        if (m_driver != Q_NULLPTR) {
            connect (m_driver, &CanDriver::recv, this, &CanOpenProtocolManager::onMsgRecv, Qt::UniqueConnection);
            if (m_hooksForDebug) {
                connect (m_driver, &CanDriver::diag, this, &CanOpenProtocolManager::onDiag, Qt::UniqueConnection);
            }
            m_active = true;
            doInitNode ();
            emit started ();
        }
    }
}

void CanOpenProtocolManager::stop (void) {
    if (m_active) {
        if (m_driver != Q_NULLPTR) {
            disconnect (this, Q_NULLPTR, m_driver, Q_NULLPTR);
            disconnect (m_driver, Q_NULLPTR, this, Q_NULLPTR);
            m_driver = Q_NULLPTR;
            m_active = false;
            emit stopped ();
        }
    }
}

bool CanOpenProtocolManager::isRemoteNodeAlive (const CanOpenNodeId nodeId) const {
    bool ret = false;
    if (CanOpenHeartbeatConsumer * consumer = m_hbConsumersByNodeId.value (nodeId)) {
        ret = consumer->alive;
    }
    return ret;
}

CanOpenHeartBeatState CanOpenProtocolManager::getRemoteNodeState (const CanOpenNodeId nodeId) const {
    CanOpenHeartBeatState ret = CanOpenHeartBeatStates::Stopped;
    if (CanOpenHeartbeatConsumer * consumer = m_hbConsumersByNodeId.value (nodeId)) {
        ret = consumer->state;
    }
    return ret;
}

void CanOpenProtocolManager::reloadPdoAndSdoConfig (void) {
    refreshSdoConfig ();
    refreshPdoMapping ();
}

void CanOpenProtocolManager::forceTransmitPdo (const int pdoNum) {
    if (CanOpenSubEntry * subEntry = m_objDict->getSubEntry (CanOpenIndex (CanOpenObjDictRanges::PDOTX_CONFIG_START + pdoNum -1), CanOpenPdoConfSubIndexes::CobId)) {
        const CanOpenCobId cobId = subEntry->readAs<quint32> ();
        if (CanOpenPdoConfigCache * pdoConfCache = m_pdoConfigCaches.value (cobId, Q_NULLPTR)) {
            pdoConfCache->currentFrameData.clear ();
            pdoConfCache->collectMappedVars (m_objDict);
            doMsgSend (new CanMessage (CanId (quint16 (pdoConfCache->cobId)),
                                       pdoConfCache->currentFrameData.getLength (),
                                       pdoConfCache->currentFrameData.getDataPtr ()));
        }
    }
}

void CanOpenProtocolManager::setLocalNodeId (const CanOpenNodeId nodeId) {
    m_localNodeId = nodeId;
}

void CanOpenProtocolManager::setLocalNetworkPosition (const CanOpenNetPosition netPos) {
    m_localNetworkPos = netPos;
}

void CanOpenProtocolManager::setSdoTimeout (const int ms) {
    m_sdoTimeout = ms;
    for (QHash<CanOpenNodeId, CanOpenSdoTransferQueue *>::const_iterator it = m_sdoTransferQueues.constBegin (), end = m_sdoTransferQueues.constEnd (); it != end; ++it) {
        it.value ()->timeout = m_sdoTimeout;
    }
}

void CanOpenProtocolManager::sendSdoWriteRequest (const CanOpenNodeId nodeId,
                                                  const CanOpenIndex idx,
                                                  const CanOpenSubIndex subIdx,
                                                  const void * srcPtr,
                                                  const CanOpenDataLen srcLen,
                                                  const bool useBlockTransfer) {
    if (srcPtr != Q_NULLPTR && srcLen > 0) {
        CanOpenSdoTransfer * transfer = new CanOpenSdoTransfer;
        transfer->commState = CanOpenSdoCommStates::PendingWrite;
        transfer->operation = CanOpenSdoOperations::SdoWrite;
        transfer->mode = CanOpenSdoModes::SdoClient;
        transfer->idx = idx;
        transfer->subIdx = subIdx;
        transfer->nodeId = nodeId;
        transfer->expectedSize = srcLen;
        transfer->createBuffer (srcLen);
        const char * srcBytes = reinterpret_cast<const char *> (srcPtr);
        for (CanOpenCounter pos = 0; pos < srcLen; ++pos) {
            transfer->buffer [pos] = srcBytes [pos];
        }
        transfer->method = (useBlockTransfer
                            ? CanOpenSdoMethods::SdoBlocks
                            : (srcLen > CanOpenSdoFrame::SDO_EXPEDITED_SIZE
                               ? CanOpenSdoMethods::SdoSegmented
                               : CanOpenSdoMethods::SdoExpedited));
        enqueueSdoTransfer (transfer);
    }
}

void CanOpenProtocolManager::sendSdoReadRequest (const CanOpenNodeId nodeId,
                                                 const CanOpenIndex idx,
                                                 const CanOpenSubIndex subIdx,
                                                 const bool useBlockTransfer) {
    CanOpenSdoTransfer * transfer = new CanOpenSdoTransfer;
    transfer->commState = CanOpenSdoCommStates::PendingRead;
    transfer->operation = CanOpenSdoOperations::SdoRead;
    transfer->mode = CanOpenSdoModes::SdoClient;
    transfer->idx = idx;
    transfer->subIdx = subIdx;
    transfer->nodeId = nodeId;
    transfer->method = (useBlockTransfer
                        ? CanOpenSdoMethods::SdoBlocks
                        : CanOpenSdoMethods::SdoUndecided);
    enqueueSdoTransfer (transfer);
}

void CanOpenProtocolManager::sendNmtChangeRequest (const CanOpenNodeId nodeId, const CanOpenNmtCmdSpecif nmtCommand) {
    quint8 frame [2];
    frame [0] = quint8 (nmtCommand);
    frame [1] = quint8 (nodeId);
    doMsgSend (new CanMessage (CanId (quint16 (0x000)), 2, frame));
}

void CanOpenProtocolManager::sendLssCommand (const CanOpenLssCmd lssCommand, const quint8 argument) {
    CanOpenLssAction * action = new CanOpenLssAction;
    action->cmd = lssCommand;
    action->argument = argument;
    action->needsReply = (lssCommand != CanOpenLssCmds::SwitchState);
    enqueueLssAction (action);
}

void CanOpenProtocolManager::onDiag (int level, const QString & description) {
    switch (CanDriver::DiagnosticLevel (level)) {
        case CanDriver::Trace:
            qDebug () << "TRACE :" << description;
            break;
        case CanDriver::Debug:
            qDebug () << "DEBUG :" << description;
            break;
        case CanDriver::Information:
            qDebug () << "INFO :" << description;
            break;
        case CanDriver::Warning:
            qWarning () << "WARNING :" << description;
            break;
        case CanDriver::Error:
            qCritical () << "ERROR :" << description;
            break;
    }
}

void CanOpenProtocolManager::onMsgRecv (CanMessage * msg) {
    if (msg != Q_NULLPTR) {
        const bool err = msg->getCanId ().isERR ();
        const bool rtr = msg->getCanId ().isRTR ();
        if (!err) {
            const CanOpenCobId   cobId   = msg->getCanId ().canIdSFF ();
            const CanOpenNodeId  nodeId  = (cobId & 0x7F);
            const CanOpenFctType fctType = CanOpenFctType ((cobId >> 7) & 0xF);
            if (m_hooksForDebug) {
                emit recvRawMsg (cobId, msg->getCanData ().toByteArray ());
            }
            bool unused = true; // FIXME : remove this hack if not in spec ?
            switch (CanOpenFctType (fctType)) {
                case CanOpenFctTypes::NMT: {
                    handleNMT (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
                case CanOpenFctTypes::SYNC_EMCY: {
                    if (nodeId == 0) {
                        handleSYNC (msg, nodeId, rtr);
                        unused = false;
                    }
                    else {
                        // TODO : EMCY ?
                        unused = false;
                    }
                    break;
                }
                case CanOpenFctTypes::TIME: {
                    if (nodeId == 0) {
                        // TODO : TIME ?
                        unused = false;
                    }
                    break;
                }
                case CanOpenFctTypes::PDOTX1:
                case CanOpenFctTypes::PDORX1:
                case CanOpenFctTypes::PDOTX2:
                case CanOpenFctTypes::PDORX2:
                case CanOpenFctTypes::PDOTX3:
                case CanOpenFctTypes::PDORX3:
                case CanOpenFctTypes::PDOTX4:
                case CanOpenFctTypes::PDORX4: {
                    handlePDO (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
                case CanOpenFctTypes::SDOTX: {
                    handleSDOTX (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
                case CanOpenFctTypes::SDORX: {
                    handleSDORX (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
                case CanOpenFctTypes::HB_NG: {
                    handleHB_NG (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
                case CanOpenFctTypes::LSS: {
                    handleLSS (msg, nodeId, rtr);
                    unused = false;
                    break;
                }
            }
            if (unused) {
                handlePDO (msg, nodeId, rtr);
            }
        }
        if (m_autoRemoveMsg) {
            delete msg;
        }
    }
}

void CanOpenProtocolManager::onSdoTimeout (void) {
    if (QTimer * timer = qobject_cast<QTimer *> (sender ())) {
        const CanOpenNodeId nodeId = timer->property ("nodeId").value<CanOpenNodeId> ();
        if (nodeId > 0 && nodeId < 128) {
            if (CanOpenSdoTransferQueue * queue = m_sdoTransferQueues.value (nodeId, Q_NULLPTR)) {
                if (CanOpenSdoTransfer * transfer = queue->currentTransfer) {
                    transfer->errState = CanOpenSdoAbortCodes::ProtocolTimeout;
                    processSdoTransfer (nodeId);
                }
            }
        }
    }
}

void CanOpenProtocolManager::onLssTimeout (void) {
    if (CanOpenLssAction * action = m_lssQueue.currentAction) {
        if (action->state == CanOpenLssRequestStates::ReplyWaiting) {
            action->state = CanOpenLssRequestStates::ReplyTimeout;
            processLssAction ();
        }
    }
}

void CanOpenProtocolManager::doMsgSend (CanMessage * msg) {
    if (msg != Q_NULLPTR) {
        if (m_driver != Q_NULLPTR) {
            if (m_hooksForDebug) {
                emit sentRawMsg (msg->getCanId ().canIdSFF (), msg->getCanData ().toByteArray ());
            }
            m_driver->send (msg);
        }
        if (m_autoRemoveMsg) {
            delete msg;
        }
    }
}

void CanOpenProtocolManager::handleNMT (CanMessage * msg,
                                        const CanOpenNodeId nodeId,
                                        const bool isRtr) {
    Q_UNUSED (nodeId)
    Q_UNUSED (isRtr)
    if (m_localNetworkPos == CanOpenNetPositions::Slave) {
        const CanOpenNmtCmdSpecif reqState = CanOpenNmtCmdSpecif (msg->getCanData ().getAlignedBitsAs<quint8> (0, 8));
        const CanOpenNodeId reqNode = msg->getCanData ().getAlignedBitsAs<quint8> (8, 8);
        if (reqNode == m_localNodeId || reqNode == 0x00) { // NOTE : addressed node is current node or all
            switch (reqState) {
                case CanOpenNmtCmdSpecifs::StartNode: {
                    m_localNodeState = CanOpenHeartBeatStates::Operational;
                    break;
                }
                case CanOpenNmtCmdSpecifs::StopNode: {
                    m_localNodeState = CanOpenHeartBeatStates::Stopped;
                    break;
                }
                case CanOpenNmtCmdSpecifs::SetPreOp: {
                    m_localNodeState = CanOpenHeartBeatStates::PreOperational;
                    break;
                }
                case CanOpenNmtCmdSpecifs::ResetNode:
                case CanOpenNmtCmdSpecifs::ResetComm: {
                    m_localNodeState = CanOpenHeartBeatStates::Initializing;
                    break;
                }
            }
            emit localNodeStateChanged (m_localNodeState);
            if (m_localNodeState == CanOpenHeartBeatStates::Initializing) {
                doInitNode ();
            }
        }
    }
}

void CanOpenProtocolManager::handleSYNC (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr) {
    Q_UNUSED (msg)
    Q_UNUSED (nodeId)
    Q_UNUSED (isRtr)
    if (m_localNodeState == CanOpenHeartBeatStates::Operational) {
        for (QHash<CanOpenCobId, CanOpenPdoConfigCache *>::const_iterator it = m_pdoConfigCaches.constBegin (), end = m_pdoConfigCaches.constEnd (); it != end; ++it) {
            CanOpenPdoConfigCache * pdoConfCache = it.value ();
            if (pdoConfCache != Q_NULLPTR && pdoConfCache->role == CanOpenPdoRoles::PdoTransmit) {
                pdoConfCache->currentSyncCount++;
                if (pdoConfCache->currentSyncCount == pdoConfCache->transmitSyncCount) {
                    pdoConfCache->currentFrameData.clear ();
                    pdoConfCache->collectMappedVars (m_objDict);
                    doMsgSend (new CanMessage (CanId (quint16 (pdoConfCache->cobId)),
                                               pdoConfCache->currentFrameData.getLength (),
                                               pdoConfCache->currentFrameData.getDataPtr ()));
                    pdoConfCache->currentSyncCount = 0;
                }
            }
        }
    }
}

void CanOpenProtocolManager::doInitNode (void) {
    connect (m_lssQueue.timer, &QTimer::timeout, this, &CanOpenProtocolManager::onLssTimeout, Qt::UniqueConnection);
    refreshSdoConfig ();
    refreshPdoMapping ();
    broadcastHeartBeat ();
    onSyncIntervalChanged ();
    onHeartBeatIntervalChanged ();
    switch (m_localNetworkPos) {
        case CanOpenNetPositions::Slave: {
            m_localNodeState = CanOpenHeartBeatStates::PreOperational;
            break;
        }
        case CanOpenNetPositions::Master: {
            m_localNodeState = CanOpenHeartBeatStates::Operational;
            break;
        }
    }
    emit localNodeStateChanged (m_localNodeState);
}

void CanOpenProtocolManager::onSyncIntervalChanged (void) {
    const quint16 syncInterval = m_objDict->getSubEntry (0x1006, 0x00)->readAs<quint16> ();
    m_syncTimer->stop ();
    if (syncInterval > 0) {
        m_syncTimer->start (syncInterval);
    }
}

void CanOpenProtocolManager::onHeartBeatIntervalChanged (void) {
    const quint16 heartBeatInterval = m_objDict->getSubEntry (0x1017, 0x00)->readAs<quint16> ();
    m_heartBeatTimer->stop ();
    if (heartBeatInterval > 0) {
        m_heartBeatTimer->start (heartBeatInterval);
    }
}

void CanOpenProtocolManager::onHeartbeatConsumerChanged (void) {
    if (CanOpenSubEntry * subEntry = qobject_cast<CanOpenSubEntry *> (sender ())) {
        const quint32 tmp = subEntry->readAs<quint32> ();
        const quint16 timeout = (tmp & 0xFFFF);
        const CanOpenNodeId nodeId = ((tmp >> 16) & 0x7F);
        if (CanOpenHeartbeatConsumer * consumer = m_hbConsumersByNodeId.value (nodeId)) {
            if (consumer->timeout != timeout) {
                consumer->timeout = timeout;
                consumer->watchdog.stop ();
                if (consumer->timeout > 0) {
                    consumer->watchdog.start (timeout, Qt::PreciseTimer, this);
                }
            }
        }
    }
}

void CanOpenProtocolManager::broadcastHeartBeat (void) {
    const quint8 frameBootUp = m_localNodeState; // NOTE : Initializing will give BootUp
    doMsgSend (new CanMessage (CanId (quint16 (0x700 + m_localNodeId)), sizeof (frameBootUp), &frameBootUp));
}

void CanOpenProtocolManager::broadcastSync (void) {
    doMsgSend (new CanMessage (CanId (quint16 (CanOpenFctTypes::SYNC_EMCY << 7)), 0, Q_NULLPTR));
    handleSYNC (Q_NULLPTR, 0, false);
}

void CanOpenProtocolManager::handleHB_NG (CanMessage * msg,
                                          const CanOpenNodeId nodeId,
                                          const bool isRtr) {
    if (isRtr) { // NOTE : Node Guarding, master asks state of node
        if (nodeId == m_localNodeId) { // NOTE : addressed node is current node
            if (m_localNodeState != CanOpenHeartBeatStates::Initializing) {
                const quint8 frameNodeGuardReply = quint8 (m_localNodeState + (m_nodeGuardToggleBit << 8));
                doMsgSend (new CanMessage (CanId (quint16 (0x700 + m_localNodeId)), sizeof (frameNodeGuardReply), &frameNodeGuardReply));
                m_nodeGuardToggleBit = !m_nodeGuardToggleBit;
            }
        }
    }
    else { // NOTE : Heart Beat, some node broadcasts its state
        const CanOpenHeartBeatState state = CanOpenHeartBeatState (msg->getCanData ().getBitsAs<quint8> (0, 7));
        if (CanOpenHeartbeatConsumer * consumer = m_hbConsumersByNodeId.value (nodeId)) {
            if (consumer->watchdog.isActive ()) {
                consumer->watchdog.stop ();
            }
            if (consumer->timeout > 0) {
                consumer->watchdog.start (consumer->timeout, Qt::PreciseTimer, this);
                if (!consumer->alive) {
                    consumer->alive = true;
                    emit remoteNodeAliveChanged (consumer->nodeId, consumer->alive);
                }
                if (consumer->state != state) {
                    consumer->state = state;
                    emit remoteNodeStateChanged (consumer->nodeId, consumer->state);
                }
            }
        }
    }
}

void CanOpenProtocolManager::handleSDORX (CanMessage * msg,
                                          const CanOpenNodeId nodeId,
                                          const bool isRtr) {
    Q_UNUSED (isRtr)
    CanOpenSdoTransferQueue * queue = m_sdoTransferQueues.value (nodeId, Q_NULLPTR);
    if (queue != Q_NULLPTR && queue->mode == CanOpenSdoModes::SdoServer) {
        CanOpenSdoClientCmdSpecif clientCommandSpecifier = CanOpenSdoClientCmdSpecif (msg->getCanData ().getBitsAs<quint8> (5, 3)); // BYTE 0, bits 5-6-7
        CanOpenSdoTransfer * transfer = queue->currentTransfer;
        if (transfer != Q_NULLPTR && transfer->method == CanOpenSdoMethods::SdoBlocks) {
            switch (transfer->operation) {
                case CanOpenSdoOperations::SdoWrite: {
                    const quint8 firstByte = msg->getCanData ().getAlignedBitsAs<quint8> (0, 8); // BYTE 0
                    if (firstByte == 0x80) {
                        // abort, will be handled by switch below
                    }
                    else if (firstByte < 0xA0) {
                        // seqno, data frame
                        const bool isLast = bool (firstByte & 0x80);
                        const CanOpenCounter seqNo = quint8 (firstByte & 0x7F);
                        if (seqNo <= transfer->blkSize) {
                            transfer->seqNo = seqNo; // TODO : check if seqNo is >= to last one
                            transfer->isLast = isLast || (transfer->seqNo == transfer->blkSize);
                            for (CanOpenCounter pos = 0; pos < CanOpenSdoFrame::SDO_SEGMENT_SIZE; ++pos) { // BYTES 1-2-3-4-5-6-7
                                transfer->buffer [transfer->currentSize] = msg->getCanData ().getAlignedBitsAs<char> ((pos +1) * 8, 8);
                                transfer->currentSize++;
                            }
                            transfer->commState = CanOpenSdoCommStates::RequestWriteDownloadBlkRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            transfer->errState = CanOpenSdoAbortCodes::InvalidSeqNo;
                            processSdoTransfer (nodeId);
                        }
                        clientCommandSpecifier = CanOpenSdoClientCmdSpecifs::RequestNone; // avoid being treated as command
                    }
                    else {
                        // commands, iwll be handled by switch below
                        qt_noop ();
                    }
                    break;
                }
                case CanOpenSdoOperations::SdoRead: {
                    // all requests are commands, handled in switch below
                    qt_noop ();
                    break;
                }
                default: break;
            }
        }
        switch (clientCommandSpecifier) {
            case CanOpenSdoClientCmdSpecifs::RequestInitDownload: {
                const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (2, 2); // BYTE 0, bits 2-3
                const bool isExpedited = msg->getCanData ().getSingleBitAs<bool> (1); // BYTE 0, bit 1
                const bool hasSizeInfo = msg->getCanData ().getSingleBitAs<bool> (0); // BYTE 0, bit 0
                const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                const CanOpenDataLen dataLen = (isExpedited
                                                ? (CanOpenSdoFrame::SDO_EXPEDITED_SIZE - emptyBytes)
                                                : (hasSizeInfo
                                                   ? msg->getCanData ().getAlignedBitsAs<quint32> (32, 32) // BYTES 4-5-6-7
                                                   : 0));
                if (transfer == Q_NULLPTR) {
                    // NOTE : client wants to initiate a write (expedited or segmented)
                    transfer = new CanOpenSdoTransfer;
                    transfer->operation = CanOpenSdoOperations::SdoWrite;
                    transfer->mode = CanOpenSdoModes::SdoServer;
                    transfer->method = (isExpedited
                                        ? CanOpenSdoMethods::SdoExpedited
                                        : CanOpenSdoMethods::SdoSegmented);
                    transfer->nodeId = nodeId;
                    transfer->idx = idx;
                    transfer->subIdx = subIdx;
                    transfer->expectedSize = dataLen;
                    transfer->createBuffer (dataLen);
                    if (isExpedited) {
                        for (CanOpenCounter pos = 0; pos < dataLen; ++pos) { // BYTES 4-5-6-7
                            transfer->buffer [pos] = msg->getCanData ().getAlignedBitsAs<char> ((pos +4) * 8, 8);
                            ++transfer->currentSize;
                        }
                    }
                    transfer->commState = (isExpedited
                                           ? CanOpenSdoCommStates::RequestWriteExpRecv
                                           : CanOpenSdoCommStates::RequestWriteInitSegRecv);
                    enqueueSdoTransfer (transfer);
                }
                else {
                    // ERROR : got request while already treating request !
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestInitUpload: {
                const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                if (transfer == Q_NULLPTR) {
                    // NOTE : client wants to initiate a read (expedited or segmented)
                    transfer = new CanOpenSdoTransfer;
                    transfer->operation = CanOpenSdoOperations::SdoRead;
                    transfer->mode = CanOpenSdoModes::SdoServer;
                    transfer->method = CanOpenSdoMethods::SdoUndecided;
                    transfer->nodeId = nodeId;
                    transfer->idx = idx;
                    transfer->subIdx = subIdx;
                    transfer->expectedSize = 0;
                    transfer->commState = CanOpenSdoCommStates::RequestReadExpOrSegRecv;
                    enqueueSdoTransfer (transfer);
                }
                else {
                    // ERROR : got request while already treating request !
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestDownloadSegment: {
                const bool toggleBit = msg->getCanData ().getSingleBitAs<bool> (4); // BYTE 0, bit 4
                const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (1, 3); // BYTE 0, bits 1-2-3
                const CanOpenDataLen dataLen = (CanOpenSdoFrame::SDO_SEGMENT_SIZE - emptyBytes);
                const bool isLastSegment = msg->getCanData ().getSingleBitAs<bool> (0); // BYTE 0, bit 0
                if (transfer != Q_NULLPTR) {
                    if (transfer->method == CanOpenSdoMethods::SdoSegmented) {
                        // NOTE : client sent segment to write in OBD
                        transfer->isLast = isLastSegment;
                        transfer->toggleBit = toggleBit;
                        for (CanOpenCounter pos = 0; pos < dataLen; ++pos) { // BYTES 1-2-3-4-5-6-7
                            transfer->buffer [transfer->currentSize] = msg->getCanData ().getAlignedBitsAs<char> ((pos +1) * 8, 8);
                            transfer->currentSize++;
                        }
                        transfer->commState = CanOpenSdoCommStates::RequestWriteDownloadSegRecv;
                        processSdoTransfer (nodeId);
                    }
                }
                else {
                    // ERROR : got segment for unstarted segment transfer
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestUploadSegment: {
                const bool toggleBit = msg->getCanData ().getSingleBitAs<bool> (4); // BYTE 1, bit 4
                if (transfer != Q_NULLPTR) {
                    if (transfer->method == CanOpenSdoMethods::SdoSegmented) {
                        // NOTE : client ask for next segment to read from OBD
                        transfer->toggleBit = toggleBit;
                        transfer->commState = CanOpenSdoCommStates::RequestReadUploadSegRecv;
                        processSdoTransfer (nodeId);
                    }
                }
                else {
                    // ERROR : got segment for unstarted segment transfer
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestAbortTransfer: {
                const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                const CanOpenSdoAbortCode code = CanOpenSdoAbortCode (msg->getCanData ().getAlignedBitsAs<quint32> (32, 32)); // BYTES 4-5-6-7
                if (transfer != Q_NULLPTR) {
                    if (transfer->idx == idx && transfer->subIdx == subIdx) {
                        // NOTE : client wants server to stop transfer
                        transfer->errState = code;
                        transfer->commState = CanOpenSdoCommStates::RequestAbortRecv;
                        processSdoTransfer (nodeId);
                    }
                    else {
                        // ERROR : abort is not for current transfer mux !
                    }
                }
                else {
                    // ERROR : abort can not be about non-existent transfer !
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestBlockDownload: {
                if (transfer == Q_NULLPTR) {
                    // NOTE : client wants to start block write
                    const bool hasSizeInfo = msg->getCanData ().getSingleBitAs<bool> (1); // BYTE 0, bit 1
                    const bool hasCrcSupport = msg->getCanData ().getSingleBitAs<bool> (2); // BYTE 0, bit 2
                    const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                    const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                    const CanOpenDataLen dataLen = (hasSizeInfo
                                                    ? msg->getCanData ().getAlignedBitsAs<quint32> (32, 32) // BYTES 4-5-6-7
                                                    : 0);
                    transfer = new CanOpenSdoTransfer;
                    transfer->operation = CanOpenSdoOperations::SdoWrite;
                    transfer->mode = CanOpenSdoModes::SdoServer;
                    transfer->method = CanOpenSdoMethods::SdoBlocks;
                    transfer->nodeId = nodeId;
                    transfer->idx = idx;
                    transfer->subIdx = subIdx;
                    transfer->useCrc = (hasCrcSupport && BLOCK_CRC_SUPPORT);
                    transfer->expectedSize = dataLen;
                    transfer->blkSize = DEFAULT_BLKSIZE;
                    transfer->createBuffer (dataLen);
                    transfer->commState = CanOpenSdoCommStates::RequestWriteInitBlkRecv;
                    enqueueSdoTransfer (transfer);
                }
                else {
                    const bool lastBit = msg->getCanData ().getSingleBitAs<bool> (0); // BYTE 0, bits 0
                    if (lastBit) { // NOTE : end command
                        const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (2, 3); // BYTE 0, bits 2-3-4
                        const quint16 crc = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                        if (transfer->useCrc) {
                            transfer->checksum = crc;
                        }
                        transfer->currentSize -= emptyBytes; // NOTE : remove empty bytes from buffer
                        transfer->commState = CanOpenSdoCommStates::RequestWriteEndBlkRecv;
                        processSdoTransfer (nodeId);
                    }
                    else {
                        // ERROR : illegal CS !
                    }
                }
                break;
            }
            case CanOpenSdoClientCmdSpecifs::RequestBlockUpload: {
                if (transfer == Q_NULLPTR) {
                    // NOTE : client wants to start block read
                    const bool hasCrcSupport = msg->getCanData ().getAlignedBitsAs<bool> (2); // BYTE 0, bit 2
                    const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                    const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                    const CanOpenCounter blkSize = msg->getCanData ().getAlignedBitsAs<quint8> (32, 8); // BYTE 4
                    //const bool canSwitchProtocol = msg->getCanData ().getBitsAs<bool> (40, 8); // BYTE 5
                    //Q_UNUSED (canSwitchProtocol)
                    transfer = new CanOpenSdoTransfer;
                    transfer->operation = CanOpenSdoOperations::SdoRead;
                    transfer->mode = CanOpenSdoModes::SdoServer;
                    transfer->method = CanOpenSdoMethods::SdoBlocks;
                    transfer->nodeId = nodeId;
                    transfer->idx = idx;
                    transfer->subIdx = subIdx;
                    transfer->useCrc = (hasCrcSupport && BLOCK_CRC_SUPPORT);
                    transfer->blkSize = blkSize;
                    transfer->commState = CanOpenSdoCommStates::RequestReadInitBlkRecv;
                    enqueueSdoTransfer (transfer);
                }
                else {
                    const quint8 firstByte = msg->getCanData ().getAlignedBitsAs<quint8> (0, 8); // BYTE 0
                    if (firstByte == 0xA3) { // NOTE : start send block
                        transfer->commState = CanOpenSdoCommStates::RequestReadUploadBlkRecv;
                        while (!transfer->isLast) {
                            processSdoTransfer (nodeId);
                        }
                    }
                    else if (firstByte == 0xA2) { // NOTE : ack and send next block
                        const CanOpenCounter lastSeqNo = msg->getCanData ().getAlignedBitsAs<quint8> (8, 8); // BYTE 1
                        const CanOpenCounter blkSize = msg->getCanData ().getAlignedBitsAs<quint8> (16, 8); // BYTE 2
                        Q_UNUSED (lastSeqNo);
                        transfer->seqNo = 0;
                        transfer->isLast = false;
                        transfer->blkSize = blkSize;
                        transfer->commState = CanOpenSdoCommStates::RequestReadAckBlkRecv;
                        while (!transfer->isLast) {
                            processSdoTransfer (nodeId);
                        }
                    }
                    else if (firstByte == 0xA1) { // NOTE : end / thanks
                        transfer->commState = CanOpenSdoCommStates::RequestReadEndBlkRecv;
                        processSdoTransfer (nodeId);
                    }
                    else {
                        // ERROR : illegal CS !
                    }
                }
                break;
            }
            default: break;
        }
    }
    else {
        // ERROR : don't handle the message because we don't watch this node ID
    }
}

void CanOpenProtocolManager::handleSDOTX (CanMessage * msg,
                                          const CanOpenNodeId nodeId,
                                          const bool isRtr) {
    Q_UNUSED (isRtr)
    CanOpenSdoTransferQueue * queue = m_sdoTransferQueues.value (nodeId, Q_NULLPTR);
    if (queue != Q_NULLPTR && queue->mode == CanOpenSdoModes::SdoClient) {
        CanOpenSdoServerCmdSpecif serverCommandSpecifier = CanOpenSdoServerCmdSpecif (msg->getCanData ().getBitsAs<quint8> (5, 3)); // BYTE 0, bits 5-6-7
        CanOpenSdoTransfer * transfer = queue->currentTransfer;
        if (transfer != Q_NULLPTR && transfer->method == CanOpenSdoMethods::SdoBlocks) {
            const quint8 firstByte = msg->getCanData ().getAlignedBitsAs<quint8> (0, 8); // BYTE 0
            switch (transfer->operation) {
                case CanOpenSdoOperations::SdoRead: {
                    if (firstByte == 0x80) {
                        // abort, will be treated by switch below
                    }
                    else if (firstByte < 0xA0) {
                        // seqno, data frame
                        const bool isLast = bool (firstByte & 0x80);
                        const CanOpenCounter seqNo = quint8 (firstByte & 0x7F);
                        if (seqNo <= transfer->blkSize) {
                            transfer->seqNo = seqNo; // TODO : check if seqNo is >= to last one
                            transfer->isLast = isLast || (transfer->seqNo == transfer->blkSize);
                            for (CanOpenCounter pos = 0; pos < CanOpenSdoFrame::SDO_SEGMENT_SIZE; ++pos) { // BYTES 1-2-3-4-5-6-7
                                transfer->buffer [transfer->currentSize] = msg->getCanData ().getAlignedBitsAs<char> ((pos +1) * 8, 8);
                                transfer->currentSize++;
                            }
                            transfer->commState = CanOpenSdoCommStates::ReplyReadUploadBlkRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            transfer->errState = CanOpenSdoAbortCodes::InvalidSeqNo;
                            processSdoTransfer (nodeId);
                        }
                        serverCommandSpecifier = CanOpenSdoServerCmdSpecifs::ReplyNone; // avoid being treated as command
                    }
                    else {
                        // 0xA*, normal command, will be handled by switch below
                    }
                    break;
                }
                case CanOpenSdoOperations::SdoWrite: {
                    // all replies are commands, handled by switch below
                    break;
                }
                default: break;
            }
        }
        switch (serverCommandSpecifier) {
            case CanOpenSdoServerCmdSpecifs::ReplyAcceptDownload: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestWriteExpSent) {
                        // NOTE : server accepted expedited write
                        transfer->commState = CanOpenSdoCommStates::ReplyWriteExpRecv;
                        processSdoTransfer (nodeId);
                    }
                    else if (transfer->commState == CanOpenSdoCommStates::RequestWriteInitSegSent) {
                        // NOTE : server starts accepting segments
                        transfer->commState = CanOpenSdoCommStates::ReplyWriteInitSegRecv;
                        processSdoTransfer (nodeId);
                    }
                    else {
                        // ERROR : should receive 'accept' only after request
                    }
                }
                else {
                    // ERROR : no current request for this node !
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyAcceptUpload: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestReadExpOrSegSent) {
                        const bool isExpedited = msg->getCanData ().getSingleBitAs<bool> (1); // BYTE 0, bit 1
                        const bool hasSizeInfo = msg->getCanData ().getSingleBitAs<bool> (0); // BYTE 0, bit 0
                        if (isExpedited) {
                            // NOTE : server accepted expedited read
                            const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (2, 2); // BYTE 0, bits 2-3
                            const CanOpenDataLen payloadSize = (CanOpenSdoFrame::SDO_EXPEDITED_SIZE - emptyBytes);
                            transfer->method = CanOpenSdoMethods::SdoExpedited;
                            transfer->createBuffer (payloadSize);
                            for (CanOpenCounter pos = 0; pos < payloadSize; ++pos) {
                                transfer->buffer [transfer->currentSize] = msg->getCanData ().getAlignedBitsAs<char> ((pos +4) * 8, 8);
                                transfer->currentSize++;
                            }
                            transfer->commState = CanOpenSdoCommStates::ReplyReadExpRecv;
                        }
                        else {
                            // NOTE : starts sending segments
                            transfer->method = CanOpenSdoMethods::SdoSegmented;
                            if (hasSizeInfo) {
                                transfer->expectedSize = msg->getCanData ().getAlignedBitsAs<quint32> (32, 32);
                                transfer->createBuffer (transfer->expectedSize);
                            }
                            else {
                                transfer->createBuffer (MAX_SDO_BUFFER_SIZE);
                            }
                            transfer->createBuffer (transfer->expectedSize);
                            transfer->commState = CanOpenSdoCommStates::ReplyReadInitSegRecv;
                        }
                        processSdoTransfer (nodeId);
                    }
                    else {
                        // ERROR : should receive 'accept' only after request
                    }
                }
                else {
                    // ERROR : no current request for this node !
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyDownloadSegment: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestWriteDownloadSegSent) {
                        if (transfer->method == CanOpenSdoMethods::SdoSegmented) {
                            transfer->toggleBit = !transfer->toggleBit; // NOTE : invert toggle bit
                            transfer->commState = CanOpenSdoCommStates::ReplyWriteDownloadSegRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            // ERROR : i'm not in a segmented transfer !
                        }
                    }
                    else {
                        // ERROR : i didn't sent write segment request !
                    }
                }
                else {
                    // ERROR : received segment for unstarted transer !
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyUploadSegment: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestReadUploadSegSent) {
                        if (transfer->method == CanOpenSdoMethods::SdoSegmented) {
                            const bool isLast = msg->getCanData ().getSingleBitAs<bool> (0); // BYTE 0, bit 0
                            const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (2, 2); // BYTE 0, bits 2-3
                            const CanOpenDataLen payloadSize = (CanOpenSdoFrame::SDO_SEGMENT_SIZE - emptyBytes);
                            for (CanOpenCounter pos = 0; pos < payloadSize; ++pos) {
                                transfer->buffer [transfer->currentSize] = msg->getCanData ().getAlignedBitsAs<char> ((pos +1) * 8, 8);
                                transfer->currentSize++;
                            }
                            transfer->isLast    = isLast; // NOTE : last segment flag
                            transfer->toggleBit = !transfer->toggleBit; // NOTE : invert toggle bit
                            transfer->commState = CanOpenSdoCommStates::ReplyReadUploadSegRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            // ERROR : i'm not in a segmented transfer !
                        }
                    }
                    else {
                        // ERROR : i didn't sent read segment request !
                    }
                }
                else {
                    // ERROR : received segment for unstarted transer !
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyAbortTransfer: {
                const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                const CanOpenSdoAbortCode code = CanOpenSdoAbortCode (msg->getCanData ().getAlignedBitsAs<quint32> (32, 32)); // BYTES 4-5-6-7
                if (transfer != Q_NULLPTR) {
                    if (transfer->idx == idx
                        && transfer->subIdx == subIdx) {
                        // NOTE : server aborted the transfer
                        transfer->errState = code;
                        transfer->commState = CanOpenSdoCommStates::ReplyAbortRecv;
                        processSdoTransfer (nodeId);
                    }
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyBlockDownload: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestWriteInitBlkSent) {
                        const quint8 lastBits = msg->getCanData ().getBitsAs<quint8> (0, 2); // BYTES 0, bits 0-1
                        if (lastBits == 0x0) {
                            const bool useCrc = msg->getCanData ().getSingleBitAs<bool> (2); // BYTES 0, bit 2
                            const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                            const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                            if (transfer->idx == idx
                                && transfer->subIdx == subIdx) {
                                const CanOpenCounter blkSize = msg->getCanData ().getAlignedBitsAs<quint8> (32, 8); // BYTE 4
                                if (blkSize > 0 && blkSize < 128) {
                                    transfer->blkSize = blkSize;
                                    transfer->isLast = false;
                                    transfer->seqNo = 0;
                                    transfer->useCrc = (BLOCK_CRC_SUPPORT && useCrc);
                                    transfer->commState = CanOpenSdoCommStates::ReplyWriteInitBlkRecv;
                                    while (!transfer->isLast) {
                                        processSdoTransfer (nodeId);
                                    }
                                }
                                else {
                                    // NOTE : blksize is invalid !
                                    transfer->errState = CanOpenSdoAbortCodes::InvalidBlockSize;
                                    processSdoTransfer (nodeId);
                                }
                            }
                            else {
                                // ERROR : obd position is not the same as the request asked !
                            }
                        }
                        else {
                            // ERROR : weird command flags ! should be 00b
                        }
                    }
                    else if (transfer->commState == CanOpenSdoCommStates::RequestWriteDownloadBlkSent) {
                        const quint8 lastBits = msg->getCanData ().getBitsAs<quint8> (0, 2); // BYTES 0, bits 0-1
                        if (lastBits == 0x2) {
                            const CanOpenCounter lastSeqNo = msg->getCanData ().getAlignedBitsAs<quint8> (8, 8); // BYTE 1
                            const CanOpenCounter blkSize   = msg->getCanData ().getAlignedBitsAs<quint8> (16, 8); // BYTE 2
                            if (lastSeqNo == transfer->seqNo) {
                                if (transfer->currentSize < transfer->expectedSize) {
                                    // NOTE : still blocks to send
                                    if (blkSize > 0 && blkSize < 128) {
                                        transfer->blkSize = blkSize;
                                        transfer->isLast = false;
                                        transfer->seqNo = 0;
                                        transfer->commState = CanOpenSdoCommStates::ReplyWriteDownloadBlkRecv;
                                        while (!transfer->isLast) {
                                            processSdoTransfer (nodeId);
                                        }
                                    }
                                    else {
                                        // NOTE : blksize is invalid !
                                        transfer->errState = CanOpenSdoAbortCodes::InvalidBlockSize;
                                        processSdoTransfer (nodeId);
                                    }
                                }
                                else {
                                    // NOTE : end transfer
                                    transfer->commState = CanOpenSdoCommStates::ReplyWriteDownloadBlkRecv;
                                    processSdoTransfer (nodeId);
                                }
                            }
                            else {
                                // ERROR : some segments were missed, must resend all !
                            }
                        }
                        else {
                            // ERROR : weird command flags ! should be 10b
                        }
                    }
                    else if (transfer->commState == CanOpenSdoCommStates::RequestWriteEndBlkSent) {
                        const quint8 lastBits = msg->getCanData ().getBitsAs<quint8> (0, 2); // BYTES 0, bits 0-1
                        if (lastBits == 0x1) {
                            // end
                            transfer->commState = CanOpenSdoCommStates::ReplyWriteEndBlkRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            // ERROR : weird command flags ! should be 01b
                        }
                    }
                }
                else {
                    // ERROR : received reply for unstarted transfer !
                }
                break;
            }
            case CanOpenSdoServerCmdSpecifs::ReplyBlockUpload: {
                if (transfer != Q_NULLPTR) {
                    if (transfer->commState == CanOpenSdoCommStates::RequestReadInitBlkSent) {
                        // server has accepted blk transfer and gives params
                        const bool hasSizeIndicator = msg->getCanData ().getSingleBitAs<bool> (1); // BYTES 0, bit 1
                        const bool useCrc = msg->getCanData ().getSingleBitAs<bool> (2); // BYTES 0, bit 2
                        const CanOpenIndex idx = msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                        const CanOpenSubIndex subIdx = msg->getCanData ().getAlignedBitsAs<quint8> (24, 8); // BYTE 3
                        if (transfer->idx == idx
                            && transfer->subIdx == subIdx) {
                            if (hasSizeIndicator) {
                                const CanOpenDataLen dataLen = msg->getCanData ().getAlignedBitsAs<quint32> (32, 32); // BYTES 4-5-6-7
                                transfer->expectedSize = dataLen;
                                transfer->createBuffer (dataLen);
                            }
                            transfer->isLast = false;
                            transfer->seqNo = 0;
                            transfer->useCrc = (BLOCK_CRC_SUPPORT && useCrc);
                            transfer->commState = CanOpenSdoCommStates::ReplyReadInitBlkRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            // ERROR : obd position is not the same as the request asked !
                        }
                    }
                    else if (transfer->commState == CanOpenSdoCommStates::RequestReadAckBlkSent) {
                        const quint8 lastBits = msg->getCanData ().getBitsAs<quint8> (0, 2); // BYTES 0, bits 0-1
                        if (lastBits == 0x1) {
                            // end
                            const CanOpenCounter emptyBytes = msg->getCanData ().getBitsAs<quint8> (2, 3); // BYTES 0, bits 2-3-4
                            transfer->currentSize -= emptyBytes; // NOTE : remove empty bytes from buffer
                            if (transfer->useCrc) {
                                transfer->checksum =  msg->getCanData ().getAlignedBitsAs<quint16> (8, 16); // BYTES 1-2
                            }
                            transfer->commState = CanOpenSdoCommStates::ReplyReadEndBlkRecv;
                            processSdoTransfer (nodeId);
                        }
                        else {
                            // ERROR : weird command flags ! should be 01b
                        }
                    }
                }
                else {
                    // ERROR : received reply for unstarted transfer !
                }
                break;
            }
            default: break;
        }
    }
    else {
        // ERROR : don't handle the message because we don't watch this node ID
    }
}

void CanOpenProtocolManager::handlePDO (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr) {
    Q_UNUSED (isRtr)
    Q_UNUSED (nodeId)
    const CanOpenCobId cobId = msg->getCanId ().canIdSFF ();
    CanOpenPdoConfigCache * pdoConfCache = m_pdoConfigCaches.value (cobId, Q_NULLPTR);
    if (pdoConfCache != Q_NULLPTR && pdoConfCache->role == CanOpenPdoRoles::PdoReceive) {
        pdoConfCache->currentFrameData.replaceWith (msg->getCanData ().getDataPtr (), msg->getCanData ().getLength ());
        pdoConfCache->parseMappedVars (m_objDict);
    }
}

void CanOpenProtocolManager::handleLSS (CanMessage * msg, const CanOpenNodeId nodeId, const bool isRtr) {
    Q_UNUSED (isRtr)
    if (nodeId == 0x65) { // LSS request
        if (m_localNetworkPos == CanOpenNetPositions::Slave) {
            // TODO : handle LSS request as slave ?
        }
    }
    else if (nodeId == 0x64) { // LSS reply
        if (m_localNetworkPos == CanOpenNetPositions::Master) {
            const quint8 cmd     = msg->getCanData ().getAlignedBitsAs<quint8> (0, 8);
            const quint8 errCode = msg->getCanData ().getAlignedBitsAs<quint8> (8, 8);
            if (m_lssQueue.currentAction != Q_NULLPTR) {
                if (m_lssQueue.currentAction->state == CanOpenLssRequestStates::ReplyWaiting) {
                    if (m_lssQueue.currentAction->cmd == cmd) {
                        if (errCode == 0) {
                            m_lssQueue.currentAction->state = CanOpenLssRequestStates::ReplyReceived;
                        }
                        else {
                            m_lssQueue.currentAction->state = CanOpenLssRequestStates::ReplyError;
                        }
                        processLssAction ();
                    }
                }
            }
        }
    }
}

void CanOpenProtocolManager::refreshSdoConfig (void) {
    QHash<CanOpenNodeId, CanOpenSdoMode> tmp;
    qDeleteAll (m_sdoTransferQueues);
    m_sdoTransferQueues.clear ();
    for (CanOpenIndex idx = CanOpenObjDictRanges::SDO_SERVERS_START; idx < CanOpenObjDictRanges::SDO_SERVERS_END; ++idx) { // NOTE : find servers
        if (CanOpenEntry * entry = m_objDict->getEntry (idx)) {
            const CanOpenCobId cobIdCliRequest = entry->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->readAs<quint32> ();
            const CanOpenCobId cobIdSrvReply   = entry->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->readAs<quint32> ();
            const CanOpenNodeId nodeIdCliRequest = CanOpenNodeId (cobIdCliRequest - CanOpenSdoCobIds::CliRequest);
            const CanOpenNodeId nodeIdSrvReply   = CanOpenNodeId (cobIdSrvReply   - CanOpenSdoCobIds::SrvReply);
            if (nodeIdCliRequest && nodeIdSrvReply) {
                if (nodeIdCliRequest == nodeIdSrvReply) {
                    CanOpenSdoTransferQueue * queue = new CanOpenSdoTransferQueue (nodeIdCliRequest, CanOpenSdoModes::SdoServer);
                    queue->timeout = m_sdoTimeout;
                    m_sdoTransferQueues.insert (queue->nodeId, queue);
                    tmp.insert (queue->nodeId, queue->mode);
                    connect (queue->timer, &QTimer::timeout, this, &CanOpenProtocolManager::onSdoTimeout, Qt::UniqueConnection);
                }
                else {
                    qWarning () << "COBID for RX and TX do not contain same server node ID !" << cobIdCliRequest << cobIdSrvReply;
                }
            }
        }
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::SDO_CLIENTS_START; idx < CanOpenObjDictRanges::SDO_CLIENTS_END; ++idx) { // NOTE : find clients
        if (CanOpenEntry * entry = m_objDict->getEntry (idx)) {
            const CanOpenCobId cobIdCliRequest = entry->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->readAs<quint32> ();
            const CanOpenCobId cobIdSrvReply   = entry->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->readAs<quint32> ();
            const CanOpenNodeId nodeIdCliRequest = CanOpenNodeId (cobIdCliRequest - CanOpenSdoCobIds::CliRequest);
            const CanOpenNodeId nodeIdSrvReply   = CanOpenNodeId (cobIdSrvReply   - CanOpenSdoCobIds::SrvReply);
            if (nodeIdCliRequest && nodeIdSrvReply) {
                if (nodeIdCliRequest == nodeIdSrvReply) {
                    CanOpenSdoTransferQueue * queue = new CanOpenSdoTransferQueue (nodeIdSrvReply, CanOpenSdoModes::SdoClient);
                    queue->timeout = m_sdoTimeout;
                    m_sdoTransferQueues.insert (queue->nodeId, queue);
                    tmp.insert (queue->nodeId, queue->mode);
                    connect (queue->timer, &QTimer::timeout, this, &CanOpenProtocolManager::onSdoTimeout, Qt::UniqueConnection);
                }
                else {
                    qWarning () << "COBID for RX and TX do not contain same client node ID !" << cobIdCliRequest << cobIdSrvReply;
                }
            }
        }
    }
    emit configuredSDOs (tmp);
}

void CanOpenProtocolManager::refreshPdoMapping (void) {
    QHash<CanOpenCobId, CanOpenPdoRole> tmp;
    qDeleteAll (m_pdoConfigCaches);
    m_pdoConfigCaches.clear ();
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDORX_CONFIG_START; idx < CanOpenObjDictRanges::PDORX_CONFIG_END; ++idx) {
        if (CanOpenEntry * entryConfig = m_objDict->getEntry (idx)) {
            const CanOpenCounter pdoNum = (idx - CanOpenObjDictRanges::PDORX_CONFIG_START);
            if (CanOpenSubEntry * subEntryCobId = entryConfig->getSubEntry (0x01)) {
                CanOpenCobId cobId = subEntryCobId->readAs<quint32> ();
                if (cobId != 0x000) {
                    if (CanOpenEntry * entryMapping = m_objDict->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDORX_MAPPING_START + pdoNum))) {
                        CanOpenPdoConfigCache * pdoConfCache = new CanOpenPdoConfigCache;
                        pdoConfCache->cobId = cobId;
                        pdoConfCache->role = CanOpenPdoRoles::PdoReceive;
                        pdoConfCache->entryMapping = entryMapping;
                        pdoConfCache->entryCommParams = entryConfig;
                        m_pdoConfigCaches.insert (cobId, pdoConfCache);
                        tmp.insert (pdoConfCache->cobId, pdoConfCache->role);
                    }
                }
            }
        }
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDOTX_CONFIG_START; idx < CanOpenObjDictRanges::PDOTX_CONFIG_END; ++idx) {
        if (CanOpenEntry * entryConfig = m_objDict->getEntry (idx)) {
            const CanOpenCounter pdoNum = (idx - CanOpenObjDictRanges::PDOTX_CONFIG_START);
            if (CanOpenSubEntry * subEntryCobId = entryConfig->getSubEntry (0x01)) {
                CanOpenCobId cobId = subEntryCobId->readAs<quint32> ();
                if (CanOpenSubEntry * subEntryTransmitType = entryConfig->getSubEntry (0x02)) {
                    Q_UNUSED (subEntryTransmitType)
                    if (cobId != 0x000) {
                        if (CanOpenEntry * entryMapping = m_objDict->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDOTX_MAPPING_START + pdoNum))) {
                            CanOpenPdoConfigCache * pdoConfCache = new CanOpenPdoConfigCache;
                            pdoConfCache->cobId = cobId;
                            pdoConfCache->role = CanOpenPdoRoles::PdoTransmit;
                            pdoConfCache->entryMapping = entryMapping;
                            pdoConfCache->entryCommParams = entryConfig;
                            pdoConfCache->transmitSyncCount = 1;
                            m_pdoConfigCaches.insert (cobId, pdoConfCache);
                            tmp.insert (pdoConfCache->cobId, pdoConfCache->role);
                        }
                    }
                }
            }
        }
    }
    emit configuredPDOs (tmp);
}

void CanOpenProtocolManager::enqueueSdoTransfer (CanOpenSdoTransfer * transfer) {
    if (transfer != Q_NULLPTR) {
        if (CanOpenSdoTransferQueue * queue = m_sdoTransferQueues.value (transfer->nodeId)) {
            if (queue->mode == transfer->mode) {
                if (queue->currentTransfer == Q_NULLPTR) {
                    queue->currentTransfer = transfer;
                    processSdoTransfer (queue->nodeId);
                }
                else {
                    queue->transfersQueue.append (transfer); // NOTE : don't process it now, wait for the current one to finish / timeout
                }
            }
            else {
                emit failedToSendSdo (QStringLiteral ("The queue for node-ID %1 is not the same mode as the transfer !").arg (transfer->nodeId));
                delete transfer;
            }
        }
        else {
            emit failedToSendSdo (QStringLiteral ("There is no SDO queue for node-ID %1 !").arg (transfer->nodeId));
            delete transfer;
        }
    }
    else {
        emit failedToSendSdo ("Can't enqueue invalid transfer !");
    }
}

void CanOpenProtocolManager::processSdoTransfer (const CanOpenNodeId nodeId) {
    CanOpenSdoTransferQueue * queue = m_sdoTransferQueues.value (nodeId, Q_NULLPTR);
    if (queue != Q_NULLPTR) {
        CanOpenSdoTransfer * transfer = queue->currentTransfer;
        if (transfer != Q_NULLPTR) {
            if (m_hooksForDebug) {
                emit beforeProcessTransfer (transfer);
            }
            queue->killTimer ();
            CanOpenSdoFrame sdoFrame;
            CanOpenSubEntry * subEntry = m_objDict->getSubEntry (transfer->idx, transfer->subIdx);
            const CanOpenDataLen objectSize = (subEntry ? subEntry->getDataLen () : 0);
            switch (transfer->mode) {
                case CanOpenSdoModes::SdoClient: {
                    switch (transfer->operation) {
                        case CanOpenSdoOperations::SdoRead: {
                            if (transfer->commState == CanOpenSdoCommStates::PendingRead) {
                                // NOTE : should send read req
                                if (transfer->method != CanOpenSdoMethods::SdoBlocks) {
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestInitUpload << 5);
                                    sdoFrame.mutiplexed.idx = transfer->idx;
                                    sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                    transfer->commState = CanOpenSdoCommStates::RequestReadExpOrSegSent;
                                    transfer->sendFrame = true;
                                }
                                else {
                                    transfer->blkSize = DEFAULT_BLKSIZE;
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockUpload << 5);
                                    sdoFrame.commandSpecifier |= (BLOCK_CRC_SUPPORT << 2); // CRC support flag
                                    sdoFrame.mutiplexed.idx = transfer->idx;
                                    sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                    sdoFrame.mutiplexed.dataBytes [0] = quint8 (transfer->blkSize);
                                    sdoFrame.mutiplexed.dataBytes [1] = false; // don't allow switch protocol
                                    transfer->commState = CanOpenSdoCommStates::RequestReadInitBlkSent;
                                    transfer->sendFrame = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyReadExpRecv) {
                                // NOTE : recv server exp read reply, done
                                transfer->finished = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyReadInitSegRecv
                                     || transfer->commState == CanOpenSdoCommStates::ReplyReadUploadSegRecv) {
                                // NOTE : recv server seg read init reply
                                if (transfer->currentSize < transfer->expectedSize && !transfer->isLast) {
                                    // NOTE : ask for next segment
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestUploadSegment << 5);
                                    sdoFrame.commandSpecifier |= (transfer->toggleBit << 4); // NOTE : copy toggle bit from request
                                    transfer->commState = CanOpenSdoCommStates::RequestReadUploadSegSent;
                                    transfer->sendFrame = true;
                                }
                                else {
                                    // NOTE : done
                                    transfer->finished = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyReadInitBlkRecv) {
                                // ask to start transfer
                                sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockUpload << 5);
                                sdoFrame.commandSpecifier |= 0x3; // start flag 11b
                                transfer->commState = CanOpenSdoCommStates::RequestReadUploadBlkSent;
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyReadUploadBlkRecv) {
                                if (transfer->isLast) {
                                    // ask for next block
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockUpload << 5);
                                    sdoFrame.commandSpecifier |= 0x2; // ack flag
                                    sdoFrame.segmentedBytes [0] = quint8 (transfer->seqNo);
                                    sdoFrame.segmentedBytes [1] = quint8 (transfer->blkSize);
                                    transfer->commState = CanOpenSdoCommStates::RequestReadAckBlkSent;
                                    transfer->sendFrame = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyReadEndBlkRecv) {
                                // ask to end transfer
                                bool mismatch = false;
                                if (transfer->useCrc) {
                                    const quint16 crc = computeCRC16CCITT (transfer->buffer.constData (), transfer->currentSize);
                                    if (transfer->checksum != crc) {
                                        mismatch = true;
                                    }
                                }
                                if (!mismatch) {
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockUpload << 5);
                                    sdoFrame.commandSpecifier |= 0x1; // end flag 01b
                                    transfer->commState = CanOpenSdoCommStates::RequestReadEndBlkSent;
                                    transfer->sendFrame = true;
                                    transfer->finished = true;
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::CrcMismatch;
                                }
                            }
                            break;
                        }
                        case CanOpenSdoOperations::SdoWrite: {
                            if (transfer->commState == CanOpenSdoCommStates::PendingWrite) {
                                // NOTE : should send write req
                                if (transfer->method != CanOpenSdoMethods::SdoBlocks) {
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestInitDownload << 5);
                                    sdoFrame.mutiplexed.idx = transfer->idx;
                                    sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                    if (transfer->method == CanOpenSdoMethods::SdoExpedited) {
                                        // NOTE : send exp write req
                                        sdoFrame.commandSpecifier |= (0x1 << 1); // NOTE : is expedited
                                        sdoFrame.commandSpecifier |= ((4 - transfer->expectedSize) << 2); // NOTE : empty bytes
                                        sdoFrame.commandSpecifier |= 0x1; // NOTE : has size info
                                        const CanOpenDataLen len = CanOpenDataLen (transfer->buffer.size ());
                                        for (CanOpenCounter pos = 0; pos < len; ++pos) {
                                            sdoFrame.mutiplexed.dataBytes [pos] = qbyte (transfer->buffer [pos]);
                                        }
                                        transfer->currentSize = len;
                                        transfer->commState = CanOpenSdoCommStates::RequestWriteExpSent;
                                    }
                                    else {
                                        // NOTE : init seg write req
                                        sdoFrame.mutiplexed.idx = transfer->idx;
                                        sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                        sdoFrame.commandSpecifier |= 0x1; // NOTE : has size info
                                        const CanOpenDataLen len = CanOpenDataLen (transfer->buffer.size ());
                                        sdoFrame.mutiplexed.dataRaw = len;
                                        transfer->commState = CanOpenSdoCommStates::RequestWriteInitSegSent;
                                    }
                                }
                                else {
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockDownload << 5);
                                    sdoFrame.commandSpecifier |= (BLOCK_CRC_SUPPORT << 2); // NOTE : CRC support flag
                                    sdoFrame.commandSpecifier |= (true << 1); // NOTE : size indication present
                                    sdoFrame.mutiplexed.idx = transfer->idx;
                                    sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                    sdoFrame.mutiplexed.dataRaw = transfer->expectedSize; // NOTE : data size
                                    transfer->commState = CanOpenSdoCommStates::RequestWriteInitBlkSent;
                                }
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyWriteExpRecv) {
                                // NOTE : done
                                transfer->finished = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyWriteInitSegRecv
                                     || transfer->commState == CanOpenSdoCommStates::ReplyWriteDownloadSegRecv) {
                                if (!transfer->isLast) {
                                    const CanOpenDataLen payloadSize = (transfer->currentSize < transfer->expectedSize
                                                                        - CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                        ? CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                        : transfer->expectedSize - transfer->currentSize);
                                    const CanOpenCounter emptyBytes = (CanOpenSdoFrame::SDO_SEGMENT_SIZE - payloadSize);
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestDownloadSegment << 5);
                                    sdoFrame.commandSpecifier |= (transfer->toggleBit << 4);
                                    sdoFrame.commandSpecifier |= (emptyBytes << 1);
                                    for (CanOpenCounter pos = 0; pos < payloadSize; ++pos) {
                                        sdoFrame.segmentedBytes [pos] = qbyte (transfer->buffer [transfer->currentSize]);
                                        transfer->currentSize++;
                                    }
                                    if (transfer->currentSize == transfer->expectedSize) {
                                        sdoFrame.commandSpecifier |= 0x1; // NOTE : set last segment flag
                                        transfer->isLast = true;
                                    }
                                    transfer->commState = CanOpenSdoCommStates::RequestWriteDownloadSegSent;
                                    transfer->sendFrame = true;
                                }
                                else {
                                    transfer->finished = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyWriteInitBlkRecv
                                     || transfer->commState == CanOpenSdoCommStates::ReplyWriteDownloadBlkRecv
                                     || transfer->commState == CanOpenSdoCommStates::RequestWriteDownloadBlkSent) {
                                if (transfer->currentSize < transfer->expectedSize) {
                                    // NOTE : still data to send
                                    if (transfer->seqNo < transfer->blkSize) {
                                        // NOTE : send one more block
                                        transfer->seqNo++;
                                        sdoFrame.commandSpecifier = quint8 (transfer->seqNo);
                                        if (transfer->seqNo == transfer->blkSize) {
                                            transfer->isLast = true; // NOTE : last segment sent
                                        }
                                        for (CanOpenCounter pos = 0; pos < CanOpenSdoFrame::SDO_SEGMENT_SIZE; ++pos) {
                                            if (transfer->currentSize < transfer->expectedSize) {
                                                sdoFrame.segmentedBytes [pos] = qbyte (transfer->buffer [transfer->currentSize]);
                                                transfer->currentSize++;
                                            }
                                            else {
                                                sdoFrame.commandSpecifier |= 0x80; // NOTE : last segment flag
                                                transfer->isLast = true; // NOTE : last segment sent
                                                break;
                                            }
                                        }
                                        transfer->commState = CanOpenSdoCommStates::RequestWriteDownloadBlkSent;
                                        transfer->sendFrame = true;
                                    }
                                }
                                else {
                                    // NOTE : send end command
                                    sdoFrame.commandSpecifier = (CanOpenSdoClientCmdSpecifs::RequestBlockDownload << 5);
                                    sdoFrame.commandSpecifier |= (0x1); // bits 0-1 : end flag
                                    const CanOpenCounter emptyBytes = CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                      - (transfer->expectedSize
                                                                         % CanOpenSdoFrame::SDO_SEGMENT_SIZE);
                                    sdoFrame.commandSpecifier |= (emptyBytes << 2); // bits 2-3-4
                                    if (transfer->useCrc) {
                                        const quint16 crc = computeCRC16CCITT (transfer->buffer.constData (), transfer->expectedSize);
                                        transfer->checksum = crc;
                                        sdoFrame.segmentedBytes [0] = (crc & 0xFF);
                                        sdoFrame.segmentedBytes [1] = ((crc >> 8) & 0xFF);
                                    }
                                    transfer->commState = CanOpenSdoCommStates::RequestWriteEndBlkSent;
                                    transfer->sendFrame = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::ReplyWriteEndBlkRecv) {
                                // NOTE : end of block transfer
                                transfer->finished = true;
                            }
                            break;
                        }
                        default: break;
                    }
                    break;
                }
                case CanOpenSdoModes::SdoServer: {
                    switch (transfer->operation) {
                        case CanOpenSdoOperations::SdoRead: {
                            if (transfer->commState == CanOpenSdoCommStates::RequestReadExpOrSegRecv) {
                                if (subEntry != Q_NULLPTR) {
                                    if (subEntry->isReadable ()) {
                                        transfer->expectedSize = objectSize;
                                        transfer->createBuffer (objectSize);
                                        subEntry->read (transfer->buffer.data (), objectSize);
                                        const CanOpenCounter emptyBytes = (CanOpenSdoFrame::SDO_EXPEDITED_SIZE - objectSize);
                                        sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyAcceptUpload << 5);
                                        sdoFrame.mutiplexed.idx = transfer->idx;
                                        sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                        if (objectSize <= CanOpenSdoFrame::SDO_EXPEDITED_SIZE) {
                                            sdoFrame.commandSpecifier |= (emptyBytes << 2);
                                            sdoFrame.commandSpecifier |= (0x1 << 1); // NOTE : expedited flag
                                            sdoFrame.commandSpecifier |= 0x1; // NOTE : data size flag
                                            for (CanOpenCounter pos = 0; pos < objectSize; ++pos) {
                                                sdoFrame.mutiplexed.dataBytes [pos] = qbyte (transfer->buffer [transfer->currentSize]);
                                                transfer->currentSize++;
                                            }
                                            transfer->method = CanOpenSdoMethods::SdoExpedited;
                                            transfer->commState = CanOpenSdoCommStates::ReplyReadExpSent;
                                            transfer->finished = true;
                                        }
                                        else {
                                            sdoFrame.mutiplexed.dataRaw = objectSize;
                                            sdoFrame.commandSpecifier |= 0x1; // NOTE : data size flag
                                            transfer->method = CanOpenSdoMethods::SdoSegmented;
                                            transfer->commState = CanOpenSdoCommStates::ReplyReadInitSegSent;
                                        }
                                        transfer->sendFrame = true;
                                    }
                                    else {
                                        transfer->errState = CanOpenSdoAbortCodes::AttemptToReadObjectThatIsWO;
                                    }
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::ObjectDoesntExistInOBD;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestReadUploadSegRecv) {
                                // NOTE : send the next read seg
                                const CanOpenDataLen payloadSize = (transfer->currentSize < transfer->expectedSize
                                                                    - CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                    ? CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                    : transfer->expectedSize - transfer->currentSize);
                                const CanOpenCounter emptyBytes = (CanOpenSdoFrame::SDO_SEGMENT_SIZE - payloadSize);
                                sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyUploadSegment << 5);
                                sdoFrame.commandSpecifier |= (transfer->toggleBit << 4); // NOTE : copy toggle bit from request
                                sdoFrame.commandSpecifier |= (emptyBytes << 1);
                                for (CanOpenCounter pos = 0; pos < payloadSize; ++pos) {
                                    sdoFrame.segmentedBytes [pos] = qbyte (transfer->buffer [transfer->currentSize]);
                                    transfer->currentSize++;
                                }
                                if (transfer->currentSize == transfer->expectedSize) {
                                    sdoFrame.commandSpecifier |= 0x1; // NOTE : set last segment flag
                                    transfer->finished = true;
                                }
                                transfer->commState = CanOpenSdoCommStates::ReplyReadUploadSegSent;
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestReadInitBlkRecv) {
                                if (subEntry != Q_NULLPTR) {
                                    if (subEntry->isReadable ()) {
                                        transfer->expectedSize = objectSize;
                                        transfer->createBuffer (objectSize);
                                        subEntry->read (transfer->buffer.data (), objectSize);
                                        sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyBlockUpload << 5);
                                        sdoFrame.commandSpecifier |= (transfer->useCrc << 2); // CRC support flag
                                        sdoFrame.commandSpecifier |= (true << 1); // size indicator present
                                        sdoFrame.mutiplexed.idx = transfer->idx;
                                        sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                        sdoFrame.mutiplexed.dataRaw = transfer->expectedSize;
                                        transfer->commState = CanOpenSdoCommStates::ReplyReadInitBlkSent;
                                        transfer->sendFrame = true;
                                    }
                                    else {
                                        transfer->errState = CanOpenSdoAbortCodes::AttemptToReadObjectThatIsWO;
                                    }
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::ObjectDoesntExistInOBD;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestReadUploadBlkRecv
                                     || transfer->commState == CanOpenSdoCommStates::RequestReadAckBlkRecv
                                     || transfer->commState == CanOpenSdoCommStates::ReplyReadUploadBlkSent) {
                                if (transfer->currentSize < transfer->expectedSize) {
                                    // NOTE : still data to send
                                    if (transfer->seqNo < transfer->blkSize) {
                                        // NOTE : send one more block
                                        transfer->seqNo++;
                                        sdoFrame.commandSpecifier = quint8 (transfer->seqNo);
                                        if (transfer->seqNo == transfer->blkSize) {
                                            transfer->isLast = true; // NOTE : last segment of this block
                                        }
                                        for (CanOpenCounter pos = 0; pos < CanOpenSdoFrame::SDO_SEGMENT_SIZE; ++pos) {
                                            if (transfer->currentSize < transfer->expectedSize) {
                                                sdoFrame.segmentedBytes [pos] = qbyte (transfer->buffer [transfer->currentSize]);
                                                transfer->currentSize++;
                                            }
                                            else {
                                                sdoFrame.commandSpecifier |= 0x80; // NOTE : last segment flag
                                                transfer->isLast = true; // NOTE : last byte of all
                                                break;
                                            }
                                        }
                                        transfer->commState = CanOpenSdoCommStates::ReplyReadUploadBlkSent;
                                        transfer->sendFrame = true;
                                    }
                                }
                                else {
                                    // NOTE : send end command
                                    sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyBlockUpload << 5);
                                    sdoFrame.commandSpecifier |= (0x1); // bits 0-1 : end flag
                                    const CanOpenCounter emptyBytes = CanOpenSdoFrame::SDO_SEGMENT_SIZE
                                                                      - (transfer->expectedSize
                                                                         % CanOpenSdoFrame::SDO_SEGMENT_SIZE);
                                    sdoFrame.commandSpecifier |= (emptyBytes << 2); // bits 2-3-4
                                    if (transfer->useCrc) {
                                        const quint16 crc = computeCRC16CCITT (transfer->buffer.constData (), transfer->expectedSize);
                                        transfer->checksum = crc;
                                        sdoFrame.segmentedBytes [0] = (crc & 0xFF);
                                        sdoFrame.segmentedBytes [1] = ((crc >> 8) & 0xFF);
                                    }
                                    transfer->isLast = true;
                                    transfer->commState = CanOpenSdoCommStates::ReplyReadEndBlkSent;
                                    transfer->sendFrame = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestReadEndBlkRecv) {
                                // NOTE : client has finished
                                transfer->finished = true;
                            }
                            break;
                        }
                        case CanOpenSdoOperations::SdoWrite: {
                            if (transfer->commState == CanOpenSdoCommStates::RequestWriteExpRecv) {
                                // NOTE : should reply done
                                if (subEntry != Q_NULLPTR) {
                                    if (subEntry->isWritable ()) {
                                        if (transfer->expectedSize == objectSize) {
                                            subEntry->write (transfer->buffer.data (), quint (transfer->buffer.size ()));
                                            sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyAcceptDownload << 5);
                                            sdoFrame.mutiplexed.idx = transfer->idx;
                                            sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                            transfer->commState = CanOpenSdoCommStates::ReplyWriteExpSent;
                                            transfer->finished = true;
                                        }
                                        else {
                                            transfer->errState = CanOpenSdoAbortCodes::DataSizeOrSizeDoesNotMatch;
                                        }
                                    }
                                    else {
                                        transfer->errState = CanOpenSdoAbortCodes::AttemptToWriteObjectThatIsRO;
                                    }
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::ObjectDoesntExistInOBD;
                                }
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestWriteInitSegRecv) {
                                if (subEntry != Q_NULLPTR) {
                                    if (subEntry->isWritable ()) {
                                        if (transfer->expectedSize == objectSize
                                            || (CanOpenDataTypes::isTypeString (subEntry->getDataType ())
                                                && transfer->expectedSize <= objectSize)) {
                                            subEntry->write (transfer->buffer.data (), quint (transfer->buffer.size ()));
                                            sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyAcceptDownload << 5);
                                            sdoFrame.mutiplexed.idx = transfer->idx;
                                            sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                            transfer->commState = CanOpenSdoCommStates::ReplyWriteInitSegSent;
                                        }
                                        else {
                                            transfer->errState = CanOpenSdoAbortCodes::DataSizeOrSizeDoesNotMatch;
                                        }
                                    }
                                    else {
                                        transfer->errState = CanOpenSdoAbortCodes::AttemptToWriteObjectThatIsRO;
                                    }
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::ObjectDoesntExistInOBD;
                                }
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestWriteDownloadSegRecv) {
                                // NOTE : should copy data from seg to buffer
                                sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyDownloadSegment << 5);
                                sdoFrame.commandSpecifier |= (transfer->toggleBit << 4); // NOTE : copy toggle bit from request
                                if (transfer->isLast) {
                                    if (subEntry != Q_NULLPTR) {
                                        subEntry->write (transfer->buffer.constData (), quint (transfer->buffer.size ()));
                                    }
                                    transfer->finished = true;
                                }
                                transfer->commState = CanOpenSdoCommStates::ReplyWriteDownloadSegSent;
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestWriteInitBlkRecv) {
                                // NOTE : should answer with blksize and CRC support
                                if (subEntry != Q_NULLPTR) {
                                    if (subEntry->isWritable ()) {
                                        if (transfer->expectedSize == objectSize) {
                                            sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyBlockDownload << 5);
                                            sdoFrame.commandSpecifier |= (BLOCK_CRC_SUPPORT << 2); // NOTE : CRC support flag
                                            sdoFrame.mutiplexed.idx = transfer->idx;
                                            sdoFrame.mutiplexed.subIdx = transfer->subIdx;
                                            sdoFrame.mutiplexed.dataBytes [0] = quint8 (transfer->blkSize);
                                            transfer->commState = CanOpenSdoCommStates::ReplyWriteInitBlkSent;
                                        }
                                        else {
                                            transfer->errState = CanOpenSdoAbortCodes::DataSizeOrSizeDoesNotMatch;
                                        }
                                    }
                                    else {
                                        transfer->errState = CanOpenSdoAbortCodes::AttemptToWriteObjectThatIsRO;
                                    }
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::ObjectDoesntExistInOBD;
                                }
                                transfer->sendFrame = true;
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestWriteDownloadBlkRecv) {
                                // NOTE : received data block
                                if (transfer->isLast) {
                                    // NOTE : should ack
                                    sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyBlockDownload << 5);
                                    sdoFrame.commandSpecifier |= (0x1 << 1); // ack flag
                                    sdoFrame.segmentedBytes [0] = quint8 (transfer->seqNo);
                                    sdoFrame.segmentedBytes [1] = quint8 (DEFAULT_BLKSIZE);
                                    transfer->commState = CanOpenSdoCommStates::ReplyWriteDownloadBlkSent;
                                    transfer->sendFrame = true;
                                }
                            }
                            else if (transfer->commState == CanOpenSdoCommStates::RequestWriteEndBlkRecv) {
                                // NOTE : transfer has been ended, write in OBD and ack
                                bool mismatch = false;
                                if (transfer->useCrc) {
                                    const quint16 crc = computeCRC16CCITT (transfer->buffer.data (), transfer->currentSize);
                                    if (crc != transfer->checksum) {
                                        mismatch = true;
                                    }
                                }
                                if (!mismatch) {
                                    if (subEntry != Q_NULLPTR) {
                                        subEntry->write (transfer->buffer.data (), transfer->currentSize);
                                    }
                                    sdoFrame.commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyBlockDownload << 5);
                                    sdoFrame.commandSpecifier |= (0x1); // NOTE : ack flag
                                    transfer->commState = CanOpenSdoCommStates::ReplyWriteEndBlkSent;
                                    transfer->sendFrame = true;
                                    transfer->finished = true;
                                }
                                else {
                                    transfer->errState = CanOpenSdoAbortCodes::CrcMismatch;
                                }
                            }
                            break;
                        }
                        default: break;
                    }
                    break;
                }
                default: break;
            }
            if (m_hooksForDebug) {
                emit afterProcessTransfer (transfer);
            }
            if (transfer->errState != CanOpenSdoAbortCodes::NoError) {
                transfer->finished = true;
                if (transfer->commState != CanOpenSdoCommStates::ReplyAbortRecv) {
                    sdoFrame.setError (transfer->idx, transfer->subIdx, transfer->errState);
                    transfer->sendFrame = true;
                }
            }
            if (transfer->sendFrame) {
                switch (transfer->mode) {
                    case CanOpenSdoModes::SdoClient: {
                        const CanId requestCobId = CanId (quint16 (CanOpenSdoCobIds::CliRequest + transfer->nodeId));
                        doMsgSend (new CanMessage (requestCobId, CanOpenSdoFrame::SDO_FRAME_SIZE, &sdoFrame));
                        break;
                    }
                    case CanOpenSdoModes::SdoServer: {
                        const CanId replyCobId = CanId (quint16 (CanOpenSdoCobIds::SrvReply + transfer->nodeId));
                        doMsgSend (new CanMessage (replyCobId, CanOpenSdoFrame::SDO_FRAME_SIZE, &sdoFrame));
                        break;
                    }
                    default: break;
                }
                if (!transfer->finished) {
                    queue->restartTimer ();
                }
                transfer->sendFrame = false;
            }
            if (transfer->finished) {
                switch (transfer->mode) {
                    case CanOpenSdoModes::SdoClient: {
                        if (transfer->operation == CanOpenSdoOperations::SdoRead) {
                            emit recvSdoReadReply (transfer->nodeId,
                                                   transfer->idx,
                                                   transfer->subIdx,
                                                   transfer->errState,
                                                   transfer->buffer.left (int (transfer->currentSize)));
                        }
                        else if (transfer->operation == CanOpenSdoOperations::SdoWrite) {
                            emit recvSdoWriteReply (transfer->nodeId,
                                                    transfer->idx,
                                                    transfer->subIdx,
                                                    transfer->errState,
                                                    transfer->buffer.left (int (transfer->currentSize)));
                        }
                        break;
                    }
                    case CanOpenSdoModes::SdoServer: {
                        if (transfer->operation == CanOpenSdoOperations::SdoRead) {
                            emit recvSdoReadRequest (transfer->nodeId,
                                                     transfer->idx,
                                                     transfer->subIdx,
                                                     transfer->errState,
                                                     transfer->buffer.left (int (transfer->currentSize)));
                        }
                        else if (transfer->operation == CanOpenSdoOperations::SdoWrite) {
                            emit recvSdoWriteRequest (transfer->nodeId,
                                                      transfer->idx,
                                                      transfer->subIdx,
                                                      transfer->errState,
                                                      transfer->buffer.left (int (transfer->currentSize)));
                        }
                        break;
                    }
                    default: break;
                }
                queue->currentTransfer = Q_NULLPTR;
                delete transfer;
                if (!queue->transfersQueue.isEmpty ()) {
                    queue->currentTransfer = queue->transfersQueue.dequeue ();
                    processSdoTransfer (nodeId);
                }
            }
        }
    }
}

void CanOpenProtocolManager::enqueueLssAction (CanOpenLssAction * action) {
    if (action != Q_NULLPTR) {
        if (m_lssQueue.currentAction == Q_NULLPTR) {
            m_lssQueue.currentAction = action;
            processLssAction ();
        }
        else {
            m_lssQueue.actionsQueue.append (action); // NOTE : don't process it now, wait for the current one to finish / timeout
        }
    }
}

void CanOpenProtocolManager::processLssAction (void) {
    if (CanOpenLssAction * action = m_lssQueue.currentAction) {
        m_lssQueue.killTimer ();
        bool done = false;
        switch (int (action->state)) {
            case CanOpenLssRequestStates::PendingSend: {
                CanData frame (Q_NULLPTR, 8);
                frame.setBitsWith<quint8> (0, 8, quint8 (action->cmd));
                switch (int (action->cmd)) {
                    case CanOpenLssCmds::SwitchState:
                        frame.setBitsWith<quint8> (8, 8, action->argument);
                        break;
                    case CanOpenLssCmds::ChangeNodeId:
                        frame.setBitsWith<quint8> (8, 8, action->argument);
                        break;
                    case CanOpenLssCmds::ChangeBitrate: // NOTE : argument on 3rd byte !
                        frame.setBitsWith<quint8> (16, 8, action->argument);
                        break;
                    case CanOpenLssCmds::StoreConfig:
                        break;
                }
                action->state = CanOpenLssRequestStates::RequestSent;
                doMsgSend (new CanMessage (CanId (quint16 (0x7E5)), frame.getLength (), frame.getDataPtr ()));
                if (action->needsReply) {
                    action->state = CanOpenLssRequestStates::ReplyWaiting;
                    m_lssQueue.restartTimer ();
                }
                else {
                    action->state = CanOpenLssRequestStates::ReplyUneeded;
#if QT_VERSION >= 0x050400
                    QTimer::singleShot (5, this, &CanOpenProtocolManager::processLssAction);
#else
                    QTimer::singleShot (5, this, SLOT (processLssAction ()));
#endif
                }
                break;
            }
            case CanOpenLssRequestStates::ReplyUneeded: {
                done = true;
                break;
            }
            case CanOpenLssRequestStates::ReplyReceived: {
                done = true;
                break;
            }
            case CanOpenLssRequestStates::ReplyError: {
                done = true;
                break;
            }
            case CanOpenLssRequestStates::ReplyTimeout: {
                done = true;
                break;
            }
        }
        if (done) {
            m_lssQueue.currentAction = Q_NULLPTR;
            delete action;
            if (!m_lssQueue.actionsQueue.isEmpty ()) {
                m_lssQueue.currentAction = m_lssQueue.actionsQueue.dequeue ();
                processLssAction ();
            }
        }
    }
}

void CanOpenProtocolManager::timerEvent (QTimerEvent * event) {
    for (QHash<CanOpenNodeId, CanOpenHeartbeatConsumer *>::const_iterator it = m_hbConsumersByNodeId.constBegin (),
         end = m_hbConsumersByNodeId.constEnd ();
         it != end;
         ++it) {
        if (CanOpenHeartbeatConsumer * consumer = it.value ()) {
            if (consumer->watchdog.timerId () == event->timerId ()) {
                if (consumer->watchdog.isActive ()) {
                    consumer->watchdog.stop ();
                }
                if (consumer->alive) {
                    consumer->alive = false;
                    emit remoteNodeAliveChanged (consumer->nodeId, consumer->alive);
                }
                break;
            }
        }
    }
}
