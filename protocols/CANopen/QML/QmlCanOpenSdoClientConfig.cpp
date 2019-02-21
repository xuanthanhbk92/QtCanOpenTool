
#include "QmlCanOpenSdoClientConfig.h"

#include "CanOpenObjDict.h"
#include "CanOpenEntry.h"
#include "CanOpenSubEntry.h"
#include "CanOpenDefs.h"

QmlCanOpenSdoClientConfig::QmlCanOpenSdoClientConfig (QObject * parent) : AbstractObdPreparator (parent)
  , m_cliNum          (0)
  , m_cobIdCliRequest (0x000)
  , m_cobIdSrvReply   (0x000)
  , m_nodeId          (0x00)
{ }

void QmlCanOpenSdoClientConfig::prepareOBD (CanOpenObjDict * obd) {
    if (obd != Q_NULLPTR) {
        if (m_cliNum) {
            CanOpenEntry * entryConfig = obd->getEntry (CanOpenIndex (CanOpenObjDictRanges::SDO_CLIENTS_START + m_cliNum -1));
            if (entryConfig != Q_NULLPTR) {
                if (entryConfig->hasSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)) {
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->writeAs<quint32> (m_cobIdCliRequest);
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdCliRequest)->resetWrittenFlag ();
                }
                if (entryConfig->hasSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)) {
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->writeAs<quint32> (m_cobIdSrvReply);
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::CobIdSrvReply)->resetWrittenFlag ();
                }
                if (entryConfig->hasSubEntry (CanOpenSdoConfSubIndexes::NodeId)) {
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::NodeId)->writeAs<quint8> (m_nodeId);
                    entryConfig->getSubEntry (CanOpenSdoConfSubIndexes::NodeId)->resetWrittenFlag ();
                }
            }
            else {
                emit diag (Warning, QStringLiteral ("Base OBD could have an issue, SDO CLI config entries are not present !"));
            }
        }
        else {
            emit diag (Warning, QStringLiteral ("SdoClient.cliNum must be greater than 0 !"));
        }
    }
}

int QmlCanOpenSdoClientConfig::getCliNum (void) const {
    return int (m_cliNum);
}

int QmlCanOpenSdoClientConfig::getCobIdCliRequest (void) const {
    return int (m_cobIdCliRequest);
}

int QmlCanOpenSdoClientConfig::getCobIdSrvReply (void) const {
    return int (m_cobIdSrvReply);
}

int QmlCanOpenSdoClientConfig::getNodeId (void) const {
    return int (m_nodeId);
}

void QmlCanOpenSdoClientConfig::setCliNum (const int cliNum) {
    if (cliNum >= 1 && cliNum <= 128) {
        const CanOpenCounter tmp = CanOpenCounter (cliNum);
        if (m_cliNum != tmp) {
            m_cliNum = tmp;
            emit cliNumChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("SdoClient.cliNum must be between 1 and 128 !"));
    }
}

void QmlCanOpenSdoClientConfig::setCobIdCliRequest (const int cobIdCliRequest) {
    const CanOpenCobId tmp = CanOpenCobId (cobIdCliRequest);
    if (m_cobIdCliRequest != tmp) {
        m_cobIdCliRequest = tmp;
        emit cobIdCliRequestChanged ();
    }
}

void QmlCanOpenSdoClientConfig::setCobIdSrvReply (const int cobIdSrvReply) {
    const CanOpenCobId tmp = CanOpenCobId (cobIdSrvReply);
    if (m_cobIdSrvReply != tmp) {
        m_cobIdSrvReply = tmp;
        emit cobIdSrvReplyChanged ();
    }
}

void QmlCanOpenSdoClientConfig::setNodeId (const int nodeId) {
    const CanOpenNodeId tmp = CanOpenNodeId (nodeId);
    if (m_nodeId != tmp) {
        m_nodeId = tmp;
        emit nodeIdChanged ();
    }
}
