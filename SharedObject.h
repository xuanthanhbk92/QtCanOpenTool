#ifndef SHAREDOBJECT_H
#define SHAREDOBJECT_H

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QVariant>
#include <QList>
#include <QStringList>
#include <QHash>
#include <QAbstractItemModel>
#include <QSortFilterProxyModel>

#include "QQmlObjectListModel.h"
#include "QQmlVarPropertyHelpers.h"
#include "QQmlPtrPropertyHelpers.h"
#include "QQmlConstRefPropertyHelpers.h"

#include "CanOpenDefs.h"

#include "CanDriverQmlWrapper.h"

class CanOpenProtocolManager;

class SharedObject : public QObject {
    Q_OBJECT
    QML_READONLY_VAR_PROPERTY (bool, nodeStarted)
    QML_CONSTANT_PTR_PROPERTY (CanDriverWrapper, driverWrapper)

public:
    explicit SharedObject (QObject * parent = Q_NULLPTR);

    static QString dumpFctType   (const CanOpenFctType fctType);
    static QString dumpAbortCode (const CanOpenSdoAbortCode code);
    static QString dumpFrame     (const quint16 cobId, const QByteArray & data);
    static QString dumpTransfer  (CanOpenSdoTransfer * transfer);

    bool         getDriverLoaded  (void) const;
    QString      getDriverName    (void) const;
    QVariantList getDriverOptions (void) const;
    QStringList  getDriversList   (void) const;

    Q_INVOKABLE int getRoleIndex (QAbstractItemModel * model, const QString & roleName);

public slots:
    void startNode               (const QString & mode, const int nodeId, const QString & obd);
    void sendNmtCommand          (const int nodeId, const int command);
    void changeSyncInterval      (const int interval);
    void changeHeartBeatInterval (const int interval);
    void sendSdoRequest          (const QString & operation,
                                  const QString & method,
                                  const int nodeId,
                                  const int index,
                                  const int subIndex,
                                  const int dataLen,
                                  const QString & type,
                                  const QString & value);

signals:
    void diagLogRequested (const QString & type, const QString & details);

protected slots:
    void onDiag (const int level, const QString & description);
    void onFailedToSendSdo (const QString & reason);
    void onNodeStateChanged (const CanOpenHeartBeatState state);
    void onRecvRawMsg (const quint16 cobId, const QByteArray & data);
    void onSentRawMsg (const quint16 cobId, const QByteArray & data);
    void onBeforeProcessTransfer (CanOpenSdoTransfer * transfer);
    void onAfterProcessTransfer  (CanOpenSdoTransfer * transfer);
    void onConfiguredPDOs (const QHash<CanOpenCobId,  CanOpenPdoRole> & pdosList);
    void onConfiguredSDOs (const QHash<CanOpenNodeId, CanOpenSdoMode> & sdosList);
    void onRecvSdoWriteRequest (const CanOpenNodeId nodeId,
                                const CanOpenIndex idx,
                                const CanOpenSubIndex subIdx,
                                const CanOpenSdoAbortCode statusCode,
                                const QByteArray & buffer);
    void onRecvSdoWriteReply   (const CanOpenNodeId nodeId,
                                const CanOpenIndex idx,
                                const CanOpenSubIndex subIdx,
                                const CanOpenSdoAbortCode statusCode,
                                const QByteArray & buffer);
    void onRecvSdoReadRequest  (const CanOpenNodeId nodeId,
                                const CanOpenIndex idx,
                                const CanOpenSubIndex subIdx,
                                const CanOpenSdoAbortCode statusCode,
                                const QByteArray & buffer);
    void onRecvSdoReadReply    (const CanOpenNodeId nodeId,
                                const CanOpenIndex idx,
                                const CanOpenSubIndex subIdx,
                                const CanOpenSdoAbortCode statusCode,
                                const QByteArray & buffer);

private:
    CanOpenProtocolManager * m_canOpenMan;
};

#endif // SHAREDOBJECT_H
