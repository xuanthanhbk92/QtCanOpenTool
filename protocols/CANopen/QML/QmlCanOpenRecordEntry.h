#ifndef QMLCANOPENRECORDENTRY_H
#define QMLCANOPENRECORDENTRY_H

#include <QObject>
#include <QVector>
#include <QQmlListProperty>

#include "AbstractObdPreparator.h"
#include "QmlCanOpenSubEntry.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenRecordEntry : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int num   READ getNum   WRITE setNum   NOTIFY numChanged)
    Q_PROPERTY (int index READ getIndex WRITE setIndex NOTIFY indexChanged)
    Q_PROPERTY (QQmlListProperty<QmlCanOpenSubEntry> subEntries READ getSubEntries)
    Q_CLASSINFO ("DefaultProperty", "subEntries")

public:
    explicit QmlCanOpenRecordEntry (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getNum   (void) const;
    int getIndex (void) const;

    QQmlListProperty<QmlCanOpenSubEntry> getSubEntries (void);

public slots:
    void setNum   (const int num);
    void setIndex (const int index);

signals:
    void numChanged   (void);
    void indexChanged (void);

private:
    CanOpenCounter m_num;
    CanOpenIndex m_index;
    QmlListWrapper<QmlCanOpenSubEntry> m_subEntries;
};

#endif // QMLCANOPENRECORDENTRY_H
