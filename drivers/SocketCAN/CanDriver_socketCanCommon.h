
#include <QString>
#include <QVariantList>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <linux/can.h>
#include <linux/can/bcm.h>
#include <linux/can/raw.h>
#include <linux/can/netlink.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include <net/if.h>
#include <net/if_arp.h>

#define CAN_BIND       ::bind
#define CAN_CLOSE      ::close
#define CAN_CONNECT    ::connect
#define CAN_IOCTL      ::ioctl
#define CAN_RECVMSG    ::recvmsg
#define CAN_RECV       ::recv
#define CAN_SENDMSG    ::sendmsg
#define CAN_SEND       ::send
#define CAN_SETSOCKOPT ::setsockopt
#define CAN_GETSOCKOPT ::getsockopt
#define CAN_SOCKET     ::socket
#define CAN_WRITE      ::write

#ifndef CAN_MAX_DLEN
#   define CAN_MAX_DLEN 8
#endif

#define QTCAN_DRIVER_EXPORT
class QTCAN_DRIVER_EXPORT CanDriver_socketCanCommon {
public:
    static QVariantList listAvailablesCanBuses (void);
};
