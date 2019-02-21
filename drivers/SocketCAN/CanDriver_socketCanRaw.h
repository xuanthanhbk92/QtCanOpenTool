#ifndef CANDRIVER_LINUXSOCKETCANRAW_H
#define CANDRIVER_LINUXSOCKETCANRAW_H

#include <QObject>
#include <QSocketNotifier>

#include "QtCAN.h"

#include "CanDriver.h"

#define QTCAN_DRIVER_EXPORT

class QTCAN_DRIVER_EXPORT CanDriver_socketCan : public CanDriver {
    Q_OBJECT

public:
    explicit CanDriver_socketCan (QObject * parent = Q_NULLPTR);
    ~CanDriver_socketCan (void);

    static const QString BUS_NAME;
    static const QString USE_LOOPBACK;
    static const QString RCV_OWN_MSG;
    static const QString TX_BUFFER_SIZE;
    static const QString RX_BUFFER_SIZE;

public slots:
    bool init (const QVariantMap & options);
    bool send (CanMessage * message);
    bool stop (void);

protected slots:
    void poll (void);

private:
    int               m_sockfd;
    bool              m_valid;
    QString           m_iface;
    QSocketNotifier * m_poller;
};

#ifndef QTCAN_STATIC_DRIVERS

class QTCAN_DRIVER_EXPORT CanDriverPlugin_socketCanRaw : public QObject, public CanDriverPlugin {
    Q_OBJECT
    Q_INTERFACES (CanDriverPlugin)
    Q_PLUGIN_METADATA (IID "QtCAN.CanDriverPlugin")

public:
    QString getDriverName (void);
    CanDriver * createDriverInstance (QObject * parent = Q_NULLPTR);
    QList<CanDriverOption *> optionsRequired (void);
};

#endif

#endif // CANDRIVER_LINUXSOCKETCANRAW_H
