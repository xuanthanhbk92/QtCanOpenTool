#ifndef CANDRIVER_SERIAL_H
#define CANDRIVER_SERIAL_H


#include "QtCAN.h"

#include "CanDriver.h"
#include "CanMessage.h"

#include <QtSerialPort/QSerialPort>

#define QTCAN_DRIVER_EXPORT

class QTCAN_DRIVER_EXPORT CanDriver_Serial : public CanDriver {
    Q_OBJECT

public:
    explicit CanDriver_Serial (QObject * parent = Q_NULLPTR);

    static const QString PORT;
    static const QString HOST;
    static const QString SERV;

public slots: // CanDriver interface
    bool init (const QVariantMap & options);
    bool send (CanMessage * message);
    bool stop (void);

private slots:
    void onReadyRead    (void);


private:
    QByteArray            m_buffer;
    QSerialPort * m_serialPort;
};

#ifndef QTCAN_STATIC_DRIVERS

class QTCAN_DRIVER_EXPORT CanDriverPlugin_Serial : public QObject, public CanDriverPlugin {
    Q_OBJECT
    Q_INTERFACES (CanDriverPlugin)
 //   Q_PLUGIN_METADATA (IID "QtCAN.CanDriverPlugin")

public:
    QString getDriverName (void);
    CanDriver * createDriverInstance (QObject * parent = Q_NULLPTR);
    QList<CanDriverOption *> optionsRequired (void);
};

#endif

#endif // CANDRIVER_JSONTCP_H
