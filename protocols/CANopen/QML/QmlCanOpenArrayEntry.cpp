
#include "QmlCanOpenArrayEntry.h"

#include "CanOpenDefs.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenObjDict.h"

#include <QDebug>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <qqml.h>

QmlCanOpenArrayEntry::QmlCanOpenArrayEntry (QObject * parent) : AbstractObdPreparator (parent)
  , m_num        (0)
  , m_index      (0x0000)
  , m_dataType   (CanOpenDataTypes::UnknownType)
  , m_dataLen    (0)
  , m_attributes (CanOpenAccessModes::RW)
  , m_count      (0x00)
{ }

void QmlCanOpenArrayEntry::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (m_count) {
            CanOpenSubIndex subIdx = 0x00;
            CanOpenEntry * entry = new CanOpenEntry (CanOpenObjTypes::Array, obd);
            obd->addEntry (entry, m_index);
            CanOpenSubEntry * subEntryCount = new CanOpenSubEntry (CanOpenDataTypes::UInt8, CanOpenAccessModes::RO, subIdx, entry);
            subEntryCount->writeAs<quint8> (m_count);
            subEntryCount->resetWrittenFlag ();
            entry->addSubEntry (subEntryCount, subIdx);
            QQmlContext subContext (qmlContext (this));
            QQmlExpression qmlExprMetaData (m_metaData, &subContext);
            for (subIdx = 0x01; subIdx <= m_count; ++subIdx) {
                subContext.setContextProperty ("subIdx", int (subIdx));
                CanOpenSubEntry * subEntry = new CanOpenSubEntry (m_dataType, m_attributes, m_dataLen, entry);
                entry->addSubEntry (subEntry, subIdx);
                subEntry->resetWrittenFlag ();
                const QVariantMap metaData = qmlExprMetaData.evaluate ().value<QVariantMap> ();
                if (qmlExprMetaData.hasError ()) {
                    qCritical () << qmlExprMetaData.error ();
                }
                else {
                    for (QVariantMap::const_iterator it = metaData.constBegin (); it != metaData.constEnd (); ++it) {
                        obd->setMetaDataForObdPos (m_index, subIdx, it.key (), it.value ());
                    }
                }
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("ArrayEntry.count should be greater than 0 !"));
        }
    }
}

int QmlCanOpenArrayEntry::getNum (void) const {
    return int (m_num);
}

int QmlCanOpenArrayEntry::getIndex (void) const {
    return int (m_index);
}

int QmlCanOpenArrayEntry::getDataType (void) const {
    return int (m_dataType);
}

int QmlCanOpenArrayEntry::getDataLen (void) const {
    return int (m_dataLen);
}

int QmlCanOpenArrayEntry::getAttributes (void) const {
    return int (m_attributes);
}

int QmlCanOpenArrayEntry::getCount (void) const {
    return int (m_count);
}

const QQmlScriptString & QmlCanOpenArrayEntry::getMetaData (void) const {
    return m_metaData;
}

void QmlCanOpenArrayEntry::setNum (const int num) {
    const CanOpenCounter tmp = CanOpenCounter (num);
    if (m_num != tmp) {
        m_num = tmp;
        emit numChanged ();
    }
}

void QmlCanOpenArrayEntry::setIndex (const int index) {
    if (index >= 0x0001 && index <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (index);
        if (m_index != tmp) {
            m_index = tmp;
            emit indexChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("ArrayEntry.index should be between 1 and 65535 !"));
    }
}

void QmlCanOpenArrayEntry::setDataType (const int dataType) {
    const CanOpenDataType tmp = CanOpenDataType (dataType);
    if (m_dataType != tmp) {
        m_dataType = tmp;
        emit dataTypeChanged ();
    }
}

void QmlCanOpenArrayEntry::setDataLen (const int dataLen) {
    const CanOpenDataLen tmp = CanOpenDataLen (dataLen);
    if (m_dataLen != tmp) {
        m_dataLen = tmp;
        emit dataLenChanged ();
    }
}

void QmlCanOpenArrayEntry::setAttributes (const int attributes) {
    const CanOpenAccessMode tmp = CanOpenAccessMode (attributes);
    if (m_attributes != tmp) {
        m_attributes = tmp;
        emit attributesChanged ();
    }
}

void QmlCanOpenArrayEntry::setCount (const int count) {
    if (count >= 0x01 && count <= 0xFF) {
        const CanOpenSubIndex tmp = CanOpenSubIndex (count);
        if (m_count != tmp) {
            m_count = tmp;
            emit countChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("ArrayEntry.count should be between 1 and 255 !"));
    }
}

void QmlCanOpenArrayEntry::setMetaData (const QQmlScriptString & metaData) {
#if QT_VERSION >= 0x050400
    if (m_metaData != metaData) {
#endif
        m_metaData = metaData;
        emit metaDataChanged ();
#if QT_VERSION >= 0x050400
    }
#endif
}
