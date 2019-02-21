
#include "ByteArrayQmlWrapper.h"

ByteArrayWrapper::ByteArrayWrapper (QObject * parent)
    : QObject (parent)
{ }

const QByteArray & ByteArrayWrapper::getBytes (void) const {
    return m_bytes;
}

void ByteArrayWrapper::setBytes (const QByteArray & bytes) {
    m_bytes = bytes;
}

bool ByteArrayWrapper::toBool (void) const {
    return toInt<bool> ();
}

QmlBiggestInt ByteArrayWrapper::toInt8 (void) const {
    return toInt<qint8> ();
}

QmlBiggestInt ByteArrayWrapper::toInt16 (void) const {
    return toInt<qint16> ();
}

QmlBiggestInt ByteArrayWrapper::toInt32 (void) const {
    return toInt<qint32> ();
}

QmlBiggestInt ByteArrayWrapper::toUInt8 (void) const {
    return toInt<quint8> ();
}

QmlBiggestInt ByteArrayWrapper::toUInt16 (void) const {
    return toInt<quint16> ();
}

QmlBiggestInt ByteArrayWrapper::toUInt32 (void) const {
    return toInt<quint32> ();
}

QString ByteArrayWrapper::toAscii (void) const {
    return QString::fromLatin1 (m_bytes).trimmed ();
}

QString ByteArrayWrapper::toHex (void) const {
    return QString::fromLatin1 (m_bytes.toHex ());
}
