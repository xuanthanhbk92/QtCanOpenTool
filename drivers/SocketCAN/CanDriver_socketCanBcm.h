#ifndef CANDRIVER_LINUXSOCKETCANBCM_H
#define CANDRIVER_LINUXSOCKETCANBCM_H

#include <QObject>
#include <QSocketNotifier>

#include "QtCAN.h"

#include "CanDriver.h"

struct SocketCanBcmFrame;

#define QTCAN_DRIVER_EXPORT

class QTCAN_DRIVER_EXPORT CanDriver_socketCanBcm : public CanDriver {
    Q_OBJECT

public:
    explicit CanDriver_socketCanBcm (QObject * parent = Q_NULLPTR);
    ~CanDriver_socketCanBcm (void);

    static const QString BUS_NAME;
    static const QString THROTTLED_CANIDS;
    static const QString WHITELISTED_CANIDS;

public slots:
    bool init (const QVariantMap & options);
    bool send (CanMessage * message);
    bool stop (void);

protected slots:
    void poll (void);

protected:
    bool sendBcmMsg (SocketCanBcmFrame * msg);

private:
    int               m_sockfd;
    bool              m_valid;
    QString           m_iface;
    QSocketNotifier * m_poller;
};

#ifndef QTCAN_STATIC_DRIVERS

class QTCAN_DRIVER_EXPORT CanDriverPlugin_socketCanBcm : public QObject, public CanDriverPlugin {
    Q_OBJECT
    Q_INTERFACES (CanDriverPlugin)
    Q_PLUGIN_METADATA (IID "QtCAN.CanDriverPlugin")

public:
    QString getDriverName (void);
    CanDriver * createDriverInstance (QObject * parent = Q_NULLPTR);
    QList<CanDriverOption *> optionsRequired (void);
};

#endif

#endif // CANDRIVER_LINUXSOCKETCANBCM_H
