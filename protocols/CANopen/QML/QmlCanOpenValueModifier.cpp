
#include "QmlCanOpenValueModifier.h"

#include "CanOpenObjDict.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"

QmlCanOpenValueModifier::QmlCanOpenValueModifier (QObject * parent)
    : AbstractObdPreparator (parent)
    , m_index    (0x0000)
    , m_subIndex (0x00)
{ }

void QmlCanOpenValueModifier::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (obd->hasEntry (m_index)) {
            CanOpenEntry * entry = obd->getEntry (m_index);
            if (entry->hasSubEntry (m_subIndex)) {
                CanOpenSubEntry * subEntry = entry->getSubEntry (m_subIndex);
                subEntry->writeFromQtVariant (m_data);
                subEntry->resetWrittenFlag ();
            }
            else {
                emit diag (Warning, QStringLiteral ("OBD entry %1 has no such subentry %2 !").arg (QString::fromLatin1 (QtCAN::hexNumberStr (m_index, 2))).arg (QString::fromLatin1 (QtCAN::hexNumberStr (m_subIndex, 1))));
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("OBD has no such entry %1 !").arg (QString::fromLatin1 (QtCAN::hexNumberStr (m_index, 2))));
        }
    }
}

int QmlCanOpenValueModifier::getIndex (void) const {
   return m_index;
}

int QmlCanOpenValueModifier::getSubIndex (void) const {
    return m_subIndex;
}

QVariant QmlCanOpenValueModifier::getData (void) const {
    return m_data;
}

void QmlCanOpenValueModifier::setIndex (const int index) {
    if (index >= 0x0001 && index <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (index);
        if (m_index != tmp) {
            m_index = tmp;
            emit indexChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("ValueModifier.index should be between 1 and 65535 !"));
    }
}

void QmlCanOpenValueModifier::setSubIndex (const int subIndex) {
    if (subIndex >= 0x00 && subIndex <= 0xFF) {
        const CanOpenSubIndex tmp = CanOpenSubIndex (subIndex);
        if (m_subIndex != tmp) {
            m_subIndex = tmp;
            emit subIndexChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("ValueModifier.subIndex should be between 0 and 255 !"));
    }
}

void QmlCanOpenValueModifier::setData (const QVariant & data) {
    if (m_data != data) {
        m_data = data;
        emit dataChanged ();
    }
}
