
#include "SharedObject.h"

#include "CanMessage.h"
#include "CanDriver.h"
#include "CanDriverLoader.h"
#include "CanOpenObjDict.h"
#include "CanOpenProtocolManager.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "AbstractObdPreparator.h"

#include <QStringBuilder>
#include <qqml.h>

#define QS QStringLiteral

#define TEST_MSG qWarning () << qPrintable (QDateTime::currentDateTimeUtc ().toString ("hh:mm:ss.zzz")) << "[TEST]"

QString SharedObject::dumpAbortCode (const CanOpenSdoAbortCode code) {
    static QMetaEnum metaEnum = CanOpenSdoAbortCodes::staticMetaObject.enumerator (0);
    return QString::fromLatin1 (metaEnum.valueToKey (code));
}

QString SharedObject::dumpFctType (const CanOpenFctType fctType) {
    switch ((CanOpenFctType) (fctType)) {
        case CanOpenFctTypes::NMT:  return "NMT";
        case CanOpenFctTypes::SYNC_EMCY: return "SYNC/EMCY";
        case CanOpenFctTypes::PDOTX1:
        case CanOpenFctTypes::PDORX1:
        case CanOpenFctTypes::PDOTX2:
        case CanOpenFctTypes::PDORX2:
        case CanOpenFctTypes::PDOTX3:
        case CanOpenFctTypes::PDORX3:
        case CanOpenFctTypes::PDOTX4:
        case CanOpenFctTypes::PDORX4:
            return "PDO RX/TX";
        case CanOpenFctTypes::SDOTX:
            return "SDO TX";
        case CanOpenFctTypes::SDORX:
            return "SDO RX";
        case CanOpenFctTypes::HB_NG:
            return "HEARTBEAT / NODEGUARD";
        case CanOpenFctTypes::LSS:
            return "LSS";
    }
    return "UNKNOWN";
}

QString SharedObject::dumpFrame (const quint16 cobId, const QByteArray & data) {
    const CanOpenNodeId  nodeId  = CanOpenNodeId  (cobId & 0x7F);
    const CanOpenFctType fctType = CanOpenFctType ((cobId >> 7) & 0xF);
    return QS ("%1 [FCT=%2 (%3) NODE=%4] %5")
            .arg (hex (cobId, 3))
            .arg (fctType)
            .arg (dumpFctType (fctType))
            .arg (nodeId)
            .arg (QString::fromLocal8Bit (data.toHex ()));
}

QString SharedObject::dumpTransfer (CanOpenSdoTransfer * transfer) {
    QString ret;
    ret.reserve (100);
    ret.append ("Transfer");
    ret.append ('(');
    ret.append (hex (qintptr (transfer), sizeof (qintptr) * 2));
    ret.append (')');
    ret.append ('{');
    if (transfer != Q_NULLPTR) {
        ret.append ("op=");
        switch (transfer->operation) {
            case CanOpenSdoOperations::SdoWrite: ret.append ("W"); break;
            case CanOpenSdoOperations::SdoRead:  ret.append ("R"); break;
            case CanOpenSdoOperations::SdoNoOp:  ret.append ("?"); break;
        }
        ret.append (' ');
        ret.append (QString::number (transfer->nodeId));
        ret.append ('@');
        ret.append (hex (transfer->idx, 4));
        ret.append (':');
        ret.append (QString::number (transfer->subIdx));
        ret.append (' ');
        ret.append ("act=");
        switch (transfer->method) {
            case CanOpenSdoMethods::SdoExpedited: ret.append ("EXP"); break;
            case CanOpenSdoMethods::SdoSegmented: ret.append ("SEG"); break;
            case CanOpenSdoMethods::SdoBlocks:    ret.append ("BLK"); break;
            case CanOpenSdoMethods::SdoUndecided: ret.append ("N/A"); break;
        }
        ret.append (' ');
        ret.append ("mod=");
        switch (transfer->mode) {
            case CanOpenSdoModes::SdoClient: ret.append ("CLI");  break;
            case CanOpenSdoModes::SdoServer: ret.append ("SRV");  break;
            case CanOpenSdoModes::SdoNeiter: ret.append ("N/A"); break;
        }
        ret.append (' ');
        ret.append ("state=");
        static QMetaEnum metaEnum = CanOpenSdoCommStates::staticMetaObject.enumerator (0);
        ret.append (QString::fromLatin1 (metaEnum.valueToKey (transfer->commState)));
        ret.append (' ');
        ret.append ("bytes=");
        ret.append (QString::number (transfer->currentSize));
        ret.append ('/');
        ret.append (QString::number (transfer->expectedSize));
        ret.append (' ');
        ret.append ("seqno=");
        ret.append (QString::number (transfer->seqNo));
        ret.append (' ');
        ret.append ("blksize=");
        ret.append (QString::number (transfer->blkSize));
        ret.append (' ');
        ret.append ("useCRC=");
        ret.append (transfer->useCrc ? 'Y' : 'N');
        ret.append (' ');
        ret.append (hex (transfer->checksum, 4));
        ret.append (' ');
        ret.append ("isLast=");
        ret.append (transfer->isLast ? 'Y' : 'N');
        ret.append (' ');
        ret.append ("done=");
        ret.append (transfer->finished ? 'Y' : 'N');
        //ret.append (" buf=");
        //ret.append ('\'');
        //ret.append (transfer->buffer.toHex ());
        //ret.append ('\'');
    }
    else {
        ret.append ("NULL");
    }
    ret.append ('}');
    return ret;
}

SharedObject::SharedObject (QObject * parent) : QObject (parent), m_nodeStarted (false) {
    m_driverWrapper = new CanDriverWrapper (this);
    m_canOpenMan = new CanOpenProtocolManager (true, true, this);
    connect (m_canOpenMan, &CanOpenProtocolManager::recvRawMsg,            this, &SharedObject::onRecvRawMsg);
    connect (m_canOpenMan, &CanOpenProtocolManager::sentRawMsg,            this, &SharedObject::onSentRawMsg);
    connect (m_canOpenMan, &CanOpenProtocolManager::beforeProcessTransfer, this, &SharedObject::onBeforeProcessTransfer);
    connect (m_canOpenMan, &CanOpenProtocolManager::afterProcessTransfer,  this, &SharedObject::onAfterProcessTransfer);
    connect (m_canOpenMan, &CanOpenProtocolManager::configuredPDOs,        this, &SharedObject::onConfiguredPDOs);
    connect (m_canOpenMan, &CanOpenProtocolManager::configuredSDOs,        this, &SharedObject::onConfiguredSDOs);
    connect (m_canOpenMan, &CanOpenProtocolManager::localNodeStateChanged, this, &SharedObject::onNodeStateChanged);
    connect (m_canOpenMan, &CanOpenProtocolManager::failedToSendSdo,       this, &SharedObject::onFailedToSendSdo);
    connect (m_canOpenMan, &CanOpenProtocolManager::recvSdoReadReply,      this, &SharedObject::onRecvSdoReadReply);
    connect (m_canOpenMan, &CanOpenProtocolManager::recvSdoReadRequest,    this, &SharedObject::onRecvSdoReadRequest);
    connect (m_canOpenMan, &CanOpenProtocolManager::recvSdoWriteReply,     this, &SharedObject::onRecvSdoWriteReply);
    connect (m_canOpenMan, &CanOpenProtocolManager::recvSdoWriteRequest,   this, &SharedObject::onRecvSdoWriteRequest);
}

int SharedObject::getRoleIndex (QAbstractItemModel * model, const QString & roleName) {
    int ret = -1;
    if (model != Q_NULLPTR) {
        ret = model->roleNames ().key (roleName.toLatin1 (), -1);
    }
    return ret;
}

void SharedObject::startNode (const QString & mode, const int nodeId, const QString & obd) {
    static const QString SLAVE  = QS ("SLAVE");
    static const QString MASTER = QS ("MASTER");
    if (nodeId > 0 && !obd.isEmpty ()) {
        if (mode == SLAVE) {
            m_canOpenMan->setLocalNetworkPosition (CanOpenNetPositions::Slave);
        }
        else if (mode == MASTER) {
            m_canOpenMan->setLocalNetworkPosition (CanOpenNetPositions::Master);
        }
        else { }
        onDiag (CanDriver::Information, QS ("Node ID : %1").arg (nodeId));
        m_canOpenMan->setLocalNodeId (CanOpenNodeId (nodeId));
        onDiag (CanDriver::Information, QS ("Driver name : %1").arg (m_driverWrapper->getDriverName ()));
        if (m_driverWrapper->getDriverLoaded ()) {
            connect (m_driverWrapper->getDriverObject (), &CanDriver::diag, this, &SharedObject::onDiag);
            onDiag (CanDriver::Information, QS ("QML OBD URL : %1").arg (obd));
            CanOpenObdPreparator preparator;
            connect (&preparator, &CanOpenObdPreparator::diag, this, &SharedObject::onDiag);
            preparator.loadFromQmlFile (obd, m_canOpenMan->getObjDict ());
            m_canOpenMan->start (m_driverWrapper->getDriverObject ());
            onDiag (CanDriver::Information, QS ("Manager started"));
            update_nodeStarted (true);
        }
    }
}

void SharedObject::sendSdoRequest (const QString & operation, const QString & method, const int nodeId, const int index, const int subIndex, const int dataLen, const QString & type, const QString & value) {
    static const QString READ   = QS ("READ");
    static const QString WRITE  = QS ("WRITE");
    static const QString BLK    = QS ("BLK");
    static const QString BYTES  = QS ("BYTES");
    static const QString NUMBER = QS ("NUMBER");
    const bool useBlockMode = (method == BLK);
    if (operation == READ) {
        onDiag (CanDriver::Debug,
                QS ("Send SDO %1 Read request to node %2 on %3:%4")
                .arg (method)
                .arg (nodeId)
                .arg (hex (index, 4))
                .arg (subIndex));
        m_canOpenMan->sendSdoReadRequest (CanOpenNodeId (nodeId), CanOpenIndex (index), CanOpenSubIndex (subIndex), useBlockMode);
    }
    else if (operation == WRITE) {
        onDiag (CanDriver::Debug,
                QS ("Send SDO %1 Write request to node %2 on %3:%4 with size %5 and data %6 '%7'")
                .arg (method)
                .arg (nodeId)
                .arg (hex (index, 4))
                .arg (subIndex)
                .arg (dataLen)
                .arg (type)
                .arg (value));
        QByteArray tmp (dataLen, '\0');
        if (type == BYTES) {
            const QByteArray data = QByteArray::fromHex (value.toLocal8Bit ());
            const quint count = quint (tmp.size () < data.size () ? tmp.size () : data.size ());
            for (quint pos = 0; pos < count; pos++) {
                tmp [pos] = data [pos];
            }
        }
        else if (type == NUMBER) {
            bool ok = false;
            const int num = value.toInt (&ok, 0); // auto base
            if (ok) {
                switch (dataLen) {
                    case 1:
                        (*((quint8 *) (tmp.data ()))) = quint8 (num);
                        break;
                    case 2:
                        (*((quint16 *) (tmp.data ()))) = quint16 (num);
                        break;
                    case 4:
                        (*((quint32 *) (tmp.data ()))) = quint32 (num);
                        break;
                    case 8:
                        (*((quint64 *) (tmp.data ()))) = quint64 (num);
                        break;
                    default:
                        break;
                }
            }
        }
        else { }
        m_canOpenMan->sendSdoWriteRequest (CanOpenNodeId (nodeId), CanOpenIndex (index), CanOpenSubIndex (subIndex), tmp.constData (), CanOpenDataLen (tmp.size ()), useBlockMode);
    }
    else { }
}

void SharedObject::sendNmtCommand (const int nodeId, const int command) {
    onDiag (CanDriver::Debug,
            QS ("Ask node %1 to go to state %2")
            .arg (nodeId)
            .arg (command));
    m_canOpenMan->sendNmtChangeRequest (CanOpenNodeId (nodeId), CanOpenNmtCmdSpecif (command));
}

void SharedObject::changeSyncInterval (const int interval) {
    onDiag (CanDriver::Debug,
            QS ("Change SYNC interval to %1ms")
            .arg (interval));
    m_canOpenMan->getObjDict ()->getSubEntry (0x1006, 0x00)->writeAs<quint16> (quint16 (interval));
}

void SharedObject::changeHeartBeatInterval (const int interval) {
    onDiag (CanDriver::Debug,
            QS ("Change HeartBeat interval to %1ms")
            .arg (interval));
    m_canOpenMan->getObjDict ()->getSubEntry (0x1017, 0x00)->writeAs<quint16> (quint16 (interval));
}

void SharedObject::onDiag (const int level, const QString & description) {
    switch ((CanDriver::DiagnosticLevel) (level)) {
        case CanDriver::Trace:
            TEST_MSG << "TRACE" << qPrintable (description);
            emit diagLogRequested ("TRACE", description);
            break;
        case CanDriver::Debug:
            TEST_MSG << "DEBUG" << qPrintable (description);
            emit diagLogRequested ("DEBUG", description);
            break;
        case CanDriver::Information:
            TEST_MSG << "INFO" << qPrintable (description);
            emit diagLogRequested ("INFO", description);
            break;
        case CanDriver::Warning:
            TEST_MSG << "WARNING" << qPrintable (description);
            emit diagLogRequested ("WARNING", description);
            break;
        case CanDriver::Error:
            TEST_MSG << "ERROR" << qPrintable (description);
            emit diagLogRequested ("ERROR", description);
            break;
    }
}

void SharedObject::onConfiguredPDOs (const QHash<CanOpenCobId, CanOpenPdoRole> & pdosList) {
    for (QHash<CanOpenCobId, CanOpenPdoRole>::const_iterator
         it = pdosList.constBegin ();
         it != pdosList.constEnd ();
         ++it) {
        QString tmp;
        switch (it.value ()) {
            case CanOpenPdoRoles::PdoReceive:  tmp = "RECEIVE";  break;
            case CanOpenPdoRoles::PdoTransmit: tmp = "TRANSMIT"; break;
            default: break;
        }
        onDiag (CanDriver::Debug,
                QS ("On test node the PDO with Cob-ID %1 is %2")
                .arg (hex (it.key (), 3))
                .arg (tmp));
    }
}

void SharedObject::onConfiguredSDOs (const QHash<CanOpenNodeId, CanOpenSdoMode> & sdosList) {
    for (QHash<CanOpenNodeId, CanOpenSdoMode>::const_iterator
         it = sdosList.constBegin ();
         it != sdosList.constEnd ();
         ++it) {
        QString tmp;
        switch (it.value ()) {
            case CanOpenSdoModes::SdoClient: tmp = "CLIENT"; break;
            case CanOpenSdoModes::SdoServer: tmp = "SERVER"; break;
            default: break;
        }
        onDiag (CanDriver::Debug,
                QS ("On test node the SDO line for node-ID %1 is %2")
                .arg (hex (it.key (), 2))
                .arg (tmp));
    }
}

void SharedObject::onRecvRawMsg (const quint16 cobId, const QByteArray & data) {
    onDiag (CanDriver::Trace,
            QS ("RECV on test node : %1")
            .arg (dumpFrame (cobId, data)));
}

void SharedObject::onSentRawMsg (const quint16 cobId, const QByteArray & data) {
    onDiag (CanDriver::Trace,
            QS ("SENT from test node : %1")
            .arg (dumpFrame (cobId, data)));
}

void SharedObject::onBeforeProcessTransfer (CanOpenSdoTransfer * transfer) {
    onDiag (CanDriver::Trace,
            QS ("Before process SDO on test node : %1")
            .arg (dumpTransfer (transfer)));
}

void SharedObject::onAfterProcessTransfer (CanOpenSdoTransfer * transfer) {
    onDiag (CanDriver::Trace,
            QS ("After process SDO on test node : %1")
            .arg (dumpTransfer (transfer)));
}

void SharedObject::onRecvSdoWriteReply (const CanOpenNodeId nodeId, const CanOpenIndex idx, const CanOpenSubIndex subIdx, const CanOpenSdoAbortCode statusCode, const QByteArray & buffer) {
    onDiag (CanDriver::Debug,
            QS ("Test node finished SDO Write %1:%2 to node %3 with code %4, data %5 bytes = '%6'")
            .arg (hex (idx, 4))
            .arg (subIdx)
            .arg (nodeId)
            .arg (dumpAbortCode (statusCode))
            .arg (buffer.size ())
            .arg (QString::fromLocal8Bit (buffer.toHex ())));
}

void SharedObject::onRecvSdoReadReply (const CanOpenNodeId nodeId, const CanOpenIndex idx, const CanOpenSubIndex subIdx, const CanOpenSdoAbortCode statusCode, const QByteArray & buffer) {
    onDiag (CanDriver::Debug,
            QS ("Test node finished SDO Read %1:%2 to node %3 with code %4, data %5 bytes = '%6'")
            .arg (hex (idx, 4))
            .arg (subIdx)
            .arg (nodeId)
            .arg (dumpAbortCode (statusCode))
            .arg (buffer.size ())
            .arg (QString::fromLocal8Bit (buffer.toHex ())));
}

void SharedObject::onRecvSdoWriteRequest (const CanOpenNodeId nodeId, const CanOpenIndex idx, const CanOpenSubIndex subIdx, const CanOpenSdoAbortCode statusCode, const QByteArray & buffer) {
    onDiag (CanDriver::Debug,
            QS ("Test node accepted SDO Write %1:%2 from node %3 with code %4, data %5 bytes = '%6'")
            .arg (hex (idx, 4))
            .arg (subIdx)
            .arg (nodeId)
            .arg (dumpAbortCode (statusCode))
            .arg (buffer.size ())
            .arg (QString::fromLocal8Bit (buffer.toHex ())));
}

void SharedObject::onRecvSdoReadRequest (const CanOpenNodeId nodeId, const CanOpenIndex idx, const CanOpenSubIndex subIdx, const CanOpenSdoAbortCode statusCode, const QByteArray & buffer) {
    onDiag (CanDriver::Debug,
            QS ("Test node accepted SDO Read %1:%2 from node %3 with code %4, data %5 bytes = '%6'")
            .arg (hex (idx, 4))
            .arg (subIdx)
            .arg (nodeId)
            .arg (dumpAbortCode (statusCode))
            .arg (buffer.size ())
            .arg (QString::fromLocal8Bit (buffer.toHex ())));
}

void SharedObject::onNodeStateChanged (const CanOpenHeartBeatState state) {
    static QMetaEnum metaEnum = CanOpenHeartBeatStates::staticMetaObject.enumerator (0);
    const QString tmp = QString::fromLatin1 (metaEnum.valueToKey (state));
    onDiag (CanDriver::Debug,
            QS ("Test node state changed to %1")
            .arg (tmp));
}

void SharedObject::onFailedToSendSdo (const QString & reason) {
    onDiag (CanDriver::Error, reason);
}
