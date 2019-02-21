#ifndef CANOPENSUBENTRY_H
#define CANOPENSUBENTRY_H

#include <QObject>
#include <QVariant>

#include "CanOpenDefs.h"

class QTCAN_CANOPEN_EXPORT CanOpenSubEntry : public QObject {
    Q_OBJECT

public:
    explicit CanOpenSubEntry (const CanOpenDataType dataType,
                              const CanOpenAccessMode accessMode,
                              const CanOpenDataLen dataLen = 0,
                              QObject * parent = Q_NULLPTR);
    virtual ~CanOpenSubEntry (void);

    bool isReadable      (void) const;
    bool isWritable      (void) const;
    bool haveBeenWritten (void) const;

    CanOpenDataLen    getDataLen    (void) const;
    CanOpenDataType   getDataType   (void) const;
    CanOpenAccessMode getAttributes (void) const;

    void read  (qvoidptr dstPtr, const quint dstSize) const;

    void write (const qvoidptr srcPtr, const quint srcSize);

    template<typename T> T readAs (void) const {
        T ret;
        read (&ret, sizeof (ret));
        return ret;
    }

    template<typename T> void writeAs (const T val) {
        write (&val, sizeof (val));
    }

    QVariant   readToQtVariant   (void) const;
    QByteArray readToQtByteArray (void) const;

    void writeFromQtVariant   (const QVariant   & var);
    void writeFromQtByteArray (const QByteArray & bytes);

    void resetWrittenFlag (void);

signals:
    void dataChanged (void);

private:
    bool m_haveBeenWritten;
    CanOpenDataLen m_dataLen;
    CanOpenDataType m_dataType;
    CanOpenAccessMode m_accessMode;
    qbyteptr m_dataPtr;
};

#endif // CANOPENSUBENTRY_H
