
#include "CanMessage.h"

#include <QDebug>

/****************** CAN ID ***********************/

CanId::CanId (const quint32 canIdEFF, const bool isRTR, const bool isERR)
    : m_canId ((canIdEFF & MASK_EFF) | (isRTR ? FLAG_RTR : 0) | (isERR ? FLAG_ERR : 0) | FLAG_EFF)
{ }

CanId::CanId (const quint16 canIdSFF, const bool isRTR, const bool isERR)
    : m_canId ((canIdSFF & MASK_SFF) | (isRTR ? FLAG_RTR : 0) | (isERR ? FLAG_ERR : 0))
{ }

qbool CanId::isEFF (void) const {
    return bool (m_canId & FLAG_EFF);
}

qbool CanId::isRTR (void) const {
    return bool (m_canId & FLAG_RTR);
}

qbool CanId::isERR (void) const {
    return bool (m_canId & FLAG_ERR);
}

quint16 CanId::canIdSFF (void) const {
    return quint16 (m_canId & MASK_SFF);
}

quint32 CanId::canIdEFF (void) const {
    return quint32 (m_canId & MASK_EFF);
}

/******************** CAN DATA **************************/

CanData::CanData (const qvoidptr ptr, const quint length) : FixedBuffer<8> (ptr, quint (length)) { }

/******************** CAN MESSAGE ************************/

CanMessage::CanMessage (const CanId & canId, const quint length, const qvoidptr ptr, const QDateTime & timestamp)
    : m_canId     (canId)
    , m_canData   (ptr, length)
    , m_timestamp (timestamp)
{ }

void CanMessage::setCanId (const CanId & canId) {
    m_canId = canId;
}

void CanMessage::setCanData (const CanData & canData) {
    m_canData = canData;
}

const CanId & CanMessage::getCanId (void) const {
    return m_canId;
}

const CanData & CanMessage::getCanData (void) const {
    return m_canData;
}

const QDateTime & CanMessage::getTimeStamp (void) const {
    return m_timestamp;
}
