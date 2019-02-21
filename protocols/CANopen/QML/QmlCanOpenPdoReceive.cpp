
#include "QmlCanOpenPdoReceive.h"

#include "CanOpenObjDict.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenDefs.h"

QmlCanOpenPdoReceive::QmlCanOpenPdoReceive (QObject * parent) : AbstractObdPreparator (parent)
  , m_pdoNum        (0)
  , m_cobId         (0x000)
  , m_receptionType (0x00)
  , m_mappedVars    (this)
{ }

void QmlCanOpenPdoReceive::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (m_pdoNum) {
            if (m_cobId) {
                CanOpenEntry * entryConfig = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDORX_CONFIG_START + m_pdoNum -1));
                if (entryConfig != Q_NULLPTR) {
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::CobId)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::CobId)->writeAs<quint32> (m_cobId);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::CobId)->resetWrittenFlag ();
                    }
                    if (entryConfig->hasSubEntry (CanOpenPdoConfSubIndexes::Type)) {
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::Type)->writeAs<quint8> (m_receptionType);
                        entryConfig->getSubEntry (CanOpenPdoConfSubIndexes::Type)->resetWrittenFlag ();
                    }
                }
                else {
                    emit diag (Warning, QStringLiteral ("Base OBD could have an issue, PDORX config entries are not present !"));
                }
                CanOpenEntry * entryMapping = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::PDORX_MAPPING_START + m_pdoNum -1));
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
                    emit diag (Warning, QStringLiteral ("OBD could have an issue, PDORX mapping entries are not present !"));
                }
            }
            else {
                emit diag (Warning, QStringLiteral ("PdoReceive.cobId must be greater than 0x000 !"));
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("PdoReceive.pdoNum must be greater than 0 !"));
        }
    }
}

QQmlListProperty<QmlCanOpenPdoMappedVar> QmlCanOpenPdoReceive::getMappedVars (void) {
    return m_mappedVars;
}

int QmlCanOpenPdoReceive::getPdoNum (void) const {
    return int (m_pdoNum);
}

int QmlCanOpenPdoReceive::getCobId (void) const {
    return int (m_cobId);
}

int QmlCanOpenPdoReceive::getReceptionType (void) const {
    return int (m_receptionType);
}

void QmlCanOpenPdoReceive::setPdoNum (const int pdoNum) {
    if (pdoNum >= 1 && pdoNum <= 512) {
        const CanOpenCounter tmp = CanOpenCounter (pdoNum);
        if (m_pdoNum != tmp) {
            m_pdoNum = tmp;
            emit pdoNumChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("PdoReceive.pdoNum must be between 1 and 512 !"));
    }
}

void QmlCanOpenPdoReceive::setCobId (const int cobId) {
    const CanOpenCobId tmp = CanOpenCobId (cobId);
    if (m_cobId != tmp) {
        m_cobId = tmp;
        emit cobIdChanged ();
    }
}

void QmlCanOpenPdoReceive::setReceptionType (const int receptionType) {
    const quint8 tmp = quint8 (receptionType);
    if (m_receptionType != tmp) {
        m_receptionType = tmp;
        emit receptionTypeChanged ();
    }
}
