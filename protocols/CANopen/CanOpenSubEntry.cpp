
#include "CanOpenSubEntry.h"

CanOpenSubEntry::CanOpenSubEntry (const CanOpenDataType   dataType,
                                  const CanOpenAccessMode accessMode,
                                  const CanOpenDataLen    dataLen,
                                  QObject * parent)
    : QObject       (parent)
    , m_haveBeenWritten (false)
    , m_dataLen     (CanOpenDataTypes::sizeOfType (dataType, dataLen))
    , m_dataType    (dataType)
    , m_accessMode  (accessMode)
    , m_dataPtr     (m_dataLen > 0 ? new qbyte [m_dataLen] : Q_NULLPTR)
{
    for (quint pos = 0; pos < m_dataLen; ++pos) {
        m_dataPtr [pos] = 0x00;
    }
}

CanOpenSubEntry::~CanOpenSubEntry (void) {
    delete [] m_dataPtr;
}

bool CanOpenSubEntry::isReadable (void) const {
    return (m_accessMode != CanOpenAccessModes::WO);
}

bool CanOpenSubEntry::isWritable (void) const {
    return (m_accessMode != CanOpenAccessModes::RO);
}

bool CanOpenSubEntry::haveBeenWritten (void) const {
    return m_haveBeenWritten;
}

void CanOpenSubEntry::resetWrittenFlag (void) {
    m_haveBeenWritten = false;
}

CanOpenDataLen CanOpenSubEntry::getDataLen (void) const {
    return m_dataLen;
}

CanOpenDataType CanOpenSubEntry::getDataType (void) const {
    return m_dataType;
}

CanOpenAccessMode CanOpenSubEntry::getAttributes (void) const {
    return m_accessMode;
}

void CanOpenSubEntry::read (qvoidptr dstPtr, const quint dstSize) const {
    if (dstPtr != Q_NULLPTR && m_dataPtr != Q_NULLPTR) {
        qbyteptr dstBytes = qbyteptr (dstPtr);
        const qbyteptr ownBytes = qbyteptr (m_dataPtr);
        const quint ownSize = m_dataLen;
        for (quint pos = 0; pos < dstSize && pos < ownSize; ++pos) {
            dstBytes [pos] = ownBytes [pos];
        }
    }
}

void CanOpenSubEntry::write (const qvoidptr srcPtr, const quint srcSize) {
    if (srcPtr != Q_NULLPTR && m_dataPtr != Q_NULLPTR) {
        m_haveBeenWritten = true;
        const qbyteptr srcBytes = qbyteptr (srcPtr);
        qbyteptr ownBytes = qbyteptr (m_dataPtr);
        const quint ownSize = m_dataLen;
        bool changed = false;
        for (quint pos = 0; pos < srcSize && pos < ownSize; ++pos) {
            if (ownBytes [pos] != srcBytes [pos]) {
                ownBytes [pos] = srcBytes [pos];
                changed = true;
            }
        }
        if (changed) {
            emit dataChanged ();
        }
    }
}

QVariant CanOpenSubEntry::readToQtVariant (void) const {
    switch (m_dataType) {
        case CanOpenDataTypes::Bool: {
            return QVariant::fromValue (bool (* reinterpret_cast<bool *> (m_dataPtr)));
        }
        case CanOpenDataTypes::UInt8: {
            return QVariant::fromValue (int (* reinterpret_cast<quint8 *> (m_dataPtr)));
        }
        case CanOpenDataTypes::UInt16: {
            return QVariant::fromValue (int (* reinterpret_cast<quint16 *> (m_dataPtr)));
        }
        case CanOpenDataTypes::UInt32: {
            return QVariant::fromValue (double (* reinterpret_cast<quint32 *> (m_dataPtr))); /// because QML int can't go over 31 bits
        }
        case CanOpenDataTypes::UInt64: {
            return QVariant::fromValue (double (* reinterpret_cast<quint64 *> (m_dataPtr))); /// because QML int can't go over 31 bits
        }
        case CanOpenDataTypes::Int8: {
            return QVariant::fromValue (int (* reinterpret_cast<qint8 *> (m_dataPtr)));
        }
        case CanOpenDataTypes::Int16: {
            return QVariant::fromValue (int (* reinterpret_cast<qint16 *> (m_dataPtr)));
        }
        case CanOpenDataTypes::Int32: {
            return QVariant::fromValue (int (* reinterpret_cast<qint32 *> (m_dataPtr)));
        }
        case CanOpenDataTypes::Int64: {
            return QVariant::fromValue (double (* reinterpret_cast<qint64 *> (m_dataPtr))); /// because QML int can't go over 31 bits
        }
        case CanOpenDataTypes::Real32: {
            return QVariant::fromValue (float (* reinterpret_cast<float *> (m_dataPtr)));
        }
        case CanOpenDataTypes::Real64: {
            return QVariant::fromValue (double (* reinterpret_cast<double *> (m_dataPtr)));
        }
        case CanOpenDataTypes::VisibleStr: {
            return QVariant::fromValue (QString::fromLatin1 (reinterpret_cast<char *> (m_dataPtr)).left (int (m_dataLen)));
        }
        case CanOpenDataTypes::OctetStr: {
            return QVariant::fromValue (QString::fromLocal8Bit (reinterpret_cast<char *> (m_dataPtr)).left (int (m_dataLen)));
        }
        case CanOpenDataTypes::UnicodeStr: {
            return QVariant::fromValue (QString::fromUtf8 (reinterpret_cast<char *> (m_dataPtr)).left (int (m_dataLen)));
        }
        case CanOpenDataTypes::Domain: {
            return QVariant::fromValue (QString::fromLatin1 (QByteArray (reinterpret_cast<char *> (m_dataPtr), int (m_dataLen)).toHex ()));
        }
        case CanOpenDataTypes::Int24:
        case CanOpenDataTypes::Int40:
        case CanOpenDataTypes::Int48:
        case CanOpenDataTypes::Int56:
        case CanOpenDataTypes::UInt24:
        case CanOpenDataTypes::UInt40:
        case CanOpenDataTypes::UInt48:
        case CanOpenDataTypes::UInt56:
        case CanOpenDataTypes::TimeOfDay:
        case CanOpenDataTypes::TimeDiff:
        case CanOpenDataTypes::PdoCommParam:
        case CanOpenDataTypes::PdoMapping:
        case CanOpenDataTypes::SdoParameter:
        case CanOpenDataTypes::Identity:
        case CanOpenDataTypes::UnknownType:
            return QVariant ();
    }
    return QVariant ();
}

QByteArray CanOpenSubEntry::readToQtByteArray (void) const {
    QByteArray ret (int (m_dataLen), 0x00);
    read (ret.data (), quint (ret.size ()));
    return ret;
}

void CanOpenSubEntry::writeFromQtVariant (const QVariant & var) {
    switch (m_dataType) {
        case CanOpenDataTypes::Bool: {
            const bool tmp = var.value<bool> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::UInt8: {
            const quint8 tmp = var.value<quint8> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::UInt16: {
            const quint16 tmp = var.value<quint16> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::UInt32: {
            const quint32 tmp = var.value<quint32> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::UInt64: {
            const quint64 tmp = var.value<quint64> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Int8: {
            const qint8 tmp = var.value<qint8> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Int16: {
            const qint16 tmp = var.value<qint16> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Int32: {
            const qint32 tmp = var.value<qint32> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Int64: {
            const qint64 tmp = var.value<qint64> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Real32: {
            const float tmp = var.value<float> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::Real64: {
            const double tmp = var.value<double> ();
            write (&tmp, sizeof (tmp));
            break;
        }
        case CanOpenDataTypes::VisibleStr: {
            const QByteArray tmp = var.value<QString> ().toLatin1 ().leftJustified (int (m_dataLen), '\0', true);
            write (tmp.data (), quint (tmp.size ()));
            break;
        }
        case CanOpenDataTypes::OctetStr: {
            const QByteArray tmp = var.value<QString> ().toLocal8Bit ().leftJustified (int (m_dataLen), '\0', true);
            write (tmp.data (), quint (tmp.size ()));
            break;
        }
        case CanOpenDataTypes::UnicodeStr: {
            const QByteArray tmp = var.value<QString> ().toUtf8 ().leftJustified (int (m_dataLen), '\0', true);
            write (tmp.data (), quint (tmp.size ()));
            break;
        }
        case CanOpenDataTypes::Domain: {
            const QByteArray tmp = QByteArray::fromHex (var.value<QString> ().toLatin1 ()).leftJustified (int (m_dataLen) * 2, '\0', true);
            write (tmp.data (), quint (tmp.size ()));
            break;
        }
        case CanOpenDataTypes::Int24:
        case CanOpenDataTypes::Int40:
        case CanOpenDataTypes::Int48:
        case CanOpenDataTypes::Int56:
        case CanOpenDataTypes::UInt24:
        case CanOpenDataTypes::UInt40:
        case CanOpenDataTypes::UInt48:
        case CanOpenDataTypes::UInt56:
        case CanOpenDataTypes::TimeOfDay:
        case CanOpenDataTypes::TimeDiff:
        case CanOpenDataTypes::PdoCommParam:
        case CanOpenDataTypes::PdoMapping:
        case CanOpenDataTypes::SdoParameter:
        case CanOpenDataTypes::Identity:
        case CanOpenDataTypes::UnknownType:
            break;
    }
}

void CanOpenSubEntry::writeFromQtByteArray (const QByteArray & bytes) {
    write (bytes.constData (), quint (bytes.size ()));
}
