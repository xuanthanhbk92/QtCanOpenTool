
#include "CanDriver_jsonTcp.h"

#include <QStringBuilder>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#define FRAME_START '{'
#define FRAME_END   '}'
#define FIELD_SEP   ','
#define PAIR_SEP    ':'
#define STR_QUOTE   '"'

#define REGEX_FRAME QRegularExpression ("{\"id\":(\\d+),\"len\":(\\d+),\"data\":\"([A-Za-z0-9=\\/\\+]+)\"}")

const QString CanDriver_jsonTcp::PORT = QStringLiteral ("port");
const QString CanDriver_jsonTcp::HOST = QStringLiteral ("host");
const QString CanDriver_jsonTcp::SERV = QStringLiteral ("isServer");

/******************************** HELPERS ***********************************/

static QByteArray canMessageToFrame (CanMessage * message) {
    QByteArray ret;
    if (message != Q_NULLPTR) {
        QString frame;
        QTextStream stream (&frame, QIODevice::ReadWrite);
        stream << FRAME_START;
        stream << STR_QUOTE << "id" << STR_QUOTE << PAIR_SEP << QString::number (message->getCanId ().canIdEFF ());
        stream << FIELD_SEP;
        stream << STR_QUOTE << "len" << STR_QUOTE << PAIR_SEP << QString::number (message->getCanData ().getLength ());
        stream << FIELD_SEP;
        stream << STR_QUOTE << "data" << STR_QUOTE << PAIR_SEP << STR_QUOTE << message->getCanData ().toByteArray ().toBase64 () << STR_QUOTE;
        stream << FRAME_END;
        stream.flush ();
        ret = frame.toUtf8 ();
    }
    else { }
    return ret;
}

static CanMessage * canMessageFromFrame (QByteArray frame) {
    CanMessage * ret = Q_NULLPTR;
    QStringList match = REGEX_FRAME.match (QString::fromUtf8 (frame)).capturedTexts ();
    if (match.count () == 4) {
        QByteArray content = QByteArray::fromBase64 (match.at (3).toLocal8Bit ());
        ret = new CanMessage (CanId (quint16 (match.at (1).toInt ())),
                              content.size (),
                              content.data ());
    }
    else { }
    return ret;
}

/****************************** OOP ********************************/

CanDriver_jsonTcp::CanDriver_jsonTcp (QObject * parent)
    : CanDriver (parent)
    , m_socket  (Q_NULLPTR)
    , m_server  (Q_NULLPTR)
{
    m_socket = new QTcpSocket (this);
    m_server = new QTcpServer (this);
    connect (m_socket, &QTcpSocket::readyRead,     this, &CanDriver_jsonTcp::onSocketReadyRead);
    connect (m_server, &QTcpServer::newConnection, this, &CanDriver_jsonTcp::onClientConnected);
}

/********************* CAN DRIVER INTERFACE REIMPL ***********************/

bool CanDriver_jsonTcp::init (const QVariantMap & options) {
    bool    ret  = false;
    bool    serv = options.value (SERV, false).toBool ();
    quint16 port = options.value (PORT, 0).toInt ();
    QString host = options.value (HOST, "localhost").toString ();
    if (serv) {
        if (m_server->listen (QHostAddress::AnyIPv4, port)) {
            port = m_server->serverPort ();
            host = QStringLiteral ("localhost");
            diag (Information, QStringLiteral ("Local server listening on port %1").arg (port));
        }
        else {
            diag (Error, QStringLiteral ("Couldn't create local server on port %1").arg (port));
        }
    }
    if (port > 0 && !host.isEmpty ()) {
        diag (Information, "Initializing local socket driver with host " % host % " and port " % QByteArray::number (port) % "...");
        m_socket->connectToHost (host, port, QIODevice::ReadWrite, QAbstractSocket::IPv4Protocol);
        if (m_socket->waitForConnected (1000)) {
            ret = true;
            diag (Information, "Local socket driver connected.");
        }
        else {
            diag (Error, "Local socket couldn't connect ! " % m_socket->errorString ());
        }
    }
    else {
        diag (Error, "Can't init local socket driver with host " % host % " and port " % QByteArray::number (port) % " !");
    }
    return ret;
}

bool CanDriver_jsonTcp::send (CanMessage * message) {
    bool ret = false;
    diag (Debug, "Local socket, sending message...");
    if (message != Q_NULLPTR) {
        QByteArray frame = canMessageToFrame (message);
        diag (Trace, QStringLiteral ("frame=%1").arg (frame.constData ()));
        if (m_socket->state () == QAbstractSocket::ConnectedState) {
            qint64 bytes = m_socket->write (frame);
            diag (Debug, QStringLiteral ("Local socket, message sent : %1 bytes written.").arg (bytes));
            m_socket->flush ();
            ret = true;
        }
        else {
            diag (Warning, "Can't write to local socket ! Not connected.");
        }
    }
    else {
        diag (Warning, "Can't send NULL message to local socket !");
    }
    return ret;
}

bool CanDriver_jsonTcp::stop (void) {
    m_socket->close ();
    m_server->close ();
    return true;
}

/*********************** INTERNAL STUFF ****************************/

void CanDriver_jsonTcp::onSocketReadyRead (void) {
    diag (Debug, "Local socket, ready to read...");
    m_buffer.append (m_socket->readAll ());
    diag (Trace, QStringLiteral ("buffer=%1").arg (QString::fromLocal8Bit (m_buffer)));
    while (!m_buffer.isEmpty ()) {
        int start = m_buffer.indexOf (FRAME_START);
        int end   = m_buffer.indexOf (FRAME_END, start);
        if (start > -1 && end > -1) {
            QByteArray frame = m_buffer.mid (start, end - start +1); // extract frame from buffer
            m_buffer.remove (0, end +1); // remove frame from buffer
            CanMessage * message = canMessageFromFrame (frame);
            if (message != Q_NULLPTR) {
                diag (Debug, "Local socket message received.");
                diag (Trace, QStringLiteral ("message cobid=%1 length=%2 data=%3")
                            .arg (QString::fromLocal8Bit (QtCAN::hexNumberStr (message->getCanId ().canIdEFF (), 8)))
                            .arg (message->getCanData ().getLength ())
                            .arg (QString::fromLocal8Bit (message->getCanData ().toByteArray ().toHex ())));
                emit recv (message);
            }
            else {
                diag (Warning, "Local socket frame mismatch !");
            }
        }
        else { break; }
    }
}

void CanDriver_jsonTcp::onClientConnected (void) {
    while (m_server->hasPendingConnections ()) {
        QTcpSocket * client = m_server->nextPendingConnection ();
        connect (client, &QTcpSocket::readyRead,    this, &CanDriver_jsonTcp::onClientSentData);
        connect (client, &QTcpSocket::disconnected, this, &CanDriver_jsonTcp::onClientDisconnected);
        m_clients.append (client);
    }
}

void CanDriver_jsonTcp::onClientDisconnected (void) {
    QTcpSocket * client = qobject_cast<QTcpSocket *> (sender ());
    if (client != Q_NULLPTR) {
        m_clients.removeAll (client);
    }
}

void CanDriver_jsonTcp::onClientSentData (void) {
    QTcpSocket * client = qobject_cast<QTcpSocket *> (sender ());
    if (client != Q_NULLPTR) {
        QByteArray data = client->readAll ();
        foreach (QTcpSocket * sock, m_clients) {
            if (sock && sock != client) {
                sock->write (data);
                sock->flush ();
            }
        }
    }
}

#ifndef QTCAN_STATIC_DRIVERS

QString CanDriverPlugin_jsonTcp::getDriverName (void) {
    static const QString ret = QStringLiteral ("JSON/TCP");
    return ret;
}

CanDriver * CanDriverPlugin_jsonTcp::createDriverInstance (QObject * parent) {
    return new CanDriver_jsonTcp (parent);
}

QList<CanDriverOption *> CanDriverPlugin_jsonTcp::optionsRequired (void) {
    QList<CanDriverOption *> ret;
    ret << new CanDriverOption (CanDriver_jsonTcp::HOST, tr ("Host"),         CanDriverOption::IPv4Address, "localhost");
    ret << new CanDriverOption (CanDriver_jsonTcp::PORT, tr ("Port"),         CanDriverOption::SocketPort,  0);
    ret << new CanDriverOption (CanDriver_jsonTcp::SERV, tr ("Start server"), CanDriverOption::BooleanFlag, false);
    return ret;
}

#endif
