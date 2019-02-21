#ifndef CANDRIVERQMLWRAPPER_H
#define CANDRIVERQMLWRAPPER_H

#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QVariantMap>
#include <QQmlEngine>
#include <QtGlobal>
#include <QtCAN.h>
#include <QMap>
class CanDriver;
class CanMessage;
class CanDriverOption;

#define QTCAN_UTILS_EXPORT Q_DECL_EXPORT

class QTCAN_UTILS_EXPORT CanDriverWrapper : public QObject {
    Q_OBJECT
    Q_PROPERTY (bool         driverLoaded  READ getDriverLoaded  NOTIFY driverLoadedChanged)
    Q_PROPERTY (QString      driverName    READ getDriverName    NOTIFY driverNameChanged)
    Q_PROPERTY (QVariantList driverOptions READ getDriverOptions NOTIFY driverOptionsChanged)
    Q_PROPERTY (QStringList  driversList   READ getDriversList   CONSTANT)

public:
    explicit CanDriverWrapper (QObject * parent = Q_NULLPTR);
    virtual ~CanDriverWrapper (void);

    static void registerQmlTypes (QQmlEngine * engine);

    bool         getDriverLoaded  (void) const;
    QString      getDriverName    (void) const;
    QVariantList getDriverOptions (void) const;
    QStringList  getDriversList   (void) const;
    CanDriver *  getDriverObject  (void) const;

    void sendCanMsg (CanMessage * message);

public slots:
    void selectDriver (const QString     & name);
    void loadDriver   (const QVariantMap & options);

signals:
    void driverNameChanged    (void);
    void driverOptionsChanged (void);
    void driverLoadedChanged  (void);
    void diagLogRequested     (const QString & type, const QString & details);
    void canMsgRecv           (CanMessage * message);
    void canMsgSent           (CanMessage * message);

protected slots:
    void onCanMsgRecv (CanMessage * message);
    void onDiag       (int level, const QString & description);

private:
    QList<CanDriverOption *> driverOptionRequire(QString driverName);
    CanDriver * loadDriver(QString driverName);
    bool         m_driverLoaded;
    QString      m_driverName;
    QVariantList m_driverOptions;
    QStringList  m_driversList;
    CanDriver *  m_canDriver;

    QMap<QString, CanDriverPlugin *> m_DriverMap;

    CanDriverPlugin * m_tcpJsonDriver;
};

#endif // CANDRIVERQMLWRAPPER_H
