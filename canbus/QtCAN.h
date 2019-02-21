#ifndef QTCANOPEN_H
#define QTCANOPEN_H

#include <QtGlobal>
#include <QString>
#include <QByteArray>
#include <QDateTime>
#include <QVariant>
#include <QDebug>

#define qvoidptr void *

typedef bool               qbool;
typedef unsigned char      qbyte;
typedef float              qreal32;
typedef double             qreal64;
typedef unsigned char *    qbyteptr;
typedef unsigned int       quint;
typedef unsigned long long qraw64;

class QtCAN;
class CanId;
class CanData;
class CanMessage;
class CanDriver;
class CanDriverPlugin;
class CanDriverOption;
class CanDriverLoader;

#define QTCAN_BASE_EXPORT Q_DECL_EXPORT

class QTCAN_BASE_EXPORT QtCAN {
public:
    static QString bitsAsStr (qraw64 bits) {
        return QString::number (bits, 2);
    }

    static QByteArray timestampStr (void) {
        return QDateTime::currentDateTimeUtc ().toString ("hh:mm:ss.zzz").toLocal8Bit ();
    }

    static QByteArray hexNumberStr (qint64 number, quint bytes) {
        return QStringLiteral ("0x%1").arg (number, int (bytes * 2), 16, QChar::fromLatin1 ('0')).toLocal8Bit ();
    }

    static QByteArray binNumberStr (qint64 bytes) {
        return QStringLiteral ("%1").arg (bytes, 0, 2).toLocal8Bit ();
    }

    static QByteArray binDataAsHex (const void * ptr, const int count) {
        static const char * HEX = "000102030405060708090A0B0C0D0E0F101112131415161718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F606162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F808182838485868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCDCECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F2F3F4F5F6F7F8F9FAFBFCFDFEFF";
        QByteArray ret (int (count * 3 -1), char (' '));
        const unsigned char * bytes = reinterpret_cast<const unsigned char *> (ptr);
        for (int pos = 0; pos < count; ++pos) {
            ret [(pos * 3) +0] = HEX [(bytes [pos] * 2) +0];
            ret [(pos * 3) +1] = HEX [(bytes [pos] * 2) +1];
        }
        return ret;
    }

    static QByteArray binDataAsHex (const QByteArray & data) {
        return binDataAsHex (data.constData (), data.size ());
    }
};

#ifndef QTCAN_NO_CUSTOM_LOG

#   define INFO qDebug    () << QtCAN::timestampStr ().constData () << "[INFO]"
#   define DUMP qDebug    () << QtCAN::timestampStr ().constData () << "[DUMP]"
#   define WARN qWarning  () << QtCAN::timestampStr ().constData () << "[WARN]"
#   define ERRO qCritical () << QtCAN::timestampStr ().constData () << "[ERRO]"

#endif

#endif // QTCANOPEN_H
