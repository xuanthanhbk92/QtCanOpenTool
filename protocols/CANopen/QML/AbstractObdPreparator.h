#ifndef ABSTRACTOBDPREPARATOR_H
#define ABSTRACTOBDPREPARATOR_H

#include <QObject>
#include <QVector>
#include <QQmlListProperty>
#include <qqml.h>
#include <QQmlEngine>
#include <QQmlContext>
#include <QQmlComponent>

#include "CanOpenDefs.h"

class CanOpenObjDict;

class QTCAN_CANOPEN_EXPORT AbstractObdPreparator : public QObject {
    Q_OBJECT

public:
    explicit AbstractObdPreparator (QObject * parent = Q_NULLPTR) : QObject (parent) { }

    virtual void prepareOBD (CanOpenObjDict * obd) = 0;

    enum DiagnosticLevel {
        Trace       = -2,
        Debug       = -1,
        Information =  0,
        Warning     =  1,
        Error       =  2,
    };

signals:
    void diag (const int level, const QString & msg);
};

class QTCAN_CANOPEN_EXPORT CanOpenObdPreparator : public QObject {
    Q_OBJECT

public:
    explicit CanOpenObdPreparator (QObject * parent = Q_NULLPTR) : QObject (parent) { }

    static void registerCanOpenQmlTypes (void);

public slots:
    void exposeQmlProperty (const QString & name, const QVariant & value);
    bool loadFromQmlFile   (const QString & url, CanOpenObjDict * obd);

signals:
    void diag (const int level, const QString & msg);

private:
    QQmlEngine m_qmlEngine;
};

template<class T>
class QTCAN_CANOPEN_EXPORT QmlListWrapper : public QQmlListProperty<T> {
public:
    explicit QmlListWrapper (QObject * object)
        : QQmlListProperty<T> (
              object,
              &m_vector,
              &QmlListWrapper<T>::_append,
              &QmlListWrapper<T>::_count,
              &QmlListWrapper<T>::_at,
              &QmlListWrapper<T>::_clear)
    { }
    static int _count (QQmlListProperty<T> * self) {
        QVector<T *> * vector = static_cast<QVector<T *> *> (self->data);
        if (vector != Q_NULLPTR) {
            return vector->count ();
        }
        else {
            return 0;
        }
    }
    static void _clear (QQmlListProperty<T> * self) {
        QVector<T *> * vector = static_cast<QVector<T *> *> (self->data);
        if (vector != Q_NULLPTR) {
            vector->clear ();
        }
    }
    static void _append (QQmlListProperty<T> * self, T * item) {
        QVector<T *> * vector = static_cast<QVector<T *> *> (self->data);
        if (vector != Q_NULLPTR) {
            vector->append (item);
        }
    }
    static T * _at (QQmlListProperty<T> * self, int pos) {
        QVector<T *> * vector = static_cast<QVector<T *> *> (self->data);
        if (vector != Q_NULLPTR) {
            return vector->at (pos);
        }
        else {
            return Q_NULLPTR;
        }
    }
    const QVector<T *> & asVector (void) const {
        return m_vector;
    }

private:
    QVector<T *> m_vector;
};

#endif // ABSTRACTOBDPREPARATOR_H
