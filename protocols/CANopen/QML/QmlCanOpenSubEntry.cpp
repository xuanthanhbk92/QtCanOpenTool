
#include "QmlCanOpenSubEntry.h"

#include "CanOpenDefs.h"

QmlCanOpenSubEntry::QmlCanOpenSubEntry (QObject * parent) : QObject (parent)
  , m_dataType   (CanOpenDataTypes::UnknownType)
  , m_dataLen    (0)
  , m_attributes (CanOpenAccessModes::RW)
{ }

int QmlCanOpenSubEntry::getDataType (void) const {
    return int (m_dataType);
}

int QmlCanOpenSubEntry::getDataLen (void) const {
    return int (m_dataLen);
}

int QmlCanOpenSubEntry::getAttributes (void) const {
    return int (m_attributes);
}

QVariantMap QmlCanOpenSubEntry::getMetaData (void) const {
    return m_metaData;
}

QVariant QmlCanOpenSubEntry::getData (void) const {
    return m_data;
}

void QmlCanOpenSubEntry::setDataType (const int dataType) {
    const CanOpenDataType tmp = CanOpenDataType (dataType);
    if (m_dataType != tmp) {
        m_dataType = tmp;
        emit dataTypeChanged ();
    }
}

void QmlCanOpenSubEntry::setDataLen (const int dataLen) {
    const CanOpenDataLen tmp = CanOpenDataLen (dataLen);
    if (m_dataLen != tmp) {
        m_dataLen = tmp;
        emit dataLenChanged ();
    }
}

void QmlCanOpenSubEntry::setAttributes (const int attributes) {
    const CanOpenAccessMode tmp = CanOpenAccessMode (attributes);
    if (m_attributes != tmp) {
        m_attributes = tmp;
        emit attributesChanged ();
    }
}

void QmlCanOpenSubEntry::setMetaData (const QVariantMap & metaData) {
    if (m_metaData != metaData) {
        m_metaData = metaData;
        emit metaDataChanged ();
    }
}

void QmlCanOpenSubEntry::setData (const QVariant & data) {
    if (m_data != data) {
        m_data = data;
        emit dataChanged ();
    }
}
