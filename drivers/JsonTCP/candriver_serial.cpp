#include "candriver_serial.h"
#include <QStringBuilder>
#include <QRegularExpression>
#include <QRegularExpressionMatch>

#define FRAME_START '{'
#define FRAME_END   '}'
#define FIELD_SEP   ','
#define PAIR_SEP    ':'
#define STR_QUOTE   '"'

#define REGEX_FRAME QRegularExpression ("{\"id\":(\\d+),\"len\":(\\d+),\"data\":\"([A-Za-z0-9=\\/\\+]+)\"}")

const QString CanDriver_Serial::PORT = QStringLiteral ("BAURATE");
const QString CanDriver_Serial::HOST = QStringLiteral ("COM");
const QString CanDriver_Serial::SERV = QStringLiteral ("isServer");

/******************************** HELPERS ***********************************/
#ifdef JSON
static QByteArray canMessageToFrame (CanMessage * message) {
    QByteArray ret;
    if (message != NULL) {
        QString frame;
        QTextStream stream (&frame, QIODevice::ReadWrite);
        stream << FRAME_START;
        stream << STR_QUOTE << "id" << STR_QUOTE << PAIR_SEP << QString::number (message->getCanId ().canIdEFF ());
        stream << FIELD_SEP;
        stream << STR_QUOTE << "len" << STR_QUOTE << PAIR_SEP << QString::number (message->getLength ());
        stream << FIELD_SEP;
        stream << STR_QUOTE << "data" << STR_QUOTE << PAIR_SEP << STR_QUOTE << message->getData ().toBase64 () << STR_QUOTE;
        stream << FRAME_END;
        stream.flush ();
        ret = frame.toUtf8 ();
    }
    else { }
    return ret;
}

static CanMessage * canMessageFromFrame (QByteArray frame) {
    CanMessage * ret = NULL;
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
#else

static uint8_t checksum( QByteArray data){
    uint8_t ret = 0;
    for (int i = 0; i < data.length() ; ++i) {
        ret = ret+ data.at(i);
    }
    ret = 0xFF - ret;
    return  ret;
}

static bool isChecksumCorrect(QByteArray data){
    uint8_t ret = 0;
    for (int i = 0; i < data.length() ; ++i) {
        ret = ret+ data.at(i);
    }
    return 0xFF == ret;
}
static QByteArray canMessageToFrame (CanMessage * message) {
    QByteArray ret;
    if (message != NULL) {
        ret.append(':');
        ret.append(message->getCanData().getLength()); //len
        uint16_t canID = message->getCanId ().canIdSFF ();
        uint8_t lo = canID & 0xFF;
        uint8_t hi = canID >> 8;
        ret.append(hi);
        ret.append(lo);
        ret.append(message->getCanData().toByteArray());
        ret.append(checksum(ret));
    }
    else { }
    return ret;
}

static CanMessage * canMessageFromFrame (QByteArray frame) {

    CanMessage * ret = NULL;
    if (isChecksumCorrect(frame)){
        if (frame.at(0)==':'){
            uint8_t len = frame.at(1);
            uint8_t lo = frame.at(3);
            uint8_t hi =  frame.at(2);
            uint16_t canID = lo | uint16_t(hi) << 8;
            QByteArray content = frame.mid(4,len);
            ret = new CanMessage (CanId (canID),
                                  len,
                                  content.data ());
        } else {}
    }
    else { }
    return ret;
}

#endif
/****************************** OOP ********************************/

CanDriver_Serial::CanDriver_Serial (QObject * parent)
    : CanDriver (parent)
{
    m_serialPort = new QSerialPort();
    connect (m_serialPort, &QSerialPort::readyRead,     this, &CanDriver_Serial::onReadyRead);
}

/********************* CAN DRIVER INTERFACE REIMPL ***********************/

bool CanDriver_Serial::init (const QVariantMap & options) {
    bool    ret  = false;
    bool    serv = options.value (SERV, false).toBool ();
    quint16 port = options.value (PORT, 0).toInt ();
    QString host = options.value (HOST, "localhost").toString ();
    if (serv) {
            diag (Information, QStringLiteral ("Run server listening on %1 with baurate %1").arg (host).arg(port));
    }
    if (port > 0 && !host.isEmpty ()) {
        diag (Information, "Initializing local serial driver with " % host % " and baurate " % QByteArray::number (port) % "...");
        m_serialPort->setPortName(host);
        m_serialPort->setBaudRate(port);
        if (m_serialPort->open(QIODevice::ReadWrite)) {
            ret = true;
            diag (Information, "Serial port driver connected.");
        }
        else {
            diag (Error, "Serial port couldn't connect ! " % m_serialPort->errorString ());
        }
    }
    else {
        diag (Error, "Can't init serial driver with COM " % host % " and baurate " % QByteArray::number (port) % " !");
    }
    return ret;
}

bool CanDriver_Serial::send (CanMessage * message) {
    bool ret = false;
    diag (Debug, "Local socket, sending message...");
    if (message != Q_NULLPTR) {
        QByteArray frame = canMessageToFrame (message);
        diag (Trace, QStringLiteral ("frame=%1").arg (QString(frame.toHex())));
        if (m_serialPort->isOpen()) {
            qint64 bytes = m_serialPort->write (frame);
            diag (Debug, QStringLiteral ("Local serial, message sent : %1 bytes written.").arg (bytes));
            m_serialPort->flush ();
            ret = true;
        }
        else {
            diag (Warning, "Can't write to serial ! Not connected.");
        }
    }
    else {
        diag (Warning, "Can't send NULL message to serial !");
    }
    return ret;
}

bool CanDriver_Serial::stop (void) {
    m_serialPort->close();
    return true;
}

/*********************** INTERNAL STUFF ****************************/

void CanDriver_Serial::onReadyRead (void) {
    diag (Debug, "Local socket, ready to read...");
    m_buffer.append (m_serialPort->readAll ());
    diag (Trace, QStringLiteral ("buffer=%1").arg (QString(m_buffer.toHex())));
    while (!m_buffer.isEmpty ()) {
        int start = m_buffer.indexOf (':');
        int end   =0;
        int len = m_buffer.at(start+1) +5;
        if (start > -1 && end > -1) {
            QByteArray frame = m_buffer.mid (start, len); // extract frame from buffer
            m_buffer.remove (0, start+len); // remove frame from buffer
            CanMessage * message = canMessageFromFrame (frame);
            if (message != Q_NULLPTR) {
                diag (Debug, "Local serial message received.");
                diag (Trace, QStringLiteral ("message cobid=%1 length=%2 data=%3")
                            .arg (QString::fromLocal8Bit (QtCAN::hexNumberStr (message->getCanId ().canIdEFF (), 8)))
                            .arg (message->getCanData ().getLength ())
                            .arg (QString::fromLocal8Bit (message->getCanData ().toByteArray ().toHex ())));
                emit recv (message);
            }
            else {
                diag (Warning, "Local serial frame mismatch !");
            }
        }
        else { break; }
    }
}


#ifndef QTCAN_STATIC_DRIVERS

QString CanDriverPlugin_Serial::getDriverName (void) {
    static const QString ret = QStringLiteral ("Serial");
    return ret;
}

CanDriver * CanDriverPlugin_Serial::createDriverInstance (QObject * parent) {
    return new CanDriver_Serial (parent);
}

QList<CanDriverOption *> CanDriverPlugin_Serial::optionsRequired (void) {
    QList<CanDriverOption *> ret;
    ret << new CanDriverOption (CanDriver_Serial::HOST, tr ("COM"),         CanDriverOption::IPv4Address, "COM3");
    ret << new CanDriverOption (CanDriver_Serial::PORT, tr ("BAURATE"),         CanDriverOption::SocketPort,  19200);
    ret << new CanDriverOption (CanDriver_Serial::SERV, tr ("Start server"), CanDriverOption::BooleanFlag, false);
    return ret;
}

#endif
