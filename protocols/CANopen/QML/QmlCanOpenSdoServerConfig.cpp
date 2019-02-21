
#include "QmlCanOpenSdoServerConfig.h"

#include "CanOpenObjDict.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenDefs.h"

QmlCanOpenSdoServerConfig::QmlCanOpenSdoServerConfig (QObject * parent) : AbstractObdPreparator (parent)
  , m_srvNum          (0)
  , m_cobIdCliRequest (0x000)
  , m_cobIdSrvReply   (0x000)
{ }

void QmlCanOpenSdoServerConfig::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (m_srvNum) {
            CanOpenEntry * entryConfig = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::SDO_SERVERS_START + m_srvNum -1));
            if (entryConfig != Q_NULLPTR) {
                if (entryConfig->hasSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)) {
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->writeAs<quint32> (m_cobIdCliRequest);
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->resetWrittenFlag ();
                }
                if (entryConfig->hasSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)) {
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->writeAs<quint32> (m_cobIdSrvReply);
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->resetWrittenFlag ();
                }
            }
            else {
                emit diag (Warning, QStringLiteral ("Base OBD could have an issue, SDO SRV config entries are not present !"));
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("SdoServer.srvNum must be greater than 0 !"));
        }
    }
}

int QmlCanOpenSdoServerConfig::getSrvNum (void) const {
    return int (m_srvNum);
}

int QmlCanOpenSdoServerConfig::getCobIdCliRequest (void) const {
    return int (m_cobIdCliRequest);
}

int QmlCanOpenSdoServerConfig::getCobIdSrvReply (void) const {
    return int (m_cobIdSrvReply);
}

void QmlCanOpenSdoServerConfig::setSrvNum (const int srvNum) {
    if (srvNum >= 1 && srvNum <= 128) {
        const CanOpenCounter tmp = CanOpenCounter (srvNum);
        if (m_srvNum != tmp) {
            m_srvNum = tmp;
            emit srvNumChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("SdoServer.srvNum must be between 1 and 128 !"));
    }
}

void QmlCanOpenSdoServerConfig::setCobIdCliRequest (const int cobIdCliRequest) {
    const CanOpenCobId tmp = CanOpenCobId (cobIdCliRequest);
    if (m_cobIdCliRequest != tmp) {
        m_cobIdCliRequest = tmp;
        emit cobIdCliRequestChanged ();
    }
}

void QmlCanOpenSdoServerConfig::setCobIdSrvReply (const int cobIdSrvReply) {
    const CanOpenCobId tmp = CanOpenCobId (cobIdSrvReply);
    if (m_cobIdSrvReply != tmp) {
        m_cobIdSrvReply = tmp;
        emit cobIdSrvReplyChanged ();
    }
}
