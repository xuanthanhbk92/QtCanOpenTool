
#include "QmlCanOpenPdoMappedVar.h"

#include "AbstractObdPreparator.h"

QmlCanOpenPdoMappedVar::QmlCanOpenPdoMappedVar (QObject * parent)
    : QObject (parent)
    , m_mapping (quint32 (0x00000000))
    , m_preparator (qobject_cast<AbstractObdPreparator *> (parent))
{ }

int QmlCanOpenPdoMappedVar::getIndex (void) const {
    return int (m_mapping.fields.idx);
}

int QmlCanOpenPdoMappedVar::getSubIndex (void) const {
    return int (m_mapping.fields.subIdx);
}

int QmlCanOpenPdoMappedVar::getBitsCount (void) const {
    return int (m_mapping.fields.bitsCount);
}

quint32 QmlCanOpenPdoMappedVar::toMapping (void) const {
    return m_mapping.raw;
}

void QmlCanOpenPdoMappedVar::setIndex (const int index) {
    if (index >= 0x0001 && index <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (index);
        if (m_mapping.fields.idx != tmp) {
            m_mapping.fields.idx = tmp;
            emit indexChanged ();
        }
    }
    else {
        emit m_preparator->diag (AbstractObdPreparator::Warning, QStringLiteral ("PdoMappedVar.index should be between 1 and 65535 !"));
    }
}

void QmlCanOpenPdoMappedVar::setSubIndex (const int subIndex) {
    if (subIndex >= 0x00 && subIndex <= 0xFF) {
        const CanOpenSubIndex tmp = CanOpenSubIndex (subIndex);
        if (m_mapping.fields.subIdx != tmp) {
            m_mapping.fields.subIdx = tmp;
            emit subIndexChanged ();
        }
    }
    else {
        emit m_preparator->diag (AbstractObdPreparator::Warning, QStringLiteral ("PdoMappedVar.subIndex should be between 0 and 255 !"));
    }
}

void QmlCanOpenPdoMappedVar::setBitsCount (const int bitsCount) {
    const quint8 bits = quint8 (bitsCount);
    if (m_mapping.fields.bitsCount != bits) {
        m_mapping.fields.bitsCount = bits;
        emit bitsCountChanged ();
    }
}
