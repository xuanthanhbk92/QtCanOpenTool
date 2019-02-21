#ifndef QMLCANOPENDATALAYER_H
#define QMLCANOPENDATALAYER_H

#include <QObject>
#include <QVector>
#include <QQmlListProperty>

#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenDataLayer : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (QString name READ getName WRITE setName NOTIFY nameChanged)
    Q_PROPERTY (QQmlListProperty<AbstractObdPreparator> content READ getContent)
    Q_CLASSINFO ("DefaultProperty", "content")

public:
    explicit QmlCanOpenDataLayer (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    const QString & getName (void) const;

    QQmlListProperty<AbstractObdPreparator> getContent (void);

public slots:
    void setName (const QString & name);

signals:
    void nameChanged (void);

private:
    QString m_name;
    QmlListWrapper<AbstractObdPreparator> m_content;
};

#endif // QMLCANOPENDATALAYER_H
