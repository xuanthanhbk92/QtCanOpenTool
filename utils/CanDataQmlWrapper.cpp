
#include "CanDataQmlWrapper.h"

CanDataWrapperRx::CanDataWrapperRx (QObject * parent)
    : QObject (parent)
    , m_canData (Q_NULLPTR, CanData::MAX_BYTES)
{ }

void CanDataWrapperRx::setCanData (const CanData & canData) {
    m_canData = canData;
}

QmlBiggestInt CanDataWrapperRx::getBitsAs (int type, int pos, int count) {
    switch (VarType (type)) {
        case VarTypes::Int8:   return QmlBiggestInt (m_canData.getBitsAs<qint8>   (quint (pos), quint (count)));
        case VarTypes::UInt8:  return QmlBiggestInt (m_canData.getBitsAs<quint8>  (quint (pos), quint (count)));
        case VarTypes::Int16:  return QmlBiggestInt (m_canData.getBitsAs<qint16>  (quint (pos), quint (count)));
        case VarTypes::UInt16: return QmlBiggestInt (m_canData.getBitsAs<quint16> (quint (pos), quint (count)));
        case VarTypes::Int32:  return QmlBiggestInt (m_canData.getBitsAs<qint32>  (quint (pos), quint (count)));
        case VarTypes::UInt32: return QmlBiggestInt (m_canData.getBitsAs<quint32> (quint (pos), quint (count)));
#ifndef NO_64_BIT_IN_QML
        case VarTypes::Int64:  return QmlBiggestInt (m_canData.getBitsAs<qint64>  (quint (pos), quint (count)));
        case VarTypes::UInt64: return QmlBiggestInt (m_canData.getBitsAs<quint64> (quint (pos), quint (count)));
#endif
        case VarTypes::Unknown: break;
    }
    return 0;
}

QString CanDataWrapperRx::getBytesAsAscii (int pos, int len) {
    return m_canData.getBytesAsString (quint (pos), quint (len));
}

QString CanDataWrapperRx::getBytesAsHex (int pos, int len) {
    return QString::fromLatin1 (m_canData.toByteArray ().mid (pos, len).toHex ());
}

CanDataWrapperTx::CanDataWrapperTx (QObject * parent)
    : QObject (parent)
    , m_canDataLength (0)
    , m_canData (Q_NULLPTR, CanData::MAX_BYTES)
{ }

const CanData & CanDataWrapperTx::getCanData (void) const {
    return m_canData;
}

void CanDataWrapperTx::prepare (int length) {
    m_canData.clear ();
    if (quint (length) <= CanData::MAX_BYTES) {
        m_canDataLength = quint (length);
    }
    else {
        m_canDataLength = 0;
        qWarning () <<  "Wrong CAN msg size ! MAX=" << CanData::MAX_BYTES;
    }
}

void CanDataWrapperTx::setBitsAs (int type, int pos, int count, QmlBiggestInt value) {
    switch (VarType (type)) {
        case VarTypes::Int8: {
            m_canData.setBitsWith<qint8> (quint (pos), quint (count), qint8 (value));
            break;
        }
        case VarTypes::UInt8: {
            m_canData.setBitsWith<quint8> (quint (pos), quint (count), quint8 (value));
            break;
        }
        case VarTypes::Int16: {
            m_canData.setBitsWith<qint16> (quint (pos), quint (count), qint16 (value));
            break;
        }
        case VarTypes::UInt16: {
            m_canData.setBitsWith<quint16> (quint (pos), quint (count), quint16 (value));
            break;
        }
        case VarTypes::Int32: {
            m_canData.setBitsWith<qint32> (quint (pos), quint (count), qint32 (value));
            break;
        }
        case VarTypes::UInt32: {
            m_canData.setBitsWith<quint32> (quint (pos), quint (count), quint32 (value));
            break;
        }
#ifndef NO_64_BIT_IN_QML
        case VarTypes::Int64: {
            m_canData.setBitsAs<qint64> (quint (pos), quint (count), qint64 (value));
            break;
        }
        case VarTypes::UInt64: {
            m_canData.setBitsAs<quint64> (quint (pos), quint (count), quint64 (value));
            break;
        }
#endif
        case VarTypes::Unknown: break;
    }
}

void CanDataWrapperTx::send (int canId) {
    emit produced (new CanMessage (CanId (quint16 (canId)), m_canDataLength, m_canData.getDataPtr ()));
}
