#ifndef CANOPENDEFS_H
#define CANOPENDEFS_H

#include <QtGlobal>
#include <QObject>
#include <QQueue>
#include <QString>
#include <QStringBuilder>
#include <QTimer>

#include "QtCAN.h"
#include "CanMessage.h"

#ifdef Q_ENUM
#   define QML_ENUM Q_ENUM
#else
#   define QML_ENUM Q_ENUMS
#endif

#pragma pack(push,1)

typedef quint16      CanOpenIndex;
typedef quint8       CanOpenSubIndex;
typedef quint8       CanOpenNodeId;
typedef quint32      CanOpenCobId;
typedef unsigned int CanOpenDataLen;
typedef unsigned int CanOpenCounter;

static inline QString hex (long long num, int digits) {
    return (QStringLiteral ("0x") % QString::number (num, 16).rightJustified (digits, QLatin1Char ('0')));
}

class CanOpenEntry;
class CanOpenSubEntry;
class CanOpenObjDict;
class CanOpenProtocolManager;

#define QTCAN_CANOPEN_EXPORT

class QTCAN_CANOPEN_EXPORT CanOpenAccessModes {
    Q_GADGET

public:
    enum Enum {
        RW  = 0x0,
        WO  = 0x1,
        RO  = 0x2,
        CST = 0x3,
    };
    QML_ENUM (Enum)
};
typedef CanOpenAccessModes::Enum CanOpenAccessMode;

class QTCAN_CANOPEN_EXPORT CanOpenObjDictRanges {
    Q_GADGET

public:
    enum Enum {
        SDO_SERVERS_START   = 0x1200,
        SDO_SERVERS_END     = 0x127F,
        SDO_CLIENTS_START   = 0x1280,
        SDO_CLIENTS_END     = 0x12FF,
        PDORX_CONFIG_START  = 0x1400,
        PDORX_CONFIG_END    = 0x15FF,
        PDOTX_CONFIG_START  = 0x1800,
        PDOTX_CONFIG_END    = 0x19FF,
        PDORX_MAPPING_START = 0x1600,
        PDORX_MAPPING_END   = 0x17FF,
        PDOTX_MAPPING_START = 0x1A00,
        PDOTX_MAPPING_END   = 0x1BFF,
    };
    QML_ENUM (Enum)
};
typedef CanOpenObjDictRanges::Enum CanOpenObjDictRange;

class QTCAN_CANOPEN_EXPORT CanOpenHeartBeatStates {
    Q_GADGET

public:
    enum Enum {
        Initializing   = 0x00,
        Stopped        = 0x04,
        Operational    = 0x05,
        PreOperational = 0x7F,
    };
    QML_ENUM (Enum)
};
typedef CanOpenHeartBeatStates::Enum CanOpenHeartBeatState;

class QTCAN_CANOPEN_EXPORT CanOpenFctTypes {
    Q_GADGET

public:
    enum Enum {
        NMT       =  0,
        SYNC_EMCY =  1,
        TIME      =  2,
        PDOTX1    =  3,
        PDORX1    =  4,
        PDOTX2    =  5,
        PDORX2    =  6,
        PDOTX3    =  7,
        PDORX3    =  8,
        PDOTX4    =  9,
        PDORX4    = 10,
        SDOTX     = 11,
        SDORX     = 12,
        HB_NG     = 14,
        LSS       = 15,
    };
    QML_ENUM (Enum)
};
typedef CanOpenFctTypes::Enum CanOpenFctType;

class QTCAN_CANOPEN_EXPORT CanOpenPdoRoles {
    Q_GADGET

public:
    enum Enum {
        PdoUnknown,
        PdoTransmit,
        PdoReceive,
    };
    QML_ENUM (Enum)
};
typedef CanOpenPdoRoles::Enum CanOpenPdoRole;

class QTCAN_CANOPEN_EXPORT CanOpenSdoMethods {
    Q_GADGET

public:
    enum Enum {
        SdoUndecided,
        SdoExpedited,
        SdoSegmented,
        SdoBlocks,
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoMethods::Enum CanOpenSdoMethod;

class QTCAN_CANOPEN_EXPORT CanOpenSdoOperations {
    Q_GADGET

public:
    enum Enum {
        SdoNoOp,
        SdoRead,
        SdoWrite,
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoOperations::Enum CanOpenSdoOperation;

class QTCAN_CANOPEN_EXPORT CanOpenSdoModes {
    Q_GADGET

public:
    enum Enum {
        SdoNeiter,
        SdoClient, // the one who issues requests
        SdoServer, // the one who handles requests and issues replies
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoModes::Enum CanOpenSdoMode;

class QTCAN_CANOPEN_EXPORT CanOpenSdoCommStates {
    Q_GADGET

public:
    enum Enum {
        InvalidState,


        // write
        PendingWrite, // client wants to send write req

        /// EXP WRITE : > RequestInitDownload[IDX][SSIDX][DATA] < RequestWriteExp
        ///             > ReplyAcceptDownload[IDX][SSIDX] < ReplyWriteExp
        RequestWriteExpSent, // client sent exp write request (with data)
        RequestWriteExpRecv, // server recv exp write request (with data)
        ReplyWriteExpSent, // server sent exp write reply (ok)
        ReplyWriteExpRecv, // client recv exp write reply (ok)

        /// SEG WRITE : > RequestInitDownload[IDX][SSIDX][SIZE] < RequestWriteInitSeg
        ///             > ReplyAcceptDownload[IDX][SSIDX] < ReplyWriteInitSeg
        ///             [
        ///                 > RequestDownloadSegment[DATA] < RequestWriteDownloadSeg
        ///                 > ReplyDownloadSegment[] < ReplyWriteDownloadSeg
        ///             ] * (SIZE / 7)
        RequestWriteInitSegSent, // client sent init seg write request (ask init, with len)
        RequestWriteInitSegRecv, // server recv init seg write request (ask init, with len)
        ReplyWriteInitSegSent, // server sent seg init write reply (accept)
        ReplyWriteInitSegRecv, // client recv seg init write reply (accept)
        RequestWriteDownloadSegSent, // client sent download seg write request (with data chunk)
        RequestWriteDownloadSegRecv, // server recv download seg write request (with data chunk)
        ReplyWriteDownloadSegSent, // server sent download seg write reply (ack)
        ReplyWriteDownloadSegRecv, // client recv download seg write reply (ack)

        /// BLK WRITE : > RequestBlockDownload[CRC=0|1][IDX][SSIDX][SIZE?] < RequestWriteInitBlk
        ///             > ReplyBlockDownload[CRC=0|1][0<BLKSIZE<128] < ReplyWriteInitBlk
        ///             [
        ///                 [
        ///                     > RequestBlockDownload[IS_LAST][SEQNO][DATA] < RequestWriteDownloadBlk
        ///                 ] * BLKSIZE
        ///                 > ReplyBlockDownload[LAST_SEQNO][0<BLKSIZE<128] < ReplyWriteDownloadBlk
        ///             ]
        ///             > RequestBlockDownload[CRC?] < RequestWriteEndBlk
        ///             > ReplyBlockDownload[] < ReplyWriteEndBlk
        RequestWriteInitBlkSent, // client sent init blk write request (ask init, with CRC and len)
        RequestWriteInitBlkRecv, // server recv init blk write request (ask init, with CRC and len)
        ReplyWriteInitBlkSent, // server sent init blk write reply (accept, with CRC and blksize)
        ReplyWriteInitBlkRecv, // client recv init blk write reply (accept, with CRC and blksize)
        RequestWriteDownloadBlkSent, // client sent download blk write request (with data chunk)
        RequestWriteDownloadBlkRecv, // server recv download blk write request (with data chunk)
        ReplyWriteDownloadBlkSent, // server sent download blk write reply (ack, with last seqno, and next blksize)
        ReplyWriteDownloadBlkRecv, // client recv download blk write reply (ack, with last seqno, and next blksize)
        RequestWriteEndBlkSent, // client sent end blk write request (free bytes, and CRC)
        RequestWriteEndBlkRecv, // server sent end blk write request (free bytes, and CRC)
        ReplyWriteEndBlkSent, // server sent end blk write reply (ack)
        ReplyWriteEndBlkRecv, // client recv end blk write reply (ack)


        // read
        PendingRead, // client wants to send read req

        RequestReadExpOrSegSent, // client sent exp write request (ask init)
        RequestReadExpOrSegRecv, // server recv exp write request (ask init)

        /// EXP READ  : > RequestInitUpload[IDX][SSIDX] < RequestReadExpOrSeg
        ///             > ReplyAcceptUpload[IDX][SSIDX][DATA] < ReplyReadExp
        ReplyReadExpSent, // server sent exp write reply (ok, with data)
        ReplyReadExpRecv, // client recv exp write reply (ok, with data)

        /// SEG READ  : > RequestInitUpload[IDX][SSIDX] < RequestReadExpOrSeg
        ///             > ReplyAcceptUpload[IDX][SSIDX][SIZE] < ReplyReadInitSeg
        ///             [
        ///                 > RequestUploadSegment < RequestReadUploadSeg
        ///                 > ReplyUploadSegment[DATA] < ReplyReadUploadSeg
        ///             ] * (SIZE / 7)
        ReplyReadInitSegSent, // server sent seg init write reply (accept, with len)
        ReplyReadInitSegRecv, // client recv seg init write reply (accept, with len)
        RequestReadUploadSegSent, // client sent upload seg write request (ask next)
        RequestReadUploadSegRecv, // server recv upload seg write request (ask next)
        ReplyReadUploadSegSent, // server sent upload seg write reply (with data chunk)
        ReplyReadUploadSegRecv, // client recv upload seg write reply (with data chunk)

        /// BLK READ : > RequestBlockUpload[CRC=0|1][IDX][SSIDX][0<BLKSIZE<128][PST=0|1] < RequestReadInitBlk
        ///            > ReplyBlockUpload[CRC=0|1][SIZE?] < ReplyReadInitBlk
        ///            > RequestBlockUpload[] < RequestReadUploadBlk
        ///            [
        ///                [
        ///                    > ReplyBlockUpload[IS_LAST][SEQNO][DATA] < ReplyReadUploadBlk
        ///                ] * BLKSIZE
        ///                > RequestBlockUpload[LAST_SEQNO][0<BLKSIZE<128] < RequestReadAckBlk
        ///            ]
        ///            > ReplyBlockUpload[CRC?] < ReplyReadEndBlk
        ///            > RequestBlockUpload[] < RequestReadEndBlk
        RequestReadInitBlkSent, // client sent init blk read request (ask init, with CRC and blksize)
        RequestReadInitBlkRecv, // server recv init blk read request (ask init, with CRC and blksize)
        ReplyReadInitBlkSent, // server sent init blk read reply (accept, with CRC and len)
        ReplyReadInitBlkRecv, // client recv init blk read reply (accept, with CRC and len)
        RequestReadUploadBlkSent, // client sent upload blk read request
        RequestReadUploadBlkRecv, // server recv upload blk read request
        ReplyReadUploadBlkSent, // server sent upload blk read reply (seqno and data)
        ReplyReadUploadBlkRecv, // client recv upload blk read reply (seqno and data)
        RequestReadAckBlkSent, // client sent upload blk read ack (with last seqno and next blksize)
        RequestReadAckBlkRecv, // server recv upload blk read ack (with last seqno and next blksize)
        ReplyReadEndBlkSent, // server sent end blk read reply (with CRC)
        ReplyReadEndBlkRecv, // client recv end blk read reply (with CRC)
        RequestReadEndBlkSent, // client sent end blk read request (end)
        RequestReadEndBlkRecv, // server sent end blk read request (end)


        // abort
        RequestAbortSent, // client sent abort request
        RequestAbortRecv, // server recv abort request
        ReplyAbortSent, // server sent abort reply
        ReplyAbortRecv, // client recv abort reply
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoCommStates::Enum CanOpenSdoCommState;

class QTCAN_CANOPEN_EXPORT CanOpenSdoClientCmdSpecifs {
    Q_GADGET

public:
    enum Enum {
        RequestNone = -1,

        RequestInitDownload    = 1, // client request write, can also contain data for expedited write
        RequestInitUpload      = 2, // client request read

        RequestDownloadSegment = 0, // client sends next segment to write
        RequestUploadSegment   = 3, // client requests next segment to read

        RequestAbortTransfer   = 4, // client wants to abort transfer

        RequestBlockDownload   = 6, // client wants to write blocks, or finished writing blocks
        RequestBlockUpload     = 5, // client wants to read blocks, or finished reading blocks
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoClientCmdSpecifs::Enum CanOpenSdoClientCmdSpecif;

class QTCAN_CANOPEN_EXPORT CanOpenSdoServerCmdSpecifs {
    Q_GADGET

public:
    enum Enum {
        ReplyNone = -1,

        ReplyAcceptDownload  = 3, // server accepted expedited write, or start accepting segments for write
        ReplyAcceptUpload    = 2, // server sends data for expedited read, or start is ready to send segments for read

        ReplyDownloadSegment = 1, // server acknowledges written segment, and ask next one
        ReplyUploadSegment   = 0, // server provides next read segment

        ReplyAbortTransfer   = 4, // server refused the transfer

        ReplyBlockDownload   = 5, // server start accepting blocks to write
        ReplyBlockUpload     = 6, // server start sending blocks to read
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoServerCmdSpecifs::Enum CanOpenSdoServerCmdSpecif;

class QTCAN_CANOPEN_EXPORT CanOpenSdoAbortCodes {
    Q_GADGET

public:
    enum Enum {
        NoError                       = 0x00000000,
        ToggleBitNotAlternated        = 0x05030000,
        ProtocolTimeout               = 0x05040000,
        InvalidCommandSpecifier       = 0x05040001,
        InvalidBlockSize              = 0x05040002,
        InvalidSeqNo                  = 0x05040003,
        CrcMismatch                   = 0x05040004,
        UnsupportedObjectAccess       = 0x06010000,
        AttemptToReadObjectThatIsWO   = 0x06010001,
        AttemptToWriteObjectThatIsRO  = 0x06010002,
        ObjectDoesntExistInOBD        = 0x06020000,
        ObjectCanNotBeMappedToPDO     = 0x06040041,
        ObjectCountAndSizeOverflowPDO = 0x06040042,
        GeneralParamIncompatibility   = 0x06040043,
        GeneralDeviceIncompatibility  = 0x06040047,
        FailedBecauseOfHardwareIssue  = 0x06060000,
        DataSizeOrSizeDoesNotMatch    = 0x06060010,
        DataSizeOrSizeTooHigh         = 0x06060012,
        DataSizeOrSizeTooLow          = 0x06060013,
        SubIndexDoesNotExist          = 0x06090011,
        ValueRangeExceeded            = 0x06090030,
        ValueTooHigh                  = 0x06090031,
        ValueTooLow                   = 0x06090032,
        MaximumValueInferiorToMinimum = 0x06090036,
        GeneralError                  = 0x08000000,
        FailedToStoreOrTransferData   = 0x08000020,
        FailedBecauseOfLocalControl   = 0x08000021,
        FailedBecauseOfDeviceState    = 0x08000022,
        ObjectDictionaryIsNotPresent  = 0x08000023,
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoAbortCodes::Enum CanOpenSdoAbortCode;

class QTCAN_CANOPEN_EXPORT CanOpenNmtCmdSpecifs {
    Q_GADGET

public:
    enum Enum {
        StartNode = 0x01,
        StopNode  = 0x02,
        SetPreOp  = 0x80,
        ResetNode = 0x81,
        ResetComm = 0x82,
    };
    QML_ENUM (Enum)
};
typedef CanOpenNmtCmdSpecifs::Enum CanOpenNmtCmdSpecif;

class QTCAN_CANOPEN_EXPORT CanOpenLssCmds {
    Q_GADGET

public:
    enum Enum {
        NoAction      = 0x00,
        SwitchState   = 0x04,
        ChangeNodeId  = 0x11,
        ChangeBitrate = 0x13,
        StoreConfig   = 0x17,
    };
    QML_ENUM (Enum)
};
typedef CanOpenLssCmds::Enum CanOpenLssCmd;

class QTCAN_CANOPEN_EXPORT CanOpenLssRequestStates {
    Q_GADGET

public:
    enum Enum {
        PendingSend,
        RequestSent,
        ReplyUneeded,
        ReplyWaiting,
        ReplyReceived,
        ReplyError,
        ReplyTimeout,
    };
    QML_ENUM (Enum)
};
typedef CanOpenLssRequestStates::Enum CanOpenLssRequestState;

class QTCAN_CANOPEN_EXPORT CanOpenDataTypes {
    Q_GADGET

public:
    enum Enum {
        UnknownType  = 0x00,

        /*** types ***/
        Bool         = 0x01,
        Int8         = 0x02,
        Int16        = 0x03,
        Int32        = 0x04,
        UInt8        = 0x05,
        UInt16       = 0x06,
        UInt32       = 0x07,
        Real32       = 0x08,
        VisibleStr   = 0x09,
        OctetStr     = 0x0A,
        UnicodeStr   = 0x0B,
        TimeOfDay    = 0x0C,
        TimeDiff     = 0x0D,
        Domain       = 0x0F,
        Int24        = 0x10,
        Real64       = 0x11,
        Int40        = 0x12,
        Int48        = 0x13,
        Int56        = 0x14,
        Int64        = 0x15,
        UInt24       = 0x16,
        UInt40       = 0x18,
        UInt48       = 0x19,
        UInt56       = 0x1A,
        UInt64       = 0x1B,

        /***  structs ***/
        PdoCommParam = 0x20,
        PdoMapping   = 0x21,
        SdoParameter = 0x22,
        Identity     = 0x23,
    };
    QML_ENUM (Enum)

    static CanOpenDataLen sizeOfType (const Enum type, const CanOpenDataLen fallback = 0);

    static bool isTypeString (const Enum type);
};
typedef CanOpenDataTypes::Enum CanOpenDataType;

class QTCAN_CANOPEN_EXPORT CanOpenObjTypes {
    Q_GADGET

public:
    enum Enum {
        Undefined = 0x0,
        DefType   = 0x5,
        DefStruct = 0x6,
        Var       = 0x7, // first sub-index is the only one used, with specific type
        Array     = 0x8, // first sub-index is UINT8 array item count, and other sub-indexes have identical types
        Record    = 0x9, // first sub-index is UINT8 struct members count, and other sub-indexes have various types
    };
    QML_ENUM (Enum)
};
typedef CanOpenObjTypes::Enum CanOpenObjType;

class QTCAN_CANOPEN_EXPORT CanOpenNetPositions {
    Q_GADGET

public:
    enum Enum {
        Slave,
        Master,
    };
    QML_ENUM (Enum)
};
typedef CanOpenNetPositions::Enum CanOpenNetPosition;

class QTCAN_CANOPEN_EXPORT CanOpenSdoCobIds {
    Q_GADGET

public:
    enum Enum {
        CliRequest = 0x600,
        SrvReply   = 0x580,
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoCobIds::Enum CanOpenSdoCobId;

class QTCAN_CANOPEN_EXPORT CanOpenSdoConfSubIndexes {
    Q_GADGET

public:
    enum Enum {
        CobIdCliRequest = 0x01,
        CobIdSrvReply   = 0x02,
        NodeId          = 0x03,
    };
    QML_ENUM (Enum)
};
typedef CanOpenSdoConfSubIndexes::Enum CanOpenSdoConfSubIndex;

class QTCAN_CANOPEN_EXPORT CanOpenPdoConfSubIndexes {
    Q_GADGET

public:
    enum Enum {
        CobId       = 0x01,
        Type        = 0x02,
        InhibitTime = 0x03,
        Reserved    = 0x04,
        EventTimer  = 0x05,
    };
    QML_ENUM (Enum)
};
typedef CanOpenPdoConfSubIndexes::Enum CanOpenPdoConfSubIndex;

struct QTCAN_CANOPEN_EXPORT CanOpenObdPos {
private:
    struct Fields {
        quint16 idx;
        quint8  subIdx;
        quint8  nodeId;
    };

public:
    explicit CanOpenObdPos (const quint32         raw = 0x00000000);
    explicit CanOpenObdPos (const CanOpenIndex    idx,
                            const CanOpenSubIndex subIdx,
                            const CanOpenNodeId   nodeId);
    union {
        quint32 raw;
        Fields  fields;
    };
};

struct QTCAN_CANOPEN_EXPORT CanOpenPdoMapping {
private:
    struct Fields {
        quint8          bitsCount;
        CanOpenSubIndex subIdx;
        CanOpenIndex    idx;
    };

public:
    explicit CanOpenPdoMapping (const quint32         raw = 0x00000000);
    explicit CanOpenPdoMapping (const CanOpenIndex    idx,
                                const CanOpenSubIndex subIdx,
                                const quint8          bitsCount);

    union {
        quint32 raw;
        Fields  fields;
    };
};

struct QTCAN_CANOPEN_EXPORT CanOpenHeartbeatConsumer {
public:
    const CanOpenNodeId nodeId;
    CanOpenHeartBeatState state;
    QBasicTimer watchdog;
    quint16 timeout;
    bool alive;

    explicit CanOpenHeartbeatConsumer (const CanOpenNodeId nodeId);
};

struct QTCAN_CANOPEN_EXPORT CanOpenSdoFrame {
public:
    static const CanOpenDataLen SDO_FRAME_SIZE     = 8;
    static const CanOpenDataLen SDO_SEGMENT_SIZE   = 7;
    static const CanOpenDataLen SDO_EXPEDITED_SIZE = 4;

private:
    struct Multiplex {
        CanOpenIndex idx;
        CanOpenSubIndex subIdx;
        union {
            quint32 dataRaw;
            quint8  dataBytes [SDO_EXPEDITED_SIZE];
        };
    };

public:
    explicit CanOpenSdoFrame (void);

    quint8 commandSpecifier;
    union {
        Multiplex mutiplexed;
        qbyte     segmentedBytes [SDO_SEGMENT_SIZE];
    };

    void setError (const CanOpenIndex    idx,
                   const CanOpenSubIndex ssidx,
                   const quint32         errorCode = 0x00);
};

struct QTCAN_CANOPEN_EXPORT CanOpenSdoTransfer {
    explicit CanOpenSdoTransfer (void);

    CanOpenSdoOperation operation;
    CanOpenSdoMode mode;
    CanOpenSdoMethod method;
    CanOpenSdoCommState commState;
    CanOpenSdoAbortCode errState;
    CanOpenNodeId nodeId;
    CanOpenIndex idx;
    CanOpenSubIndex subIdx;
    CanOpenDataLen expectedSize;
    CanOpenDataLen currentSize;
    CanOpenCounter seqNo;
    CanOpenCounter blkSize;
    bool isLast;
    bool toggleBit;
    bool finished;
    bool sendFrame;
    bool useCrc;
    quint16 checksum;
    QByteArray buffer;

    void createBuffer (const CanOpenDataLen len);
};

struct QTCAN_CANOPEN_EXPORT CanOpenSdoTransferQueue {
    explicit CanOpenSdoTransferQueue (const CanOpenNodeId nodeId = 0x00,
                                      const CanOpenSdoMode mode = CanOpenSdoModes::SdoNeiter);

    int timeout;
    CanOpenNodeId nodeId;
    CanOpenSdoMode mode;
    QTimer * timer;
    CanOpenSdoTransfer * currentTransfer;
    QQueue<CanOpenSdoTransfer *> transfersQueue;

    void killTimer    (void);
    void restartTimer (void);
};

struct QTCAN_CANOPEN_EXPORT CanOpenLssAction {
    explicit CanOpenLssAction (void);

    CanOpenLssRequestState state;
    CanOpenLssCmd cmd;
    quint8 argument;
    bool needsReply;
};

struct QTCAN_CANOPEN_EXPORT CanOpenLssQueue {
    explicit CanOpenLssQueue (void);

    int timeout;
    QTimer * timer;
    CanOpenLssAction * currentAction;
    QQueue<CanOpenLssAction *> actionsQueue;

    void killTimer    (void);
    void restartTimer (void);
};

struct QTCAN_CANOPEN_EXPORT CanOpenPdoConfigCache {
    explicit CanOpenPdoConfigCache (void);

    CanOpenCobId cobId;
    CanOpenPdoRole role;
    CanOpenCounter currentSyncCount;
    CanOpenCounter transmitSyncCount;
    CanData currentFrameData;
    CanOpenEntry * entryCommParams;
    CanOpenEntry * entryMapping;

    void parseMappedVars   (CanOpenObjDict * obd);
    void collectMappedVars (CanOpenObjDict * obd);
};

#pragma pack(pop)

#endif // CANOPENDEFS_H
