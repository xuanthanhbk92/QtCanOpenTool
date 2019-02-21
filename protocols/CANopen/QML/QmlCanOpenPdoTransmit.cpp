
#include "QmlCanOpenPdoTransmit.h"

#include "CanOpenObjDict.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenDefs.h"

QmlCanOpenPdoTransmit::QmlCanOpenPdoTransmit (QObject * parent) : AbstractObdPreparator (parent)
  , m_pdoNum           (0)
  , m_cobId            (0x000)
  , m_transmissionType (0x00)
  , m_inhibitTime      (0)
  , m_eventTimer       (0)
  , m_mappedVars       (this)
{ }

void QmlCanOpenPdoTransmit::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (m_pdoNum) {
            if (m_cobId) {
                CanOpenEntry * entryConfig = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDOTX_CONFIG_START + m_pdoNum -1));
                if (entryConfig != Q_NULLPTR) {
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::CobId)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::CobId)->writeAs<quint32> (m_cobId);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::CobId)->resetWrittenFlag ();
                    }
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::Type)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::Type)->writeAs<quint8> (m_transmissionType);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::Type)->resetWrittenFlag ();
                    }
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::InhibitTime)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::InhibitTime)->writeAs<quint16> (m_inhibitTime);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::InhibitTime)->resetWrittenFlag ();
                    }
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::EventTimer)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::EventTimer)->writeAs<quint16> (m_eventTimer);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::EventTimer)->resetWrittenFlag ();
                    }
                }
                else {
                    emit diag (Warning, QStringLiteral ("Base OBD could have an issue, PDORX config entries are not present !"));
                }
                CanOpenEntry * entryMapping = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDOTX_MAPPING_START + m_pdoNum -1));
                if (entryMapping != Q_NULLPTR) {
                    CanOpenSubIndex subIdx = 0x00;
                    if (entryMapping->hasSubEntry (subIdx)) {
                        entryMapping->getSubEntry (subIdx)->writeAs<quint8> (quint8 (m_mappedVars.asVector ().count ()));
                        entryMapping->getSubEntry (subIdx)->resetWrittenFlag ();
                        subIdx++;
                    }
                    foreach (QmlCanOpenPdoMappedVar * mappedVar, m_mappedVars.asVector ()) {
                        if (entryMapping->hasSubEntry (subIdx)) {
                            entryMapping->getSubEntry (subIdx)->writeAs<quint32> (mappedVar->toMapping ());
                            entryMapping->getSubEntry (subIdx)->resetWrittenFlag ();
                            subIdx++;
                        }
                    }
                }
                else {
                    emit diag (Warning, QStringLiteral ("PdoTransmit.cobId must be greater than 0x000 !"));
                }
            }
            else {
                emit diag (Warning, QStringLiteral ("PdoTransmit.cobId must be greater than 0x000 !"));
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("PdoTransmit.pdoNum must be greater than 0 !"));
        }
    }
}

int QmlCanOpenPdoTransmit::getPdoNum (void) const {
    return int (m_pdoNum);
}

int QmlCanOpenPdoTransmit::getCobId (void) const {
    return int (m_cobId);
}

int QmlCanOpenPdoTransmit::getTransmissionType (void) const {
    return int (m_transmissionType);
}

int QmlCanOpenPdoTransmit::getInhibitTime (void) const {
    return int (m_inhibitTime);
}

int QmlCanOpenPdoTransmit::getEventTimer (void) const {
    return int (m_eventTimer);
}

QQmlListProperty<QmlCanOpenPdoMappedVar> QmlCanOpenPdoTransmit::getMappedVars (void) {
    return m_mappedVars;
}

void QmlCanOpenPdoTransmit::setPdoNum (const int pdoNum) {
    if (pdoNum >= 1 && pdoNum <= 512) {
        const CanOpenCounter tmp = CanOpenCounter (pdoNum);
        if (m_pdoNum != tmp) {
            m_pdoNum = tmp;
            emit pdoNumChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("PdoTransmit.pdoNum must be between 1 and 512 !"));
    }
}

void QmlCanOpenPdoTransmit::setCobId (const int cobId) {
    const CanOpenCobId tmp = CanOpenCobId (cobId);
    if (m_cobId != tmp) {
        m_cobId = tmp;
        emit cobIdChanged ();
    }
}

void QmlCanOpenPdoTransmit::setTransmissionType (const int transmissionType) {
    const quint8 tmp = quint8 (transmissionType);
    if (m_transmissionType != tmp) {
        m_transmissionType = tmp;
        emit transmissionTypeChanged ();
    }
}

void QmlCanOpenPdoTransmit::setInhibitTime (const int inhibitTime) {
    const quint16 tmp = quint16 (inhibitTime);
    if (m_inhibitTime != tmp) {
        m_inhibitTime = tmp;
        emit inhibitTimeChanged ();
    }
}

void QmlCanOpenPdoTransmit::setEventTimer (const int eventTimer) {
    const quint16 tmp = quint16 (eventTimer);
    if (m_eventTimer != tmp) {
        m_eventTimer = tmp;
        emit eventTimerChanged ();
    }
}
