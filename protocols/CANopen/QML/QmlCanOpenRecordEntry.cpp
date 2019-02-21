
#include "QmlCanOpenRecordEntry.h"

#include "CanOpenDefs.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenObjDict.h"

QmlCanOpenRecordEntry::QmlCanOpenRecordEntry (QObject * parent) : AbstractObdPreparator (parent)
  , m_num        (0)
  , m_index      (0x0000)
  , m_subEntries (this)
{ }

void QmlCanOpenRecordEntry::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        CanOpenEntry * entry = new CanOpenEntry (CanOpenObjTypes::Record, obd);
        obd->addEntry (entry, m_index);
        const CanOpenSubIndex maxSubIdx = CanOpenSubIndex (m_subEntries.asVector ().count ());
        CanOpenSubIndex subIdx = 0x00;
        CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RO, 0, entry);
        subEntryCount->writeAs<quint8> (maxSubIdx);
        subEntryCount->resetWrittenFlag ();
        entry->addSubEntry (subEntryCount, subIdx);
        ++subIdx;
        foreach (QmlCanOpenSubEntry * subVar, m_subEntries.asVector ()) {
            if (subVar != Q_NULLPTR) {
                CanOpenSubEntry * subEntry = new CanOpenSubEntry (CanOpenDataType   (subVar->getDataType ()),
                                                                  CanOpenAccessMode (subVar->getAttributes ()),
                                                                  CanOpenDataLen    (subVar->getDataLen ()),
                                                                  entry);
                entry->addSubEntry (subEntry, subIdx);
                subEntry->writeFromQtVariant (subVar->getData ());
                subEntry->resetWrittenFlag ();
                const QVariantMap metaData = subVar->getMetaData ();
                for (QVariantMap::const_iterator it = metaData.constBegin (); it != metaData.constEnd (); ++it) {
                    obd->setMetaDataForObdPos (m_index, subIdx, it.key (), it.value ());
                }
                ++subIdx;
            }
        }
    }
}

int QmlCanOpenRecordEntry::getNum (void) const {
    return int (m_num);
}

int QmlCanOpenRecordEntry::getIndex (void) const {
    return int (m_index);
}

QQmlListProperty<QmlCanOpenSubEntry> QmlCanOpenRecordEntry::getSubEntries (void) {
    return m_subEntries;
}

void QmlCanOpenRecordEntry::setNum (const int num) {
    const CanOpenCounter tmp = CanOpenCounter (num);
    if (m_num != tmp) {
        m_num = tmp;
        emit numChanged ();
    }
}

void QmlCanOpenRecordEntry::setIndex (const int index) {
    if (index >= 0x0001 && index <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (index);
        if (m_index != tmp) {
            m_index = tmp;
            emit indexChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("RecordEntry.index should be between 1 and 65535 !"));
    }
}
