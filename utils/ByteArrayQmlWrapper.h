#ifndef BYTEARRAYQMLWRAPPER_H
#define BYTEARRAYQMLWRAPPER_H

#include <QObject>
#include <QByteArray>
#include <QString>

#include "TypesQmlWrapper.h"

class QTCAN_UTILS_EXPORT ByteArrayWrapper : public QObject {
    Q_OBJECT

public:
    explicit ByteArrayWrapper (QObject * parent = Q_NULLPTR);

    const QByteArray & getBytes (void) const;

    void setBytes (const QByteArray & bytes);

    Q_INVOKABLE bool toBool (void) const;

    Q_INVOKABLE QmlBiggestInt toInt8   (void) const;
    Q_INVOKABLE QmlBiggestInt toInt16  (void) const;
    Q_INVOKABLE QmlBiggestInt toInt32  (void) const;
    Q_INVOKABLE QmlBiggestInt toUInt8  (void) const;
    Q_INVOKABLE QmlBiggestInt toUInt16 (void) const;
    Q_INVOKABLE QmlBiggestInt toUInt32 (void) const;

    Q_INVOKABLE QString toAscii (void) const;
    Q_INVOKABLE QString toHex    (void) const;

    template<typename IntType> QmlBiggestInt toInt (void) const {
        IntType ret = 0;
        if (m_bytes.size () == sizeof (IntType)) {
            for (unsigned int byte = 0; byte < sizeof (IntType); ++byte) {
                reinterpret_cast<char *> (&ret) [byte] = m_bytes.at (byte);
            }
        }
        return QmlBiggestInt (ret);
    }
    template<typename IntType> void fromInt (const IntType value) {
        m_bytes.clear ();
        m_bytes.reserve (sizeof (IntType));
        for (unsigned int byte = 0; byte < sizeof (IntType); ++byte) {
            m_bytes.append (reinterpret_cast<const char *> (&value) [byte]);
        }
    }

private:
    QByteArray m_bytes;
};

#endif // BYTEARRAYQMLWRAPPER_H
