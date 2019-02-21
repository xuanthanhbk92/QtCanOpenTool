
#include "CanDriver_socketCanBcm.h"
#include "CanDriver_socketCanCommon.h"

#include "CanMessage.h"

#include <QElapsedTimer>
#include <QStringBuilder>

struct SocketCanBcmFrame {
    bcm_msg_head bcmHeader;
    can_frame    canFrame;
};

const QString CanDriver_socketCanBcm::BUS_NAME           = QStringLiteral ("busName");
const QString CanDriver_socketCanBcm::THROTTLED_CANIDS   = QStringLiteral ("throttledCanIds");
const QString CanDriver_socketCanBcm::WHITELISTED_CANIDS = QStringLiteral ("whitelistedCanIds");

CanDriver_socketCanBcm::CanDriver_socketCanBcm (QObject * parent) : CanDriver (parent)
  , m_sockfd (-1)
  , m_valid  (false)
  , m_poller (Q_NULLPTR)
{ }

CanDriver_socketCanBcm::~CanDriver_socketCanBcm (void) {
    stop ();
}

bool CanDriver_socketCanBcm::init (const QVariantMap & options) {
    bool ret = false;
    QByteArray busName = options.value (BUS_NAME, "can0").toByteArray ();
    bool isNum;
    busName.left (1).toInt (&isNum);
    if (isNum) {
        busName.prepend (QByteArrayLiteral ("can"));
    }
    m_sockfd = CAN_SOCKET (PF_CAN, SOCK_DGRAM, CAN_BCM);
    if (m_sockfd >= 0) {
        ifreq ifr;
        qstrncpy (ifr.ifr_name, busName.constData (), IFNAMSIZ);
        int err = 0;
        err = CAN_IOCTL (m_sockfd, SIOCGIFINDEX, &ifr);
        if (!err) {
            sockaddr_can addr;
            addr.can_family  = AF_CAN;
            addr.can_ifindex = ifr.ifr_ifindex;
            err = CAN_CONNECT (m_sockfd, (sockaddr *) &addr, sizeof (addr));
            if (!err) {
                diag (Information,"SocketCAN BCM connected and configured : OK.");
                ret = true;
                m_valid = true;
                m_poller = new QSocketNotifier (m_sockfd, QSocketNotifier::Read, this);
                connect (m_poller, &QSocketNotifier::activated, this, &CanDriver_socketCanBcm::poll);
                m_poller->setEnabled (true);

                /// setup RX config to receive all whitelisted frame
                SocketCanBcmFrame bcmFrame1;
                bcmFrame1.bcmHeader.opcode  = RX_SETUP;
                bcmFrame1.bcmHeader.flags   = RX_FILTER_ID;
                bcmFrame1.bcmHeader.nframes = 0;
                int whitelistedCount = 0;
                const QVariantList whitelistedCanIds = options.value (WHITELISTED_CANIDS).toList ();
                for (QVariantList::const_iterator it = whitelistedCanIds.constBegin (); it != whitelistedCanIds.constEnd (); ++it) {
                    const quint16 canId = (* it).value<quint16> ();
                    bcmFrame1.bcmHeader.can_id  = canId;
                    if (!sendBcmMsg (&bcmFrame1)) {
                        diag (Error, "Failed to setup filtering for for CAN ID " % QString::number (canId) % " on CAN BCM socket !");
                    }
                    else {
                        whitelistedCount++;
                    }
                }
                diag (Information, "Whitelisted " % QString::number (whitelistedCount) % " CAN IDs");

                /// setup RX config to receive throtteled frames every N ms OR when payload changes
                SocketCanBcmFrame bcmFrame2;
                bcmFrame2.bcmHeader.opcode        = RX_SETUP;
                bcmFrame2.bcmHeader.flags         = (SETTIMER | STARTTIMER);
                bcmFrame2.bcmHeader.nframes       = 1;
                bcmFrame2.bcmHeader.ival1.tv_sec  = 0;
                bcmFrame2.bcmHeader.ival1.tv_usec = 0; // receive timeout error for frame : none
                bcmFrame2.bcmHeader.ival2.tv_sec  = 0;
                bcmFrame2.canFrame.can_dlc        = 8;
                bcmFrame2.canFrame.data [0]       = 0xFF;
                bcmFrame2.canFrame.data [1]       = 0xFF;
                bcmFrame2.canFrame.data [2]       = 0xFF;
                bcmFrame2.canFrame.data [3]       = 0xFF;
                bcmFrame2.canFrame.data [4]       = 0xFF;
                bcmFrame2.canFrame.data [5]       = 0xFF;
                bcmFrame2.canFrame.data [6]       = 0xFF;
                bcmFrame2.canFrame.data [7]       = 0xFF;
                int throttledCount = 0;
                const QVariantMap throttledCanIds = options.value (THROTTLED_CANIDS).toMap ();
                for (QVariantMap::const_iterator it = throttledCanIds.constBegin (); it != throttledCanIds.constEnd (); ++it) {
                    const quint16 canId    = it.key   ().toInt ();
                    const int     interval = it.value ().toInt ();
                    bcmFrame2.bcmHeader.can_id = canId;
                    bcmFrame2.bcmHeader.ival2.tv_usec = (interval * 1000); // sub-sampling/throtteling to reduce CPU load
                    bcmFrame2.canFrame.can_id = canId;
                    if (!sendBcmMsg (&bcmFrame2)) {
                        diag (Error, "Failed to setup throtteling for CAN ID " % QString::number (canId) % " socket !");
                    }
                    else {
                        throttledCount++;
                    }
                }
                diag (Information, "Throttled " % QString::number (throttledCount) % " CAN IDs");

            }
            else {
                diag (Error, "Connecting CAN BCM socket failed !");
            }
        }
        else {
            diag (Error, QStringLiteral ("Getting IF index for %1 failed").arg (ifr.ifr_name));
        }
    }
    else {
        diag (Error, "Socket creation failed !");
    }
    return ret;
}

bool CanDriver_socketCanBcm::send (CanMessage * message) {
    bool ret = false;
    if (m_valid) {
        if (message != Q_NULLPTR) {
            SocketCanBcmFrame bcmFrame;
            memset (&bcmFrame, 0, sizeof (bcmFrame));
            bcmFrame.bcmHeader.opcode = TX_SEND;
            bcmFrame.bcmHeader.nframes = 1;
            CanId canId = message->getCanId ();
            bcmFrame.canFrame.can_id = (canId.isEFF () ? canId.canIdEFF () : canId.canIdSFF ());
            if (canId.isEFF ()) {
                bcmFrame.canFrame.can_id |= CAN_EFF_FLAG;
            }
            if (canId.isRTR ()) {
                bcmFrame.canFrame.can_id |= CAN_RTR_FLAG;
            }
            if (canId.isERR ()) {
                bcmFrame.canFrame.can_id |= CAN_ERR_FLAG;
            }
            bcmFrame.canFrame.can_dlc = quint8 (message->getCanData ().getLength ());
            if ( bcmFrame.canFrame.can_dlc) {
                memcpy (bcmFrame.canFrame.data, message->getCanData ().getDataPtr (), bcmFrame.canFrame.can_dlc);
            }
            bcmFrame.bcmHeader.can_id = bcmFrame.canFrame.can_id;
            if (sendBcmMsg (&bcmFrame)) {
                ret = true;
            }
            else {
                diag (Warning, "Message send failed !");
            }
        }
        else {
            diag (Warning, "Can't send NULL message to CAN BCM socket !");
        }
    }
    else {
        diag (Warning, "Can't send message because state is not valid !");
    }
    return ret;
}

bool CanDriver_socketCanBcm::stop (void) {
    bool ret = false;
    if (m_valid) {
        m_valid = false;
        m_poller->setEnabled (false);
        CAN_CLOSE (m_sockfd);
        m_poller->deleteLater ();
        m_poller = Q_NULLPTR;
        ret = true;
    }
    return ret;
}

void CanDriver_socketCanBcm::poll (void) {
    static const quint FRAME_SIZE = sizeof (SocketCanBcmFrame);
    if (m_valid) {
        SocketCanBcmFrame bcmFrame;
        forever {
            if (CAN_RECV (m_sockfd, &bcmFrame, FRAME_SIZE, MSG_DONTWAIT) > 0) {
                if (bcmFrame.bcmHeader.opcode == RX_CHANGED) {
                    emit recv (new CanMessage (((bcmFrame.canFrame.can_id & CAN_EFF_FLAG)
                                                ? CanId (quint32 (bcmFrame.canFrame.can_id & CAN_EFF_MASK),
                                                         bool    (bcmFrame.canFrame.can_id & CAN_RTR_FLAG),
                                                         bool    (bcmFrame.canFrame.can_id & CAN_ERR_FLAG))
                                                : CanId (quint16 (bcmFrame.canFrame.can_id & CAN_SFF_MASK),
                                                         bool    (bcmFrame.canFrame.can_id & CAN_RTR_FLAG),
                                                         bool    (bcmFrame.canFrame.can_id & CAN_ERR_FLAG))),
                                               bcmFrame.canFrame.can_dlc,
                                               bcmFrame.canFrame.data));
                }
            }
            else { break; }
        }
    }
}

bool CanDriver_socketCanBcm::sendBcmMsg (SocketCanBcmFrame * msg) {
    return (CAN_SEND (m_sockfd, msg, sizeof (*msg), 0) > 0);
}

#ifndef QTCAN_STATIC_DRIVERS

QString CanDriverPlugin_socketCanBcm::getDriverName (void) {
    static const QString ret = QStringLiteral ("SocketCAN BCM");
    return ret;
}

CanDriver * CanDriverPlugin_socketCanBcm::createDriverInstance (QObject * parent) {
    return new CanDriver_socketCanBcm (parent);
}

QList<CanDriverOption *> CanDriverPlugin_socketCanBcm::optionsRequired (void) {
    QList<CanDriverOption *> ret;

    const QVariantList list = CanDriver_socketCanCommon::listAvailablesCanBuses ();
    ret << new CanDriverOption (CanDriver_socketCanBcm::BUS_NAME,
                                QStringLiteral ("CAN Bus"),
                                CanDriverOption::ListChoice,
                                (!list.isEmpty () ? list.first ().toString () : ""),
                                list);

    return ret;
}

#endif
