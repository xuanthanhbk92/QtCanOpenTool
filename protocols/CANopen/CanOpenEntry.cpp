
#include "CanOpenEntry.h"

#include "CanOpenSubEntry.h"

CanOpenEntry::CanOpenEntry (const CanOpenObjType objectType, QObject * parent)
    : QObject      (parent)
    , m_objectType (objectType)
{ }

CanOpenObjType CanOpenEntry::getObjectType (void) const {
    return m_objectType;
}

void CanOpenEntry::addSubEntry (CanOpenSubEntry * subEntry, const CanOpenSubIndex subIdx) {
    if (!m_subEntriesBySubIndex.contains (subIdx)) {
        m_subEntriesBySubIndex.insert (subIdx, subEntry);
    }
}

bool CanOpenEntry::hasSubEntry (const CanOpenSubIndex subIdx) const {
    return m_subEntriesBySubIndex.contains (subIdx);
}

CanOpenSubEntry * CanOpenEntry::getSubEntry (const CanOpenSubIndex subIdx) const {
    return m_subEntriesBySubIndex.value (subIdx, Q_NULLPTR);
}
