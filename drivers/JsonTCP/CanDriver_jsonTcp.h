#ifndef CANDRIVER_JSONTCP_H
#define CANDRIVER_JSONTCP_H

#include "QtCAN.h"

#include "CanDriver.h"
#include "CanMessage.h"

#include <QTcpSocket>
#include <QTcpServer>

#define QTCAN_DRIVER_EXPORT

class QTCAN_DRIVER_EXPORT CanDriver_jsonTcp : public CanDriver {
    Q_OBJECT

public:
    explicit CanDriver_jsonTcp (QObject * parent = Q_NULLPTR);

    static const QString PORT;
    static const QString HOST;
    static const QString SERV;

public slots: // CanDriver interface
    bool init (const QVariantMap & options);
    bool send (CanMessage * message);
    bool stop (void);

private slots:
    void onSocketReadyRead    (void);
    void onClientConnected    (void);
    void onClientSentData     (void);
    void onClientDisconnected (void);

private:
    QByteArray            m_buffer;
    QTcpSocket          * m_socket;
    QTcpServer          * m_server;
    QList<QTcpSocket *>   m_clients;
};

#ifndef QTCAN_STATIC_DRIVERS

class QTCAN_DRIVER_EXPORT CanDriverPlugin_jsonTcp : public QObject, public CanDriverPlugin {
    Q_OBJECT
    Q_INTERFACES (CanDriverPlugin)
    Q_PLUGIN_METADATA (IID "QtCAN.CanDriverPlugin")

public:
    QString getDriverName (void);
    CanDriver * createDriverInstance (QObject * parent = Q_NULLPTR);
    QList<CanDriverOption *> optionsRequired (void);
};

#endif

#endif // CANDRIVER_JSONTCP_H
