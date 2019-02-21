
#include "QmlCanOpenEntryRepeater.h"

#include "QmlCanOpenVarEntry.h"
#include "QmlCanOpenArrayEntry.h"
#include "QmlCanOpenRecordEntry.h"

#include <QDebug>

QmlCanOpenEntryRepeater::QmlCanOpenEntryRepeater (QObject * parent) : AbstractObdPreparator (parent)
  , m_indexFirst    (0x0000)
  , m_indexLast     (0x0000)
  , m_entryTemplate (Q_NULLPTR)
{ }

void QmlCanOpenEntryRepeater::prepareOBD (CanOpenObjDict * obd) {
    if (m_entryTemplate != Q_NULLPTR && obd != Q_NULLPTR) {
        quint num = 1;
        Incubator qmlIncubator (QQmlIncubator::Synchronous, this);
        if (!m_indexes.isEmpty ()) { // NOTE : custom list of indexes
            foreach (const CanOpenIndex idx, m_indexes) {
                qmlIncubator.prepareEntry (obd, m_entryTemplate, idx, num);
                num++;
            }
        }
        else {
            const CanOpenIndex indexFirst = qMin (m_indexFirst, m_indexLast);
            const CanOpenIndex indexLast  = qMax (m_indexFirst, m_indexLast);
            if (indexFirst > 0 && indexLast > 0 && (indexLast - indexFirst) > 0) { // NOTE : range with first/last index
                for (CanOpenIndex idx = indexFirst; idx <= indexLast; idx++, num++) {
                    qmlIncubator.prepareEntry (obd, m_entryTemplate, idx, num);
                }
            }
            else {
                emit diag (Warning, QStringLiteral ("EntryRepeater should have either valid indexFirst/indexLast, or valid list of indexes !"));
            }
        }
    }
}

void QmlCanOpenEntryRepeater::Incubator::prepareEntry (CanOpenObjDict * obd,
                                                       QQmlComponent * entryTemplate,
                                                       const CanOpenIndex idx,
                                                       const quint num) {
    currentNum = num;
    currentIndex = idx;
    entryTemplate->create (* this);
    QObject * obj = object ();
    AbstractObdPreparator * preparator = qobject_cast<AbstractObdPreparator *> (obj);
    if (preparator != Q_NULLPTR) {
        connect (preparator, &AbstractObdPreparator::diag, repeater, &AbstractObdPreparator::diag);
        preparator->prepareOBD (obd);
    }
}

void QmlCanOpenEntryRepeater::Incubator::setInitialState (QObject * object) {
    QmlCanOpenVarEntry * varEntry = qobject_cast<QmlCanOpenVarEntry *> (object);
    if (varEntry != Q_NULLPTR) {
        varEntry->setNum (int (currentNum));
        varEntry->setIndex (int (currentIndex));
    }
    else {
        QmlCanOpenArrayEntry * arrayEntry = qobject_cast<QmlCanOpenArrayEntry *> (object);
        if (arrayEntry != Q_NULLPTR) {
            arrayEntry->setNum (int (currentNum));
            arrayEntry->setIndex (int (currentIndex));
        }
        else {
            QmlCanOpenRecordEntry * recordEntry = qobject_cast<QmlCanOpenRecordEntry *> (object);
            if (recordEntry != Q_NULLPTR) {
                recordEntry->setNum (int (currentNum));
                recordEntry->setIndex (int (currentIndex));
            }
            else {
                emit repeater->diag (Warning, QStringLiteral ("EntryRepeater.entryTemplate must be one of VarEntry, ArrayEntry, or RecordEntry !"));
            }
        }
    }
}

int QmlCanOpenEntryRepeater::getIndexFirst (void) const {
    return m_indexFirst;
}

int QmlCanOpenEntryRepeater::getIndexLast (void) const {
    return m_indexLast;
}

QQmlComponent * QmlCanOpenEntryRepeater::getEntryTemplate (void) const {
    return m_entryTemplate;
}

QVariantList QmlCanOpenEntryRepeater::getIndexes (void) const {
    QVariantList ret;
    ret.reserve (m_indexes.size ());
    foreach (const CanOpenIndex index, m_indexes) {
        ret.append (index);
    }
    return ret;
}

void QmlCanOpenEntryRepeater::setIndexFirst (const int indexFirst) {
    if (indexFirst >= 0x0001 && indexFirst <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (indexFirst);
        if (m_indexFirst != tmp) {
            m_indexFirst = tmp;
            emit indexFirstChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("EntryRepeater.indexFirst should be between 1 and 65535 !"));
    }
}

void QmlCanOpenEntryRepeater::setIndexLast (const int indexLast) {
    if (indexLast >= 0x0001 && indexLast <= 0xFFFF) {
        const CanOpenIndex tmp = CanOpenIndex (indexLast);
        if (m_indexLast != tmp) {
            m_indexLast = tmp;
            emit indexLastChanged ();
        }
    }
    else {
        emit diag (Warning, QStringLiteral ("EntryRepeater.indexLast should be between 1 and 65535 !"));
    }
}

void QmlCanOpenEntryRepeater::setIndexes (const QVariantList & indexes) {
    QVector<CanOpenIndex> tmpList;
    tmpList.reserve (indexes.size ());
    foreach (const QVariant & var, indexes) {
        const int val = var.value<int> ();
        if (val >= 0x0001 && val <= 0xFFFF) {
            const CanOpenIndex tmp = CanOpenIndex (val);
            tmpList.append (tmp);
        }
        else {
            emit diag (Warning, QStringLiteral ("EntryRepeater.indexes values should be between 1 and 65535 !"));
        }
    }
    if (m_indexes != tmpList) {
        m_indexes = tmpList;
        emit indexesChanged ();
    }
}

void QmlCanOpenEntryRepeater::setEntryTemplate (QQmlComponent * objectTemplate) {
    if (m_entryTemplate != objectTemplate) {
        m_entryTemplate = objectTemplate;
        emit entryTemplateChanged ();
    }
}

