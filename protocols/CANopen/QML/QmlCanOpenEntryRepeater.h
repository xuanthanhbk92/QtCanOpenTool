#ifndef QMLCANOPENENTRYREPEATER_H
#define QMLCANOPENENTRYREPEATER_H

#include <QObject>
#include <QQmlComponent>
#include <QQmlIncubator>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenEntryRepeater : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int             indexFirst    READ getIndexFirst    WRITE setIndexFirst    NOTIFY indexFirstChanged)
    Q_PROPERTY (int             indexLast     READ getIndexLast     WRITE setIndexLast     NOTIFY indexLastChanged)
    Q_PROPERTY (QVariantList    indexes       READ getIndexes       WRITE setIndexes       NOTIFY indexesChanged)
    Q_PROPERTY (QQmlComponent * entryTemplate READ getEntryTemplate WRITE setEntryTemplate NOTIFY entryTemplateChanged)

public:
    explicit QmlCanOpenEntryRepeater (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int             getIndexFirst    (void) const;
    int             getIndexLast     (void) const;
    QQmlComponent * getEntryTemplate (void) const;
    QVariantList    getIndexes       (void) const;

    class Incubator : public QQmlIncubator {
    public:
        explicit Incubator (QQmlIncubator::IncubationMode mode, QmlCanOpenEntryRepeater * repeater)
            : QQmlIncubator (mode)
            , currentNum (1)
            , currentIndex (0)
            , repeater (repeater)
        { }

        void setInitialState (QObject * object);

        void prepareEntry (CanOpenObjDict * obd, QQmlComponent * entryTemplate, const CanOpenIndex idx, const quint num);

        quint currentNum;
        CanOpenIndex currentIndex;
        QmlCanOpenEntryRepeater * repeater;
    };

public slots:
    void setIndexFirst    (const int indexFirst);
    void setIndexLast     (const int indexFirst);
    void setIndexes       (const QVariantList & indexes);
    void setEntryTemplate (QQmlComponent * objectTemplate);

signals:
    void indexFirstChanged    (void);
    void indexLastChanged     (void);
    void entryTemplateChanged (void);
    void indexesChanged       (void);

private:
    CanOpenIndex m_indexFirst;
    CanOpenIndex m_indexLast;
    QVector<CanOpenIndex> m_indexes;
    QQmlComponent * m_entryTemplate;
};

#endif // QMLCANOPENENTRYREPEATER_H
