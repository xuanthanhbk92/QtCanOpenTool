
#include "QmlCanOpenVarEntry.h"

#include "CanOpenDefs.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenObjDict.h"

QmlCanOpenVarEntry::QmlCanOpenVarEntry (QObject * parent) : AbstractObdPreparator (parent)
  , m_num        (0)
  , m_index      (0x0000)
  , m_dataType   (CanOpenDataTypes::UnknownType)
  , m_dataLen    (0)
  , m_attributes (CanOpenAccessModes::RW)
{ }

void QmlCanOpenVarEntry::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        const CanOpenSubIndex subIndex = 0x00;
        CanOpenEntry * entry = new CanOpenEntry (CanOpenObjTypes::Var, obd);
        obd->addEntry (entry, m_index);
        CanOpenSubEntry * subEntry = new CanOpenSubEntry (m_dataType, m_attributes, m_dataLen, entry);
        entry->addSubEntry (subEntry, subIndex);
        subEntry->writeFromQtVariant (m_data);
        subEntry->resetWrittenFlag ();
        for (QVariantMap::const_iterator it = m_metaData.constBegin (); it != m_metaData.constEnd (); ++it) {
            obd->setMetaDataForObdPos (m_index, subIndex, it.key (), it.value ());
        }
    }
}

int QmlCanOpenVarEntry::getNum (void) const {
    return int (m_num);
}

int QmlCanOpenVarEntry::getIndex (void) const {
    return int (m_index);
}

int QmlCanOpenVarEntry::getDataType (void) const {
    return int (m_dataType);
}

int QmlCanOpenVarEntry::getDataLen (void) const {
    return int (m_dataLen);
}

int QmlCanOpenVarEntry::getAttributes (void) const {
    return int (m_attributes);
}

QVariantMap QmlCanOpenVarEntry::getMetaData (void) const {
    return m_metaData;
}

QVariant QmlCanOpenVarEntry::getData (void) const {
    return m_data;
}

void QmlCanOpenVarEntry::setNum (const int num) {
    const CanOpenCounter tmp = CanOpenCounter (num);
    if (m_num != tmp) {
        m_num = tmp;
        emit numChanged ();
    }
}

void QmlCanOpenVarEntry::setIndex (const int index) {
    if (index >= 0x0001 && index <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (index);
        if (m_index != tmp) {
            m_index = tmp;
            emit indexChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("VarEntry.index should be between 1 and 65535 !"));
    }
}

void QmlCanOpenVarEntry::setDataType (const int dataType) {
    const CanOpenDataType tmp = CanOpenDataType (dataType);
    if (m_dataType != tmp) {
        m_dataType = tmp;
        emit dataTypeChanged ();
    }
}

void QmlCanOpenVarEntry::setDataLen (const int dataLen) {
    const CanOpenDataLen tmp = CanOpenDataLen (dataLen);
    if (m_dataLen != tmp) {
        m_dataLen = tmp;
        emit dataLenChanged ();
    }
}

void QmlCanOpenVarEntry::setAttributes (const int attributes) {
    const CanOpenAccessMode tmp = CanOpenAccessMode (attributes);
    if (m_attributes != tmp) {
        m_attributes = tmp;
        emit attributesChanged ();
    }
}

void QmlCanOpenVarEntry::setMetaData (const QVariantMap & metaData) {
    if (m_metaData != metaData) {
        m_metaData = metaData;
        emit metaDataChanged ();
    }
}

void QmlCanOpenVarEntry::setData (const QVariant & data) {
    if (m_data != data) {
        m_data = data;
        emit dataChanged ();
    }
}
