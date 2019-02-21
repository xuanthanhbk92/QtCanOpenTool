#ifndef CANDATAQMLWRAPPER_H
#define CANDATAQMLWRAPPER_H

#include <QObject>

#include "CanMessage.h"

#include "TypesQmlWrapper.h"

class QTCAN_UTILS_EXPORT CanDataWrapperRx : public QObject {
    Q_OBJECT

public:
    explicit CanDataWrapperRx (QObject * parent = Q_NULLPTR);

    void setCanData (const CanData & canData);

public slots:
    QmlBiggestInt getBitsAs (int type, int pos, int count);

    QString getBytesAsAscii (int pos = 0, int len = 8);
    QString getBytesAsHex   (int pos = 0, int len = 8);

private:
    CanData m_canData;
};

class QTCAN_UTILS_EXPORT CanDataWrapperTx : public QObject {
    Q_OBJECT

public:
    explicit CanDataWrapperTx (QObject * parent = Q_NULLPTR);

    const CanData & getCanData (void) const;

public slots:
    void prepare   (int length);
    void setBitsAs (int type, int pos, int count, QmlBiggestInt value);
    void send      (int canId);

signals:
    void produced (CanMessage * msg);

private:
    quint m_canDataLength;
    CanData m_canData;
};

#endif // CANDATAQMLWRAPPER_H
