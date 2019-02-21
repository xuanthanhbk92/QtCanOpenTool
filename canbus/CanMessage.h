#ifndef CANMESSAGE_H
#define CANMESSAGE_H

#include <QByteArray>
#include <QDateTime>
#include <QVariant>
#include <QtGlobal>

#include "QtCAN.h"

#pragma pack(push, 1)

class QTCAN_BASE_EXPORT CanId {
public:
    explicit CanId (const quint32 canIdEFF, const bool isRTR = false, const bool isERR = false);
    explicit CanId (const quint16 canIdSFF, const bool isRTR = false, const bool isERR = false);

    static const quint16 MASK_SFF = 0x7FFu;      // Standard CAN identifier (11 bit)
    static const quint32 MASK_EFF = 0x1FFFFFFFu; // Extended CAN identifier (29 bit)

    static const quint32 FLAG_ERR = 0x20000000u; // error frame flag (0 = data frame, 1 = err frame)
    static const quint32 FLAG_RTR = 0x40000000u; // remote transmission request flag (0 = normal frame, 1 = rtr frame)
    static const quint32 FLAG_EFF = 0x80000000u; // frame format flag (0 = standard 11 bit, 1 = extended 29 bit)

    bool isEFF (void) const;
    bool isRTR (void) const;
    bool isERR (void) const;

    quint16 canIdSFF (void) const;
    quint32 canIdEFF (void) const;

private:
    quint32 m_canId;
};

template<int SIZE> class FixedBuffer {
public:
    explicit FixedBuffer (const void * ptr = Q_NULLPTR, const quint length = 0)
        : m_length (length)
    {
        for (quint pos = 0; pos < SIZE; ++pos) {
            m_bytes [pos] = 0x00;
        }
        if (ptr != Q_NULLPTR && m_length > 0) {
            qbyteptr bytes = qbyteptr (ptr);
            const quint loops = (m_length < SIZE ? m_length : SIZE);
            for (quint pos = 0; pos < loops; ++pos) {
                m_bytes [pos] = bytes [pos];
            }
        }
    }

    quint getLength (void) const {
        return m_length;
    }

    const void * getDataPtr (void) const {
        return &m_bytes [0];
    }

    QByteArray toByteArray (void) const {
        return QByteArray (reinterpret_cast<const char *> (&m_bytes [0]), static_cast<int> (m_length));
    }

    void replaceWith (const void * ptr, const quint length) {
        if (ptr != Q_NULLPTR && length > 0 && length <= SIZE) {
            const qbyte * tmp = reinterpret_cast<const qbyte *> (ptr);
            for (quint pos = 0; pos < length; ++pos) {
                m_bytes [pos] = tmp [pos];
            }
        }
    }

    qbyte getByte (const quint bytePos, bool * ok = Q_NULLPTR) const {
        qbyte ret ('\0');
        const bool check = (bytePos < m_length);
        if (check) {
            ret = m_bytes [bytePos];
        }
        if (ok != Q_NULLPTR) {
            *ok = check;
        }
        return ret;
    }

    QString getBytesAsString (const quint bytePos, const quint bytesCount = SIZE) const {
        QString ret;
        if (bytePos + bytesCount <= m_length) {
            ret = QString::fromLatin1 (reinterpret_cast<const char *> (&m_bytes [bytePos]), bytesCount);
        }
        return ret;
    }

    static inline void copyBit (const qbyte srcBytes [], qbyte dstBytes [], const quint srcBitPos, const quint dstBitPos) {
        if ((srcBytes [srcBitPos / 8] >> (srcBitPos % 8)) & 0x1) {
            dstBytes [dstBitPos / 8] |= (0x1 << (dstBitPos % 8)); // NOTE : set bit
        }
        else {
            dstBytes [dstBitPos / 8] &= ~(0x1 << (dstBitPos % 8)); // NOTE : unset bit
        }
    }

    template<typename IntType> IntType getSingleBitAs (const quint bitPos, bool * ok = Q_NULLPTR) const {
        IntType ret = 0;
        const bool noReadPastEnd = (bitPos < m_length * 8);
        if (noReadPastEnd) {
            ret = ((m_bytes [bitPos / 8] >> (bitPos % 8)) & 0x1);
        }
        if (ok != Q_NULLPTR) {
            *ok = noReadPastEnd;
        }
        return ret;
    }

    template<typename IntType> IntType getAlignedBitsAs (const quint bitPos, const quint bitsCount = (sizeof (IntType) * 8), bool * ok = Q_NULLPTR) const {
        IntType ret = 0;
        const bool noReadPastEnd = (bitPos + bitsCount <= m_length * 8);
        const bool noOverflowRet = (bitsCount <= sizeof (ret) * 8);
        const bool aligned = ((bitPos % 8 == 0) && (bitsCount % 8 == 0));
        const bool check = (noReadPastEnd && noOverflowRet && aligned);
        if (check) {
            const quint byteFirst  = (bitPos    / 8);
            const quint bytesCount = (bitsCount / 8);
            for (quint byteOffset = 0; byteOffset < bytesCount; ++byteOffset) {
                reinterpret_cast<qbyte *> (&ret) [byteOffset] = m_bytes [byteFirst + byteOffset];
            }
        }
        if (ok != Q_NULLPTR) {
            *ok = check;
        }
        return ret;
    }

    template<typename IntType> IntType getUnalignedBitsAs (const quint bitPos, const quint bitsCount = (sizeof (IntType) * 8), bool * ok = Q_NULLPTR) const {
        IntType ret = 0;
        const bool noReadPastEnd = (bitPos + bitsCount <= m_length * 8);
        const bool noOverflowRet = (bitsCount <= sizeof (ret) * 8);
        const bool check = (noReadPastEnd && noOverflowRet);
        if (check) {
            for (quint srcBitPos = bitPos, dstBitPos = 0; dstBitPos < bitsCount; ++srcBitPos, ++dstBitPos) {
                copyBit (&m_bytes [0], qbyteptr (&ret), srcBitPos, dstBitPos);
            }
        }
        if (ok != Q_NULLPTR) {
            *ok = check;
        }
        return ret;
    }

    template<typename IntType> IntType getBitsAs (const quint bitPos,
                                                  const quint bitsCount = (sizeof (IntType) * 8),
                                                  bool * ok = Q_NULLPTR) const {
        IntType ret = 0;
        if (bitsCount == 1) {
           ret = getSingleBitAs<IntType> (bitPos, ok);
        }
        else if ((bitPos % 8 == 0) && (bitsCount % 8 == 0)) {
            ret = getAlignedBitsAs<IntType> (bitPos, bitsCount, ok);
        }
        else {
            ret = getUnalignedBitsAs<IntType> (bitPos, bitsCount, ok);
        }
        return ret;
    }

    template<typename IntType> bool setBitsAs (const quint bitPos,
                                               const quint bitsCount = (sizeof (IntType) * 8),
                                               const void * bytes = Q_NULLPTR) {
        const bool noWritePastEnd = (bitPos + bitsCount <= m_length * 8);
        const bool noOverflowSrc  = (bitsCount <= sizeof (IntType) * 8);
        const bool noInvalidData  = (bytes != Q_NULLPTR);
        bool ret = (noWritePastEnd && noOverflowSrc && noInvalidData);
        if (ret) {
            for (quint srcBitPos = 0, dstBitPos = bitPos; srcBitPos < bitsCount; ++srcBitPos, ++dstBitPos) {
                copyBit (qbyteptr (bytes), &m_bytes [0], srcBitPos, dstBitPos);
            }
            ret = true;
        }
        return ret;
    }

    template<typename IntType> bool setBitsWith (const quint bitPos,
                                                 const quint bitsCount = (sizeof (IntType) * 8),
                                                 const IntType & value = 0) {
        return setBitsAs<IntType> (bitPos, bitsCount, &value);
    }

    void clear (void) {
        for (quint pos = 0; pos < SIZE; ++pos) {
            m_bytes [pos] = 0x00;
        }
    }

private:
    quint m_length;
    qbyte m_bytes [SIZE];
};

class QTCAN_BASE_EXPORT CanData : public FixedBuffer<8> {
public:
    explicit CanData (const qvoidptr ptr = Q_NULLPTR, const quint length = 0);

    static const quint MAX_BYTES = 8;
    static const quint MAX_BITS  = 64;
};

class QTCAN_BASE_EXPORT CanMessage {
public:
    explicit CanMessage (const CanId & getCanId,
                         const quint length = 0,
                         const qvoidptr ptr = Q_NULLPTR,
                         const QDateTime & timestamp = QDateTime::currentDateTimeUtc ());

    void setCanId   (const CanId   & canId);
    void setCanData (const CanData & canData);

    const CanId     & getCanId     (void) const;
    const CanData   & getCanData   (void) const;
    const QDateTime & getTimeStamp (void) const;

private:
    CanId     m_canId;
    CanData   m_canData;
    QDateTime m_timestamp;
};

#pragma pack(pop)

#endif // CANMESSAGE_H
