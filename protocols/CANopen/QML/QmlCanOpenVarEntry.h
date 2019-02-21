#ifndef QMLCANOPENVARENTRY_H
#define QMLCANOPENVARENTRY_H

#include <QObject>
#include <QVariantMap>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenVarEntry : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int         num        READ getNum        WRITE setNum        NOTIFY numChanged)
    Q_PROPERTY (int         index      READ getIndex      WRITE setIndex      NOTIFY indexChanged)
    Q_PROPERTY (int         dataType   READ getDataType   WRITE setDataType   NOTIFY dataTypeChanged)
    Q_PROPERTY (int         dataLen    READ getDataLen    WRITE setDataLen    NOTIFY dataLenChanged)
    Q_PROPERTY (int         attributes READ getAttributes WRITE setAttributes NOTIFY attributesChanged)
    Q_PROPERTY (QVariant    data       READ getData       WRITE setData       NOTIFY dataChanged)
    Q_PROPERTY (QVariantMap metaData   READ getMetaData   WRITE setMetaData   NOTIFY metaDataChanged)

public:
    explicit QmlCanOpenVarEntry (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int         getNum        (void) const;
    int         getIndex      (void) const;
    int         getDataType   (void) const;
    int         getDataLen    (void) const;
    int         getAttributes (void) const;
    QVariant    getData       (void) const;
    QVariantMap getMetaData   (void) const;

public slots:
    void setNum        (const int num);
    void setIndex      (const int index);
    void setDataType   (const int dataType);
    void setDataLen    (const int dataLen);
    void setAttributes (const int attributes);
    void setData       (const QVariant & data);
    void setMetaData   (const QVariantMap & metaData);

signals:
    void numChanged        (void);
    void indexChanged      (void);
    void dataTypeChanged   (void);
    void dataLenChanged    (void);
    void attributesChanged (void);
    void dataChanged       (void);
    void metaDataChanged   (void);

private:
    CanOpenCounter m_num;
    CanOpenIndex m_index;
    CanOpenDataType m_dataType;
    CanOpenDataLen m_dataLen;
    CanOpenAccessMode m_attributes;
    QVariant m_data;
    QVariantMap m_metaData;
};

#endif // QMLCANOPENVARENTRY_H
