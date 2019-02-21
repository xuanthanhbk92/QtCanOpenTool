
#include "CanDriver_socketCanCommon.h"

#include <QDebug>

QVariantList CanDriver_socketCanCommon::listAvailablesCanBuses (void) {
    QVariantList list;

    int sockNetlink = CAN_SOCKET (AF_NETLINK, SOCK_RAW, NETLINK_ROUTE);
    if (sockNetlink >= 0) {

        pid_t pid = getpid ();

        sockaddr_nl addrLocalUser;
        memset (&addrLocalUser, 0, sizeof (addrLocalUser));
        addrLocalUser.nl_family = AF_NETLINK;
        addrLocalUser.nl_pid    = pid;
        addrLocalUser.nl_groups = 0;

        sockaddr_nl addrRemoteKernel;
        memset (&addrRemoteKernel, 0, sizeof (addrRemoteKernel));
        addrRemoteKernel.nl_family = AF_NETLINK;

        if (CAN_BIND (sockNetlink, (sockaddr *) &addrLocalUser, sizeof (addrLocalUser)) >= 0) {

            struct req_nl {
                nlmsghdr hdr;
                rtgenmsg gen;
            } reqNetlink;
            memset (&reqNetlink, 0, sizeof (reqNetlink));
            reqNetlink.hdr.nlmsg_len    = NLMSG_LENGTH (sizeof (reqNetlink.gen));
            reqNetlink.hdr.nlmsg_type   = RTM_GETLINK;
            reqNetlink.hdr.nlmsg_flags  = NLM_F_REQUEST | NLM_F_DUMP;
            reqNetlink.hdr.nlmsg_seq    = 1;
            reqNetlink.hdr.nlmsg_pid    = pid;
            reqNetlink.gen.rtgen_family = AF_CAN;

            iovec ioTX;
            memset (&ioTX, 0, sizeof (ioTX));
            ioTX.iov_base = &reqNetlink;
            ioTX.iov_len  = reqNetlink.hdr.nlmsg_len;

            msghdr hdrNetlinkTX;
            memset (&hdrNetlinkTX, 0, sizeof (hdrNetlinkTX));
            hdrNetlinkTX.msg_iov     = &ioTX;
            hdrNetlinkTX.msg_iovlen  = 1;
            hdrNetlinkTX.msg_name    = &addrRemoteKernel;
            hdrNetlinkTX.msg_namelen = sizeof (addrRemoteKernel);

            CAN_SENDMSG (sockNetlink, &hdrNetlinkTX, 0);

            const unsigned int IFLIST_REPLY_BUFFER = 8192;
            QByteArray buffer (IFLIST_REPLY_BUFFER, '\0');

            bool end = false;
            while (!end) {

                iovec ioRX;
                memset (&ioRX, 0, sizeof (ioRX));
                ioRX.iov_base = buffer.data ();
                ioRX.iov_len  = IFLIST_REPLY_BUFFER;

                msghdr hdrNetlinkRX;
                memset (&hdrNetlinkRX, 0, sizeof (hdrNetlinkRX));
                hdrNetlinkRX.msg_iov     = &ioRX;
                hdrNetlinkRX.msg_iovlen  = 1;
                hdrNetlinkRX.msg_name    = &addrRemoteKernel;
                hdrNetlinkRX.msg_namelen = sizeof (addrRemoteKernel);

                unsigned int msgLen = CAN_RECVMSG (sockNetlink, &hdrNetlinkRX, 0);
                if (msgLen) {
                    nlmsghdr * msg_ptr = NULL;
                    for (msg_ptr = (nlmsghdr *) buffer.data ();
                         NLMSG_OK (msg_ptr, msgLen);
                         msg_ptr = NLMSG_NEXT (msg_ptr, msgLen)) {
                        QByteArray name;
                        if (msg_ptr->nlmsg_type == NLMSG_DONE) {
                            end = true;
                        }
                        else if (msg_ptr->nlmsg_type == NLMSG_MIN_TYPE) {
                            ifinfomsg * iface = (ifinfomsg *) NLMSG_DATA (msg_ptr);

                            rtattr * attribute;
                            unsigned int infoLen = (msg_ptr->nlmsg_len - NLMSG_LENGTH (sizeof (* iface)));
                            for (attribute = IFLA_RTA (iface);
                                 RTA_OK (attribute, infoLen);
                                 attribute = RTA_NEXT (attribute, infoLen)) {
                                if (attribute->rta_type == IFLA_IFNAME) {
                                    name = QByteArray ((char *) RTA_DATA (attribute));
                                }
                            }

                            const int ARPHRD_CAN = 280; // FIXME : check if there is a standard enum somewhere
                            if (iface->ifi_type == ARPHRD_CAN) {
                                list << QString::fromLocal8Bit (name);
                            }
                        }
                    }
                }
            }

            close (sockNetlink);
        }
        else {
            qWarning () << "Couldn't bind Netlink socket, check support !";
        }
    }
    else {
        qWarning () << "Couldn't get Netlink handle !";
    }

    return list;
}
