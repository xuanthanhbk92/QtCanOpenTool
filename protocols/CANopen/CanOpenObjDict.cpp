
#include "CanOpenObjDict.h"

#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"

CanOpenObjDict::CanOpenObjDict (QObject * parent) : QObject (parent) {
    /// prepare mandatory objects of communication profile
    createVarEntry (0x1000, CanOpenDataTypes::UInt32); // NOTE : device type
    createVarEntry (0x1001, CanOpenDataTypes::UInt8); // NOTE : error register
    // TODO : 0x1002 : manufacturer status register
    // TODO : 0x1003 : predefined error field
    // NOTE : 0x1004 : not used
    // TODO : 0x1005 : COBID SYNC
    createVarEntry (0x1006, CanOpenDataTypes::UInt16); // NOTE : communication cycle period SYNC
    // TODO : 0x1007 : synchronous window length
    createVarEntry (0x1008, CanOpenDataTypes::VisibleStr, 4); // NOTE : manufacturer device name
    //createVarEntry (0x1009, CanOpenDataTypes::VisibleStr, 4); // NOTE : manufacturer hardware version
    //createVarEntry (0x100A, CanOpenDataTypes::VisibleStr, 4); // NOTE : manufacturer software version
    // NOTE : 0x100B : not used
    createVarEntry (0x100C, CanOpenDataTypes::UInt16); // NOTE : guard time
    createVarEntry (0x100D, CanOpenDataTypes::UInt8); // NOTE : life time factor
    // NOTE : 0x100E - 0x100F : not used
    // TODO : 0x1010 : store parameters
    // TODO : 0x1011 : restore default parameters
    // TODO : 0x1012 : COBID TIME
    // TODO : 0x1013 : high resolutin timestamp
    // TODO : 0x1014 : COBID EMCY
    // TODO : 0x1015 : inhibit time EMCY
    createArrayEntry (0x1016, 127, CanOpenDataTypes::UInt32); // NOTE : consumer heartbeat time
    createVarEntry (0x1017, CanOpenDataTypes::UInt16); // NOTE : producer heartbeat time
    // TODO : 0x1018 : identity object
    // NOTE : 0x1019 - 0x11FF : not used
    for (CanOpenIndex idx = CanOpenObjDictRanges::SDO_SERVERS_START; idx < CanOpenObjDictRanges::SDO_SERVERS_END; ++idx) {
        createSdoConfigEntry (idx);
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::SDO_CLIENTS_START; idx < CanOpenObjDictRanges::SDO_CLIENTS_END; ++idx) {
        createSdoConfigEntry (idx);
    }
    // NOTE : 0x1300 - 0x13FF : not used
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDORX_CONFIG_START; idx < CanOpenObjDictRanges::PDORX_CONFIG_END; ++idx) {
        createPdoCommParamEntry (idx);
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDORX_MAPPING_START; idx < CanOpenObjDictRanges::PDORX_MAPPING_END; ++idx) {
        createPdoMappingEntry (idx);
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDOTX_CONFIG_START; idx < CanOpenObjDictRanges::PDOTX_CONFIG_END; ++idx) {
        createPdoCommParamEntry (idx);
    }
    for (CanOpenIndex idx = CanOpenObjDictRanges::PDOTX_MAPPING_START; idx < CanOpenObjDictRanges::PDOTX_MAPPING_END; ++idx) {
        createPdoMappingEntry (idx);
    }
    // NOTE : 0x1C00 - 0x1FFF : not used
}

void CanOpenObjDict::createVarEntry (const CanOpenIndex idx, const CanOpenDataType dataType, CanOpenDataLen dataLen) {
    CanOpenEntry * ret = new CanOpenEntry (CanOpenObjTypes::Var, this);
    ret->addSubEntry (new CanOpenSubEntry (dataType, CanOpenAccessModes::RW, dataLen, ret), 0x00);
    addEntry (ret, idx);
}

void CanOpenObjDict::createArrayEntry (const CanOpenIndex idx, const CanOpenSubIndex count, const CanOpenDataType dataType, const CanOpenDataLen dataLen) {
    CanOpenEntry * entry = new CanOpenEntry (CanOpenObjTypes::Array, this);
    CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RO, 0, entry);
    subEntryCount->writeAs<quint8> (0x03);
    for (CanOpenSubIndex subIdx = 0x01; subIdx <= count; ++subIdx) {
        entry->addSubEntry (new CanOpenSubEntry (dataType, CanOpenAccessModes::RW, dataLen, entry), subIdx);
    }
    addEntry (entry, idx);
}

void CanOpenObjDict::createSdoConfigEntry (const CanOpenIndex idx) {
    CanOpenEntry * ret = new CanOpenEntry (CanOpenObjTypes::Record, this);
    CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RO, 0, ret);
    subEntryCount->writeAs<quint8> (0x03);
    ret->addSubEntry (subEntryCount, 0x00);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt32, CanOpenAccessModes::RW, 0, ret), CanOpenSdoConfSubIndexes::CobIdCliRequest);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt32, CanOpenAccessModes::RW, 0, ret), CanOpenSdoConfSubIndexes::CobIdSrvReply);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt8,  CanOpenAccessModes::RW, 0, ret), CanOpenSdoConfSubIndexes::NodeId);
    addEntry (ret, idx);
}

void CanOpenObjDict::createPdoCommParamEntry (const CanOpenIndex idx) {
    CanOpenEntry * ret = new CanOpenEntry (CanOpenObjTypes::Record, this);
    CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RO, 0, ret);
    subEntryCount->writeAs<quint8> (0x05);
    ret->addSubEntry (subEntryCount, 0x00);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt32, CanOpenAccessModes::RW, 0, ret), CanOpenPdoConfSubIndexes::CobId);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt8,  CanOpenAccessModes::RW, 0, ret), CanOpenPdoConfSubIndexes::Type);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt16, CanOpenAccessModes::RW, 0, ret), CanOpenPdoConfSubIndexes::InhibitTime);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt8,  CanOpenAccessModes::RW, 0, ret), CanOpenPdoConfSubIndexes::Reserved);
    ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt16, CanOpenAccessModes::RW, 0, ret), CanOpenPdoConfSubIndexes::EventTimer);
    addEntry (ret, idx);
}

void CanOpenObjDict::createPdoMappingEntry (const CanOpenIndex idx) {
    CanOpenEntry * ret = new CanOpenEntry (CanOpenObjTypes::Array, this);
    CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RW, 0, ret);
    subEntryCount->writeAs<quint8> (0x00); // NOTE : don't declare any by default
    ret->addSubEntry (subEntryCount, 0x00);
    static const CanOpenSubIndex PDO_MAPPING_MAX_INDEX = 0x40;
    for (CanOpenSubIndex subIdx = 0x01; subIdx < PDO_MAPPING_MAX_INDEX; ++subIdx) {
        ret->addSubEntry (new CanOpenSubEntry (CanOpenDataTypes::UInt32, CanOpenAccessModes::RW, 0, ret), subIdx);
    }
    addEntry (ret, idx);
}

void CanOpenObjDict::addEntry (CanOpenEntry * entry, const CanOpenIndex idx) {
    if (!m_entriesByIndex.contains (idx)) {
        m_entriesByIndex.insert (idx, entry);
    }
}

bool CanOpenObjDict::hasEntry (const CanOpenIndex idx) const {
    return m_entriesByIndex.contains (idx);
}

bool CanOpenObjDict::hasSubEntry (const CanOpenIndex idx, const CanOpenSubIndex subIdx) const {
    bool ret = false;
    if (CanOpenEntry * entry = getEntry (idx)) {
        ret = entry->hasSubEntry (subIdx);
    }
    return ret;
}

CanOpenEntry * CanOpenObjDict::getEntry (const CanOpenIndex idx) const {
    return m_entriesByIndex.value (idx, Q_NULLPTR);
}

CanOpenSubEntry * CanOpenObjDict::getSubEntry (const CanOpenIndex idx, const CanOpenSubIndex subIdx) const {
    CanOpenSubEntry * subEntry = Q_NULLPTR;
    if (CanOpenEntry * entry = getEntry (idx)) {
        subEntry = entry->getSubEntry (subIdx);
    }
    return subEntry;
}

static inline QByteArray metadataDynamicQtProperty (const QString & metaKey) {
    return ("qtcanv2_canopenobd_metadata__" % metaKey.toLatin1 ());
}

void CanOpenObjDict::setMetaDataForObdPos (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const QString & metaKey, const QVariant & metaData) {
    if (!metaKey.isEmpty () && (int (metaData.type ()) != int (QMetaType::UnknownType))) {
        if (CanOpenSubEntry * subEntry = getSubEntry (idx, subIdx)) {
            subEntry->setProperty (metadataDynamicQtProperty (metaKey), metaData);
        }
    }
}

QVariant CanOpenObjDict::getMetaDataForObdPos (const CanOpenIndex idx, const CanOpenSubIndex subIdx, const QString & metaKey) const {
    QVariant ret;
    if (!metaKey.isEmpty ()) {
        if (CanOpenSubEntry * subEntry = getSubEntry (idx, subIdx)) {
            ret = subEntry->property (metadataDynamicQtProperty (metaKey));
        }
    }
    return ret;
}

void CanOpenObjDict::traverseObd (void) {
    emit beginObd (this);
    for (SparseArray<CanOpenIndex, CanOpenEntry *>::iterator itIdx = m_entriesByIndex.begin (); itIdx != m_entriesByIndex.end (); ++itIdx) {
        if (CanOpenEntry * entry = (* itIdx).value) {
            emit beginEntry ((* itIdx).key, entry);
            for (SparseArray<CanOpenSubIndex, CanOpenSubEntry *>::iterator itSubIdx = entry->m_subEntriesBySubIndex.begin (); itSubIdx != entry->m_subEntriesBySubIndex.end (); ++itSubIdx) {
                if (CanOpenSubEntry * subEntry = (* itSubIdx).value) {
                    emit beginSubEntry ((* itSubIdx).key, subEntry);
                    emit endSubEntry ();
                }
            }
            emit endEntry ();
        }
    }
    emit endObd ();
}
