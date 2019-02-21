#ifndef QMLCANOPENPDOMAPPEDVAR_H
#define QMLCANOPENPDOMAPPEDVAR_H

#include <QObject>

#include "CanOpenDefs.h"

class AbstractObdPreparator;

class QTCAN_CANOPEN_EXPORT QmlCanOpenPdoMappedVar : public QObject {
    Q_OBJECT
    Q_PROPERTY (int index     READ getIndex     WRITE setIndex     NOTIFY indexChanged)
    Q_PROPERTY (int subIndex  READ getSubIndex  WRITE setSubIndex  NOTIFY subIndexChanged)
    Q_PROPERTY (int bitsCount READ getBitsCount WRITE setBitsCount NOTIFY bitsCountChanged)

public:
    explicit QmlCanOpenPdoMappedVar (QObject * parent = Q_NULLPTR);

    int getIndex     (void) const;
    int getSubIndex  (void) const;
    int getBitsCount (void) const;

    quint32 toMapping (void) const;

public slots:
    void setIndex     (const int index);
    void setSubIndex  (const int subIndex);
    void setBitsCount (const int bitsCount);

signals:
    void indexChanged     (void);
    void subIndexChanged  (void);
    void bitsCountChanged (void);

private:
    CanOpenPdoMapping m_mapping;
    AbstractObdPreparator * m_preparator;
};

#endif // QMLCANOPENPDOMAPPEDVAR_H
