
#include <QGuiApplication>
#include <QElapsedTimer>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <qqml.h>
#include <3rdparty/QtQmlTricks-UiElements/QtQmlTricksPlugin.h>
#include "SharedObject.h"

#define QTCAN_PLUGIN_DRIVERS 1

int main (int argc, char * argv []) {
    QGuiApplication app (argc, argv);
    QQmlApplicationEngine engine;
    registerQtQmlTricksUiElements (&engine);
    CanDriverWrapper::registerQmlTypes (&engine);
    static const int     maj = 2;
    static const int     min = 0;
    static const char *  uri = "QtCAN.CanOpenNodeTestUi"; // @uri QtCAN.CanOpenNodeTestUi
    static const QString msg = "!!!";
    qmlRegisterType<QSortFilterProxyModel> (uri, maj, min, "SortFilterProxyModel");
    engine.rootContext ()->setContextProperty ("Shared", new SharedObject);
    engine.load (QUrl ("qrc:///ui_CanOpenNodeTestUi.qml"));
    if (!engine.rootObjects ().isEmpty ()) {
        return app.exec ();
    }
    else {
        return -1;
    }
}
