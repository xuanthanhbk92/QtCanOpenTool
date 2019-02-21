#ifndef QMLCANOPENVALUEMODIFIER_H
#define QMLCANOPENVALUEMODIFIER_H

#include <QObject>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenValueModifier : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int      index    READ getIndex    WRITE setIndex    NOTIFY indexChanged)
    Q_PROPERTY (int      subIndex READ getSubIndex WRITE setSubIndex NOTIFY subIndexChanged)
    Q_PROPERTY (QVariant data     READ getData     WRITE setData     NOTIFY dataChanged)

public:
    explicit QmlCanOpenValueModifier (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int      getIndex    (void) const;
    int      getSubIndex (void) const;
    QVariant getData     (void) const;

public slots:
    void setIndex    (const int index);
    void setSubIndex (const int subIndex);
    void setData     (const QVariant & data);

signals:
    void indexChanged    (void);
    void subIndexChanged (void);
    void dataChanged     (void);

private:
    CanOpenIndex m_index;
    CanOpenSubIndex m_subIndex;
    QVariant m_data;
};

#endif // QMLCANOPENVALUEMODIFIER_H
