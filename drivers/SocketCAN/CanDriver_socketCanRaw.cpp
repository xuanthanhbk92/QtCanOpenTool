
#include "CanDriver_socketCanRaw.h"
#include "CanDriver_socketCanCommon.h"

#include "CanMessage.h"

#include <QElapsedTimer>

const QString CanDriver_socketCan::BUS_NAME       = QStringLiteral ("busName");
const QString CanDriver_socketCan::USE_LOOPBACK   = QStringLiteral ("useLoopback");
const QString CanDriver_socketCan::RCV_OWN_MSG    = QStringLiteral ("canRecvOwnMsg");
const QString CanDriver_socketCan::TX_BUFFER_SIZE = QStringLiteral ("txBufferSize");
const QString CanDriver_socketCan::RX_BUFFER_SIZE = QStringLiteral ("rxBufferSize");

CanDriver_socketCan::CanDriver_socketCan (QObject * parent) : CanDriver (parent)
  , m_sockfd (-1)
  , m_valid  (false)
  , m_poller (Q_NULLPTR)
{ }

CanDriver_socketCan::~CanDriver_socketCan (void) {
    stop ();
}

bool CanDriver_socketCan::init (const QVariantMap & options) {
    bool ret = false;
    QByteArray busName = options.value (BUS_NAME).toByteArray ();
    bool isNum;
    busName.left (1).toInt (&isNum);
    if (isNum) {
        busName.prepend (QByteArrayLiteral ("can"));
    }
    m_sockfd = CAN_SOCKET (PF_CAN, SOCK_RAW, CAN_RAW);
    if (m_sockfd >= 0) {
        ifreq ifr;
        qstrncpy (ifr.ifr_name, busName.constData (), IFNAMSIZ);
        int err = 0;
        err = CAN_IOCTL (m_sockfd, SIOCGIFINDEX, &ifr);
        if (!err) {
            int loopback = options.value (USE_LOOPBACK, true).value<bool> ();
            if (CAN_SETSOCKOPT (m_sockfd, SOL_CAN_RAW, CAN_RAW_LOOPBACK, &loopback, sizeof (loopback))) {
                diag (Warning,"Set CAN socket option 'CAN_RAW_LOOPBACK' failed !");
            }
            else {
                diag (Information, QStringLiteral ("Set CAN socket option 'CAN_RAW_LOOPBACK' to %1").arg (loopback));
            }
            int recv_own_msgs = options.value (RCV_OWN_MSG, false).value<bool> ();
            if (CAN_SETSOCKOPT (m_sockfd, SOL_CAN_RAW, CAN_RAW_RECV_OWN_MSGS, &recv_own_msgs, sizeof (recv_own_msgs))) {
                diag (Warning, "Set CAN socket option 'CAN_RAW_RECV_OWN_MSGS' failed !");
            }
            else {
                diag (Information, QStringLiteral ("Set CAN socket option 'CAN_RAW_RECV_OWN_MSGS' to %1").arg (recv_own_msgs));
            }
            int drop_monitor = true;
            if (CAN_SETSOCKOPT (m_sockfd, SOL_SOCKET, SO_RXQ_OVFL, &drop_monitor, sizeof (drop_monitor))) {
                diag (Warning, "Set CAN socket option 'SO_RXQ_OVFL' failed !");
            }
            else {
                diag (Information, QStringLiteral ("Set CAN socket option 'SO_RXQ_OVFL' to %1").arg (drop_monitor));
            }
            int rx_buffer_size = options.value (RX_BUFFER_SIZE, 64).toInt ();
            if (CAN_SETSOCKOPT (m_sockfd, SOL_SOCKET, SO_RCVBUF, &rx_buffer_size, sizeof (rx_buffer_size))) {
                diag (Warning, "Set CAN socket option 'SO_RCVBUF' failed !");
            }
            else {
                int tmpVal = rx_buffer_size;
                int tmpLen = sizeof (tmpVal);
                CAN_GETSOCKOPT (m_sockfd, SOL_SOCKET, SO_RCVBUF, &tmpVal, reinterpret_cast<socklen_t *> (&tmpLen));
                diag (Information, QStringLiteral ("Set CAN socket option 'SO_RCVBUF' to %1").arg (tmpVal));
            }
            int tx_buffer_size = options.value (TX_BUFFER_SIZE, 64).toInt ();
            if (CAN_SETSOCKOPT (m_sockfd, SOL_SOCKET, SO_SNDBUF, &tx_buffer_size, sizeof (tx_buffer_size))) {
                diag (Warning, "Set CAN socket option 'SO_SNDBUF' failed !");
            }
            else {
                int tmpVal = rx_buffer_size;
                int tmpLen = sizeof (tmpVal);
                CAN_GETSOCKOPT (m_sockfd, SOL_SOCKET, SO_SNDBUF, &tmpVal, reinterpret_cast<socklen_t *> (&tmpLen));
                diag (Information, QStringLiteral ("Set CAN socket option 'SO_SNDBUF' to %1").arg (tmpVal));
            }
            timeval timeout;
            timeout.tv_sec  = 1;
            timeout.tv_usec = 0;
            if (CAN_SETSOCKOPT (m_sockfd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof (timeout))) {
                diag (Warning, "Set CAN socket option 'SO_RCVTIMEO' failed !");
            }
            sockaddr_can addr;
            addr.can_family  = AF_CAN;
            addr.can_ifindex = ifr.ifr_ifindex;
            err = CAN_BIND (m_sockfd, reinterpret_cast<sockaddr *> (&addr), sizeof (addr));
            if (!err) {
                diag (Information,"Socket connected and configured : OK.");
                ret = true;
                m_iface = QString::fromLocal8Bit (busName);
                m_valid = true;
                m_poller = new QSocketNotifier (m_sockfd, QSocketNotifier::Read, this);
                connect (m_poller, &QSocketNotifier::activated, this, &CanDriver_socketCan::poll);
                m_poller->setEnabled (true);
            }
            else {
                diag (Error, "Connecting CAN socket failed !");
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

bool CanDriver_socketCan::send (CanMessage * message) {
    bool ret = false;
    if (m_valid) {
        if (message != Q_NULLPTR) {
            can_frame frame;
            memset (&frame, 0, sizeof (frame));
            CanId canId = message->getCanId ();
            frame.can_id = (canId.isEFF () ? canId.canIdEFF () : canId.canIdSFF ());
            if (canId.isEFF ()) {
                 frame.can_id |= CAN_EFF_FLAG;
            }
            if (canId.isRTR ()) {
                frame.can_id |= CAN_RTR_FLAG;
            }
            if (canId.isERR ()) {
                frame.can_id |= CAN_ERR_FLAG;
            }
            frame.can_dlc = quint8 (message->getCanData ().getLength ());
            if (frame.can_dlc) {
                memcpy (frame.data, message->getCanData ().getDataPtr (), frame.can_dlc);
            }
            if (CAN_WRITE (m_sockfd, &frame, sizeof (frame)) > 0) {
                ret = true;
            }
            else {
                diag (Warning, "Message send failed !");
            }
        }
        else {
            diag (Warning, "Can't send NULL message to CAN socket !");
        }
    }
    else {
        diag (Warning, "Can't send message because state is not valid !");
    }
    return ret;
}

bool CanDriver_socketCan::stop (void) {
    bool ret = false;
    if (m_valid) {
        m_valid = false;
        m_iface.clear ();
        m_poller->setEnabled (false);
        CAN_CLOSE (m_sockfd);
        m_poller->deleteLater ();
        m_poller = Q_NULLPTR;
        ret = true;
    }
    return ret;
}

void CanDriver_socketCan::poll (void) {
    can_frame frame;
    const quint FRAME_SIZE = sizeof (frame);
    forever {
        if (CAN_RECV (m_sockfd, &frame, FRAME_SIZE, MSG_DONTWAIT) > 0) {
            emit recv (new CanMessage (((frame.can_id & CAN_EFF_FLAG)
                                        ? CanId (quint32 (frame.can_id & CAN_EFF_MASK),
                                                 bool    (frame.can_id & CAN_RTR_FLAG),
                                                 bool    (frame.can_id & CAN_ERR_FLAG))
                                        : CanId (quint16 (frame.can_id & CAN_SFF_MASK),
                                                 bool    (frame.can_id & CAN_RTR_FLAG),
                                                 bool    (frame.can_id & CAN_ERR_FLAG))),
                                       frame.can_dlc,
                                       frame.data));
        }
        else { break; }
    }
}

#ifndef QTCAN_STATIC_DRIVERS

QString CanDriverPlugin_socketCanRaw::getDriverName (void) {
    static const QString ret = QStringLiteral ("SocketCAN RAW");
    return ret;
}

CanDriver * CanDriverPlugin_socketCanRaw::createDriverInstance (QObject * parent) {
    return new CanDriver_socketCan (parent);
}

QList<CanDriverOption *> CanDriverPlugin_socketCanRaw::optionsRequired (void) {
    QList<CanDriverOption *> ret;

    const QVariantList list = CanDriver_socketCanCommon::listAvailablesCanBuses ();

    ret << new CanDriverOption (CanDriver_socketCan::BUS_NAME,
                                QStringLiteral ("CAN Bus"),
                                CanDriverOption::ListChoice,
                                (!list.isEmpty () ? list.first ().toString () : ""),
                                list);

    ret << new CanDriverOption (CanDriver_socketCan::USE_LOOPBACK,
                                QStringLiteral ("Use loopback"),
                                CanDriverOption::BooleanFlag,
                                true);

    ret << new CanDriverOption (CanDriver_socketCan::RCV_OWN_MSG,
                                QStringLiteral ("Receive own messages"),
                                CanDriverOption::BooleanFlag,
                                false);

    return ret;
}

#endif
