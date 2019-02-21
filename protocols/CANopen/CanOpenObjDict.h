#ifndef CANOPENOBJDICT_H
#define CANOPENOBJDICT_H

#include <QObject>
#include <QVariant>
#include <QString>
#include <QList>
#include <QMap>

#include "CanOpenDefs.h"

#include "SparseArray.h"

class QTCAN_CANOPEN_EXPORT CanOpenObjDict : public QObject {
    Q_OBJECT

public:
    explicit CanOpenObjDict (QObject * parent = Q_NULLPTR);

    void addEntry (CanOpenEntry * entry, const CanOpenIndex idx);

    bool hasEntry    (const CanOpenIndex idx) const;
    bool hasSubEntry (const CanOpenIndex idx, const CanOpenSubIndex subIdx) const;

    CanOpenEntry    * getEntry    (const CanOpenIndex idx) const;
    CanOpenSubEntry * getSubEntry (const CanOpenIndex idx, const CanOpenSubIndex subIdx) const;

    void     setMetaDataForObdPos (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const QString & metaKey, const QVariant & metaData);
    QVariant getMetaDataForObdPos (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const QString & metaKey) const;

public slots:
    void traverseObd (void);

signals:
    void beginObd      (CanOpenObjDict * obd);
    void beginEntry    (const CanOpenIndex    idx,    CanOpenEntry    * entry);
    void beginSubEntry (const CanOpenSubIndex subIdx, CanOpenSubEntry * subEntry);
    void endSubEntry   (void);
    void endEntry      (void);
    void endObd        (void);

protected:
    void createVarEntry          (const CanOpenIndex idx, const CanOpenDataType dataType, const CanOpenDataLen dataLen = 0);
    void createArrayEntry        (const CanOpenIndex idx, const CanOpenSubIndex count, const CanOpenDataType dataType, const CanOpenDataLen dataLen = 0);
    void createSdoConfigEntry    (const CanOpenIndex idx);
    void createPdoCommParamEntry (const CanOpenIndex idx);
    void createPdoMappingEntry   (const CanOpenIndex idx);

private:
    SparseArray<CanOpenIndex, CanOpenEntry *> m_entriesByIndex;
};

#endif // CANOPENOBJDICT_H
