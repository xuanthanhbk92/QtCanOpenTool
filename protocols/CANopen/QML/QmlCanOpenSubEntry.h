#ifndef QMLCANOPENSUBENTRY_H
#define QMLCANOPENSUBENTRY_H

#include <QObject>

#include "CanOpenDefs.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenSubEntry : public QObject {
    Q_OBJECT
    Q_PROPERTY (int         dataType   READ getDataType   WRITE setDataType   NOTIFY dataTypeChanged)
    Q_PROPERTY (int         dataLen    READ getDataLen    WRITE setDataLen    NOTIFY dataLenChanged)
    Q_PROPERTY (int         attributes READ getAttributes WRITE setAttributes NOTIFY attributesChanged)
    Q_PROPERTY (QVariant    data       READ getData       WRITE setData       NOTIFY dataChanged)
    Q_PROPERTY (QVariantMap metaData   READ getMetaData   WRITE setMetaData   NOTIFY metaDataChanged)

public:
    explicit QmlCanOpenSubEntry (QObject * parent = Q_NULLPTR);

    int         getDataType   (void) const;
    int         getDataLen    (void) const;
    int         getAttributes (void) const;
    QVariant    getData       (void) const;
    QVariantMap getMetaData   (void) const;

public slots:
    void setDataType   (const int dataType);
    void setDataLen    (const int dataLen);
    void setAttributes (const int attributes);
    void setData       (const QVariant & data);
    void setMetaData   (const QVariantMap & metaData);

signals:
    void dataTypeChanged   (void);
    void dataLenChanged    (void);
    void attributesChanged (void);
    void dataChanged       (void);
    void metaDataChanged   (void);

private:
    CanOpenDataType m_dataType;
    CanOpenDataLen m_dataLen;
    CanOpenAccessMode m_attributes;
    QVariant m_data;
    QVariantMap m_metaData;
};

#endif // QMLCANOPENSUBENTRY_H
