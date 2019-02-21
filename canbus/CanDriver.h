#ifndef CANDRIVER_H
#define CANDRIVER_H

#include <QObject>
#include <QString>
#include <QVariant>
#include <QList>
#include <QMap>

#include "QtCAN.h"

class QTCAN_BASE_EXPORT CanDriverOption : public QObject {
    Q_OBJECT
    Q_ENUMS (OptType)
    Q_PROPERTY (int          type     READ getType     CONSTANT)
    Q_PROPERTY (QString      key      READ getKey      CONSTANT)
    Q_PROPERTY (QString      label    READ getLabel    CONSTANT)
    Q_PROPERTY (QVariant     fallback READ getFallback CONSTANT)
    Q_PROPERTY (QVariantList list     READ getList     CONSTANT)

public:
    enum OptType { Invalid, IPv4Address, SocketPort, BaudRate, NameString, BooleanFlag, ListChoice };

    explicit CanDriverOption (const QString & key = QString (),
                              const QString & label = QString (),
                              const OptType type = Invalid,
                              const QVariant & fallback = QVariant (),
                              const QVariantList & list = QVariantList (),
                              QObject * parent = Q_NULLPTR)
        : QObject    (parent)
        , m_type     (type)
        , m_key      (key)
        , m_label    (label)
        , m_fallback (fallback)
        , m_list     (list)
    { }

    int          getType     (void) const { return m_type;     }
    QString      getKey      (void) const { return m_key;      }
    QString      getLabel    (void) const { return m_label;    }
    QVariant     getFallback (void) const { return m_fallback; }
    QVariantList getList     (void) const { return m_list;     }

private:
    OptType      m_type;
    QString      m_key;
    QString      m_label;
    QVariant     m_fallback;
    QVariantList m_list;
};

class QTCAN_BASE_EXPORT CanDriver : public QObject {
    Q_OBJECT
    Q_ENUMS (DiagnosticLevel)

public:
    explicit CanDriver (QObject * parent = Q_NULLPTR) : QObject (parent) { }

    enum DiagnosticLevel {
        Trace       = -2,
        Debug       = -1,
        Information =  0,
        Warning     =  1,
        Error       =  2,
    };

public slots:
    virtual bool init (const QVariantMap & options) = 0;
    virtual bool send (CanMessage * message) = 0;
    virtual bool stop (void) = 0;
    virtual QVariantMap info (void) { return QVariantMap (); }

signals:
    void recv (CanMessage * message);
    void diag (int level, const QString & description);
};

class QTCAN_BASE_EXPORT CanDriverPlugin {
public:
    virtual QString getDriverName (void) = 0;
    virtual CanDriver * createDriverInstance (QObject * parent = Q_NULLPTR) = 0;
    virtual QList<CanDriverOption *> optionsRequired (void) = 0;
};

Q_DECLARE_INTERFACE (CanDriverPlugin, "QtCAN.CanDriverPlugin")

#endif // CANDRIVER_H
