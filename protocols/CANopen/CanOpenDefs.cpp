
#include "CanOpenDefs.h"

#include "CanMessage.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenObjDict.h"

/*** CAN OPEN DEFS TYPES ***/

CanOpenDataLen CanOpenDataTypes::sizeOfType (const CanOpenDataType type, const CanOpenDataLen fallback) {
    switch (type) {
        case Bool:
        case Int8:
        case UInt8:
            return 1;
        case Int16:
        case UInt16:
            return 2;
        case Int32:
        case UInt32:
        case Real32:
            return 4;
        case Int64:
        case UInt64:
        case Real64:
            return 8;
        default:
            return fallback;
    }
}

bool CanOpenDataTypes::isTypeString (const CanOpenDataType type) {
    switch (type) {
        case OctetStr:
        case VisibleStr:
        case UnicodeStr:
            return true;
        default:
            return false;
    }
}

/*** CAN OPEN OBD POS ***/

CanOpenObdPos::CanOpenObdPos (const quint32 raw)
    : raw (raw)
{ }

CanOpenObdPos::CanOpenObdPos (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const CanOpenNodeId nodeId) {
    fields.idx    = idx;
    fields.subIdx = subIdx;
    fields.nodeId = nodeId;
}

/*** CAN OPEN PDO MAPPING ***/

CanOpenPdoMapping::CanOpenPdoMapping (const quint32 raw)
    : raw (raw)
{ }

CanOpenPdoMapping::CanOpenPdoMapping (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const quint8 bitsCount) {
    fields.bitsCount = bitsCount;
    fields.subIdx    = subIdx;
    fields.idx       = idx;
}

/*** CAN OPEN SDO FRAME ***/

CanOpenSdoFrame::CanOpenSdoFrame (void) {
    memset (this, 0x00, sizeof (CanOpenSdoFrame));
}

void CanOpenSdoFrame::setError (const CanOpenIndex idx, const CanOpenSubIndex ssidx, const quint32 errorCode) {
    commandSpecifier = (CanOpenSdoServerCmdSpecifs::ReplyAbortTransfer << 5);
    mutiplexed.idx = idx;
    mutiplexed.subIdx = ssidx;
    mutiplexed.dataRaw = errorCode;
}

/*** CAN OPEN SDO TRANSFER ***/

CanOpenSdoTransfer::CanOpenSdoTransfer (void)
    : operation (CanOpenSdoOperations::SdoNoOp)
    , mode (CanOpenSdoModes::SdoNeiter)
    , method (CanOpenSdoMethods::SdoUndecided)
    , commState (CanOpenSdoCommStates::InvalidState)
    , errState (CanOpenSdoAbortCodes::NoError)
    , nodeId (0x00)
    , idx (0x0000)
    , subIdx (0x00)
    , expectedSize (0)
    , currentSize (0)
    , seqNo (0)
    , blkSize (0)
    , isLast (false)
    , toggleBit (false)
    , finished (false)
    , sendFrame (false)
    , useCrc (false)
    , checksum (0x0000)
{ }

void CanOpenSdoTransfer::createBuffer (const CanOpenDataLen len) {
    buffer.clear ();
    buffer.resize (int (len));
    memset (buffer.data (), 0x00, len);
}

/*** CAN OPEN SDO TRANSFERS QUEUE ***/

CanOpenSdoTransferQueue::CanOpenSdoTransferQueue (const CanOpenNodeId nodeId, const CanOpenSdoMode mode)
    : timeout (1000)
    , nodeId (nodeId)
    , mode (mode)
    , timer (new QTimer)
    , currentTransfer (Q_NULLPTR)
{
    timer->setProperty ("nodeId", nodeId);
    timer->setTimerType (Qt::PreciseTimer);
    timer->setSingleShot (true);
}

void CanOpenSdoTransferQueue::killTimer (void) {
    if (timer->isActive ()) {
        timer->stop ();
    }
}

void CanOpenSdoTransferQueue::restartTimer (void) {
    killTimer ();
    timer->start (timeout);
}

/*** CAN OPEN PDO CONFIG/MAPPING CACHE ***/

CanOpenPdoConfigCache::CanOpenPdoConfigCache (void)
    : cobId (0x000)
    , role (CanOpenPdoRoles::PdoUnknown)
    , currentSyncCount (0)
    , transmitSyncCount (0)
    , currentFrameData (Q_NULLPTR, 8)
    , entryCommParams (Q_NULLPTR)
    , entryMapping (Q_NULLPTR)
{ }

void CanOpenPdoConfigCache::parseMappedVars (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR && entryMapping != Q_NULLPTR) {
        CanOpenPdoMapping mappingMux (quint32 (0x00000000));
        quint currentBitIdx = 0;
        const quint mappingsCount = entryMapping->getSubEntry (0x00)->readAs<quint8> ();
        for (quint mappingIdx = 0; mappingIdx < mappingsCount; mappingIdx++) {
            if (CanOpenSubEntry * subEntry = entryMapping->getSubEntry (CanOpenSubIndex (mappingIdx +1))) {
                mappingMux.raw = subEntry->readAs<quint32> ();
                if (mappingMux.raw > 0) {
                    if (mappingMux.fields.bitsCount > 0) {
                        if (CanOpenSubEntry * subEntryDest = obd->getSubEntry (mappingMux.fields.idx, mappingMux.fields.subIdx)) {
                            switch (subEntryDest->getDataType ()) {
                                case CanOpenDataTypes::Bool: {
                                    subEntryDest->writeAs<bool> (currentFrameData.getBitsAs<bool> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::Int8: {
                                    subEntryDest->writeAs<qint8> (currentFrameData.getBitsAs<qint8> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::UInt8: {
                                    subEntryDest->writeAs<quint8> (currentFrameData.getBitsAs<quint8> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::Int16: {
                                    subEntryDest->writeAs<qint16> (currentFrameData.getBitsAs<qint16> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::UInt16: {
                                    subEntryDest->writeAs<quint16> (currentFrameData.getBitsAs<quint16> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::Int32: {
                                    subEntryDest->writeAs<qint32> (currentFrameData.getBitsAs<qint32> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::UInt32: {
                                    subEntryDest->writeAs<quint32> (currentFrameData.getBitsAs<quint32> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::Int64: {
                                    subEntryDest->writeAs<qint64> (currentFrameData.getBitsAs<qint64> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                case CanOpenDataTypes::UInt64: {
                                    subEntryDest->writeAs<quint64> (currentFrameData.getBitsAs<quint64> (currentBitIdx, mappingMux.fields.bitsCount));
                                    break;
                                }
                                default: {
                                    qWarning () << "Can't map non scalar type !";
                                    break;
                                }
                            }
                        }
                        currentBitIdx += mappingMux.fields.bitsCount;
                    }
                }
            }
        }
    }
}

void CanOpenPdoConfigCache::collectMappedVars (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR && entryMapping != Q_NULLPTR) {
        CanOpenPdoMapping mappingMux (quint32 (0x00000000));
        quint currentBitIdx = 0;
        const quint mappingsCount = entryMapping->getSubEntry (0x00)->readAs<quint8> ();
        for (quint mappingIdx = 0; mappingIdx < mappingsCount; mappingIdx++) {
            CanOpenSubEntry * subEntry = entryMapping->getSubEntry (CanOpenSubIndex (mappingIdx +1));
            if (subEntry != Q_NULLPTR) {
                mappingMux.raw = subEntry->readAs<quint32> ();
                if (mappingMux.raw > 0) {
                    if (mappingMux.fields.bitsCount > 0) {
                        CanOpenSubEntry * subEntryDest = obd->getSubEntry (mappingMux.fields.idx, mappingMux.fields.subIdx);
                        if (subEntryDest != Q_NULLPTR) {
                            switch (subEntryDest->getDataType ()) {
                                case CanOpenDataTypes::Bool: {
                                    const bool tmp = subEntryDest->readAs<bool> ();
                                    currentFrameData.setBitsAs<bool> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::Int8: {
                                    const qint8 tmp = subEntryDest->readAs<qint8> ();
                                    currentFrameData.setBitsAs<qint8> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::UInt8: {
                                    const quint8 tmp = subEntryDest->readAs<quint8> ();
                                    currentFrameData.setBitsAs<quint8> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::Int16: {
                                    const qint16 tmp = subEntryDest->readAs<qint16> ();
                                    currentFrameData.setBitsAs<qint16> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::UInt16: {
                                    const quint16 tmp = subEntryDest->readAs<quint16> ();
                                    currentFrameData.setBitsAs<quint16> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::Int32: {
                                    const qint32 tmp = subEntryDest->readAs<qint32> ();
                                    currentFrameData.setBitsAs<qint32> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::UInt32: {
                                    const quint32 tmp = subEntryDest->readAs<quint32> ();
                                    currentFrameData.setBitsAs<quint32> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::Int64: {
                                    const qint64 tmp = subEntryDest->readAs<qint64> ();
                                    currentFrameData.setBitsAs<qint64> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                case CanOpenDataTypes::UInt64: {
                                    const quint64 tmp = subEntryDest->readAs<quint64> ();
                                    currentFrameData.setBitsAs<quint64> (currentBitIdx, mappingMux.fields.bitsCount, &tmp);
                                    break;
                                }
                                default: {
                                    qWarning () << "Can't map non scalar type !";
                                    break;
                                }
                            }
                        }
                        currentBitIdx += mappingMux.fields.bitsCount;
                    }
                }
            }
        }
    }
}

CanOpenLssAction::CanOpenLssAction (void)
    : state (CanOpenLssRequestStates::PendingSend)
    , cmd (CanOpenLssCmds::NoAction)
    , argument (0x00)
    , needsReply (false)
{ }

CanOpenLssQueue::CanOpenLssQueue (void)
    : timeout (1000)
    , timer (new QTimer)
    , currentAction (Q_NULLPTR)
{
    timer->setTimerType (Qt::PreciseTimer);
    timer->setSingleShot (true);
}

void CanOpenLssQueue::killTimer (void) {
    if (timer->isActive ()) {
        timer->stop ();
    }
}

void CanOpenLssQueue::restartTimer (void) {
    killTimer ();
    timer->start (timeout);
}

CanOpenHeartbeatConsumer::CanOpenHeartbeatConsumer (const CanOpenNodeId nodeId)
    : nodeId (nodeId)
    , state (CanOpenHeartBeatStates::Initializing)
    , watchdog ()
    , timeout (0x0000)
    , alive (false)
{ }
