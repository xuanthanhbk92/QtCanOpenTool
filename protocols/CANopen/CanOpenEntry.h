#ifndef CANOPENENTRY_H
#define CANOPENENTRY_H

#include <QObject>
#include <QList>
#include <QMap>

#include "CanOpenDefs.h"

#include "SparseArray.h"

class CanOpenSubEntry;
class CanOpenObjDict;

class QTCAN_CANOPEN_EXPORT CanOpenEntry : public QObject {
    Q_OBJECT

public:
    explicit CanOpenEntry (const CanOpenObjType objectType,
                           QObject * parent = Q_NULLPTR);

    CanOpenObjType getObjectType (void) const;

    void addSubEntry (CanOpenSubEntry * subEntry, const CanOpenSubIndex subIdx);

    bool hasSubEntry (const CanOpenSubIndex subIdx) const;

    CanOpenSubEntry * getSubEntry (const CanOpenSubIndex subIdx) const;

private:
    CanOpenObjType m_objectType;
    SparseArray<CanOpenSubIndex, CanOpenSubEntry *> m_subEntriesBySubIndex;

    friend class CanOpenObjDict;
};

#endif // CANOPENENTRY_H
