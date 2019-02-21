
#include "QmlCanOpenDataLayer.h"

QmlCanOpenDataLayer::QmlCanOpenDataLayer (QObject * parent) : AbstractObdPreparator (parent)
    , m_content (this)
{ }

void QmlCanOpenDataLayer::prepareOBD (CanOpenObjDict * obd) {
    foreach (AbstractObdPreparator * gen, m_content.asVector ()) {
        if (gen != Q_NULLPTR) {
            connect (gen, &AbstractObdPreparator::diag, this, &AbstractObdPreparator::diag, Qt::UniqueConnection);
            gen->prepareOBD (obd);
        }
    }
}

const QString & QmlCanOpenDataLayer::getName (void) const {
    return m_name;
}

QQmlListProperty<AbstractObdPreparator> QmlCanOpenDataLayer::getContent (void) {
    return m_content;
}

void QmlCanOpenDataLayer::setName (const QString & name) {
    if (m_name != name) {
        m_name = name;
        emit nameChanged ();
    }
}
