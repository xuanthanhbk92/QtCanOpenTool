#ifndef QMLCANOPENARRAYENTRY_H
#define QMLCANOPENARRAYENTRY_H

#include <QObject>
#include <QString>
#include <QQmlScriptString>

#include "CanOpenDefs.h"
#include "AbstractObdPreparator.h"

class QTCAN_CANOPEN_EXPORT QmlCanOpenArrayEntry : public AbstractObdPreparator {
    Q_OBJECT
    Q_PROPERTY (int              num        READ getNum        WRITE setNum        NOTIFY numChanged)
    Q_PROPERTY (int              index      READ getIndex      WRITE setIndex      NOTIFY indexChanged)
    Q_PROPERTY (int              dataType   READ getDataType   WRITE setDataType   NOTIFY dataTypeChanged)
    Q_PROPERTY (int              dataLen    READ getDataLen    WRITE setDataLen    NOTIFY dataLenChanged)
    Q_PROPERTY (int              attributes READ getAttributes WRITE setAttributes NOTIFY attributesChanged)
    Q_PROPERTY (int              count      READ getCount      WRITE setCount      NOTIFY countChanged)
    Q_PROPERTY (QQmlScriptString metaData   READ getMetaData   WRITE setMetaData   NOTIFY metaDataChanged)

public:
    explicit QmlCanOpenArrayEntry (QObject * parent = Q_NULLPTR);

    void prepareOBD (CanOpenObjDict * obd);

    int getNum        (void) const;
    int getIndex      (void) const;
    int getDataType   (void) const;
    int getDataLen    (void) const;
    int getAttributes (void) const;
    int getCount      (void) const;

    const QQmlScriptString & getMetaData (void) const;

public slots:
    void setNum        (const int num);
    void setIndex      (const int index);
    void setDataType   (const int dataType);
    void setDataLen    (const int dataLen);
    void setAttributes (const int attributes);
    void setCount      (const int count);
    void setMetaData   (const QQmlScriptString & metaData);

signals:
    void numChanged        (void);
    void indexChanged      (void);
    void dataTypeChanged   (void);
    void dataLenChanged    (void);
    void attributesChanged (void);
    void countChanged      (void);
    void metaDataChanged   (void);

private:
    CanOpenCounter m_num;
    CanOpenIndex m_index;
    CanOpenDataType m_dataType;
    CanOpenDataLen m_dataLen;
    CanOpenAccessMode m_attributes;
    CanOpenSubIndex m_count;
    QQmlScriptString m_metaData;
};

#endif // QMLCANOPENARRAYENTRY_H
