import QtQuick 2.1;
import QtQuick.Window 2.1;
import Qt.labs.settings 1.0
import Qt.labs.folderlistmodel 2.1;
import QtQmlTricks.UiElements 2.0;
import QtCAN.CanOpenNodeTestUi 2.0;
import QtCAN.Utils 2.0;

Window {
    title: "Qt CAN 2.0 - CANopen Node test UI";
    color: Style.colorWindow;
    width: 1280;
    height: 600;
    visible: true;
    minimumWidth: 800;
    minimumHeight: 400;
    Component.onDestruction: { Qt.quit (); }

    Item {
        states: State {
            when: (Style !== null);

            PropertyChanges {
                target: Style;
                useDarkTheme: false;
                themeFadeTime: 0;
                fontSizeNormal: 15;
                spacingNormal: 6;
            }
        }
    }
    Timer {
        repeat: true;
        running: (toggleAutoScroll.checked && proxy.dirty);
        interval: (1000 / 20);
        triggeredOnStart: true;
        onTriggered: {
            list.contentY = (list.contentHeight - list.height);
            proxy.dirty = false;
        }
    }
    Settings {
        property alias localNodeId    : inputNodeId.text;
        property alias localNodeMode  : comboMode.currentIdx;
        property alias localQmlObdUrl : inputObdUrl.text;
    }
    ListModel {
        id: modelLogs;
        onCountChanged: { proxy.dirty = true; }
        Component.onCompleted: {
            Shared.diagLogRequested.connect (function (type, details) {
                modelLogs.append ({ "type" : type, "details" : details });
            });
        }

        ListElement { type: ""; details: ""; }
    }
    ToolBar {
        id: toolBar;

        TextButton {
            id: btnClearLogs;
            text: qsTr ("Clear logs");
            icon: SvgIconLoader { icon: "actions/clear"; }
            visible: Shared.nodeStarted;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { modelLogs.clear (); }
        }
        Stretcher { }
        TextButton {
            id: btnScrollToTop;
            text: qsTr ("Scroll to top");
            icon: SvgIconLoader { icon: "actions/arrow-top"; }
            visible: Shared.nodeStarted;
            enabled: !toggleAutoScroll.checked;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { list.contentY = 0; }
        }
        TextButton {
            id: btnScrollToBottom;
            text: qsTr ("Scroll to bottom");
            icon: SvgIconLoader { icon: "actions/arrow-bottom"; }
            visible: Shared.nodeStarted;
            enabled: !toggleAutoScroll.checked;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { list.contentY = (list.contentHeight - list.height); }
        }
        TextButton {
            id: toggleAutoScroll;
            text: qsTr ("Auto-scroll");
            icon: SvgIconLoader { icon: "actions/lock"; }
            checked: true;
            visible: Shared.nodeStarted;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        Stretcher {
            visible: sidePanel.visible;
            implicitWidth: (sidePanel.expanded ? sidePanel.size : sidePanel.minifiedSize);
        }
    }
    StatusBar {
        id: statusBar;

        TextLabel {
            text: qsTr ("Message filter");
            visible: Shared.nodeStarted;
            anchors.verticalCenter: parent.verticalCenter;
        }
        TextButton {
            id: toggleErrors;
            text: "ERROR";
            checked: true;
            visible: Shared.nodeStarted;
            textFont.pixelSize: Style.fontSizeSmall;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        TextButton {
            id: toggleWarnings;
            text: "WARNING";
            checked: true;
            visible: Shared.nodeStarted;
            textFont.pixelSize: Style.fontSizeSmall;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        TextButton {
            id: toggleInfos;
            text: "INFO";
            checked: true;
            visible: Shared.nodeStarted;
            textFont.pixelSize: Style.fontSizeSmall;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        TextButton {
            id: toggleDebugs;
            text: "DEBUG";
            checked: true;
            visible: Shared.nodeStarted;
            textFont.pixelSize: Style.fontSizeSmall;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        TextButton {
            id: toggleTraces;
            text: "TRACE";
            checked: false;
            visible: Shared.nodeStarted;
            textFont.pixelSize: Style.fontSizeSmall;
            anchors.verticalCenter: parent.verticalCenter;
            onClicked: { checked = !checked; }
        }
        Stretcher { }
    }
    Item {
        id: workspace;
        anchors {
            top: toolBar.bottom;
            bottom: statusBar.top;
        }
        ExtraAnchors.horizontalFill: parent;

        Item {
            id: centralArea;
            anchors.right: (sidePanel.visible ? sidePanel.left : parent.right);
            ExtraAnchors.leftDock: parent;

            DriverSelector {
                id: driverSelector;
                driverWrapper: Shared.driverWrapper;
                anchors.centerIn: parent;
            }
            DriverOptionsForm {
                id: driverOptForm;
                driverWrapper: Shared.driverWrapper;
                anchors.centerIn: parent;
            }
            StretchColumnContainer {
                id: nodeSetupForm;
                width: Style.realPixels (500);
                spacing: Style.spacingBig;
                visible: (Shared.driverWrapper.driverName !== "" && Shared.driverWrapper.driverLoaded);
                anchors.centerIn: parent;

                TextLabel {
                    text: qsTr ("CANopen node setup");
                    horizontalAlignment: Text.AlignHCenter;
                    font.pixelSize: Style.fontSizeBig;
                }
                Line { }
                FormContainer {
                    colSpacing: Style.spacingNormal;
                    rowSpacing: Style.spacingNormal;

                    TextLabel {
                        text: qsTr ("Node ID :");
                    }
                    TextBox {
                        id: inputNodeId;

                        readonly property int value : {
                            var ret = 0;
                            var tmp = parseInt (text);
                            if (!isNaN (tmp)) {
                                ret = tmp;
                            }
                            return ret;
                        }
                    }
                    TextLabel {
                        text: qsTr ("Node type :");
                    }
                    ComboList {
                        id: comboMode;
                        currentIdx: 0;
                        model: ListModel {
                            ListElement { key: "SLAVE";  value: "Slave"; }
                            ListElement { key: "MASTER"; value: "Master"; }
                        }
                    }
                    TextLabel {
                        text: qsTr ("QML OBD file URL :");
                    }
                    StretchRowContainer {
                        spacing: Style.spacingNormal;

                        MultiLineTextBox  {
                            id: inputObdUrl;
                            implicitWidth: -1;
                            anchors.verticalCenter: parent.verticalCenter;
                        }
                        TextButton {
                            id: btnBrowseQmlFile;
                            text: "...";
                            anchors.verticalCenter: parent.verticalCenter;
                            onClicked: {
                                var tmp = inputObdUrl.text.trim ();
                                var props = {};
                                if (tmp !== "") {
                                    props ["folder"] = tmp.substring (0, tmp.lastIndexOf ('/'));
                                }
                                compoDlg.createObject (Introspector.window (btnBrowseQmlFile), props);
                            }

                            Component {
                                id: compoDlg;

                                ModalDialog {
                                    id: dialog;
                                    title: qsTr ("Select a QML OBD");
                                    buttons: (buttonAccept | buttonCancel);
                                    minWidth: 650;
                                    maxWidth: 650;
                                    onButtonClicked: {
                                        switch (buttonType) {
                                        case buttonAccept:
                                            if (fsel.currentPath.trim () !== "") {
                                                inputObdUrl.text = fsel.currentPath;
                                                hide ();
                                            }
                                            else {
                                                shake ();
                                            }
                                            break;
                                        case buttonCancel:
                                            hide ();
                                            break
                                        }
                                    }

                                    FileSelector {
                                        id: fsel;
                                        nameFilters: ["*.qml"];
                                        implicitHeight: 300;
                                    }
                                }
                            }
                        }
                    }
                }
                StretchRowContainer {
                    spacing: 0;

                    Stretcher { }
                    TextButton {
                        text: qsTr ("Start node !");
                        icon: SymbolLoader { symbol: Style.symbolCheck; }
                        onClicked: { Shared.startNode (comboMode.currentKey, inputNodeId.value, inputObdUrl.text); }
                    }
                    Stretcher { }
                }
            }
            ScrollContainer {
                id: logView;
                visible: (Shared.nodeStarted && Shared.driverWrapper.driverName !== "" && Shared.driverWrapper.driverLoaded);
                showBorder: false;
                indicatorOnly: toggleAutoScroll.checked;
                anchors.fill: parent;

                ListView {
                    id: list;
                    model: SortFilterProxyModel {
                        id: proxy;
                        filterRole: Shared.getRoleIndex (modelLogs, "type");
                        sourceModel: modelLogs;
                        filterRegExp: new RegExp (pattern);
                        filterKeyColumn: 0;
                        dynamicSortFilter: true;
                        onPatternChanged: { dirty = true; }

                        property bool dirty : false;

                        readonly property string pattern : {
                            var tmp = [];
                            for (var type in toggles) {
                                if (toggles [type].checked) {
                                    tmp.push (type);
                                }
                            }
                            return ("^(" + tmp.join ("|") + ")$");
                        }

                        readonly property var colors : { "ERROR"   : Style.colorError,
                                                         "WARNING" : Style.colorError,
                                                         "INFO"    : Style.colorLink,
                                                         "DEBUG"   : Style.colorValid,
                                                         "TRACE"   : Style.colorForeground,
                        };

                        readonly property var toggles : { "ERROR"   : toggleErrors,
                                                          "WARNING" : toggleWarnings,
                                                          "INFO"    : toggleInfos,
                                                          "DEBUG"   : toggleDebugs,
                                                          "TRACE"   : toggleTraces,
                        };
                    }
                    delegate: TextLabel {
                        text: "<b>%1 - %2</b> : %3".arg (model ["index"]).arg (model ["type"]).arg (model ["details"]);
                        color: (proxy.colors [model ["type"]] || Style.colorBorder);
                        textFormat: Text.StyledText;
                    }
                }
            }
        }
        PanelContainer {
            id: sidePanel;
            size: 350;
            title: qsTr ("Toolbox");
            visible: (!detached && Shared.nodeStarted);
            borderSide: Item.Left;
            resizable: false;
            detachable: true;
            collapsable: true;
            ExtraAnchors.rightDock: parent;

            Accordion {
                anchors.fill: parent;

                Group {
                    title: qsTr ("SDO request");

                    FormContainer {
                        colSpacing: Style.spacingNormal;
                        rowSpacing: Style.spacingNormal;
                        anchors.fill: parent;
                        anchors.margins: Style.spacingBig;

                        TextLabel {
                            text: qsTr ("Operation");
                        }
                        ComboList {
                            id: comboOperation;
                            currentIdx: 0;
                            model: ListModel {
                                ListElement { key: "READ";  value: "Read";  }
                                ListElement { key: "WRITE"; value: "Write"; }
                            }
                        }
                        TextLabel {
                            text: qsTr ("Method");
                        }
                        ComboList {
                            id: comboMethod;
                            currentIdx: 0;
                            model: ListModel {
                                ListElement { key: "EXP/SEG"; value: "Expedited / Segmented"; }
                                ListElement { key: "BLK";     value: "Block Transfer";        }
                            }
                        }
                        TextLabel {
                            text: qsTr ("Node ID");
                        }
                        TextBox {
                            id: inputNode;
                            text: "1";

                            readonly property int value : {
                                var ret = -1;
                                var tmp = parseInt (text);
                                if (!isNaN (tmp)) {
                                    ret = tmp;
                                }
                                return ret;
                            }
                        }
                        TextLabel {
                            text: qsTr ("Index");
                        }
                        TextBox {
                            id: inputIdx;
                            text: "0x1000";

                            readonly property int value : {
                                var ret = -1;
                                var tmp = parseInt (text);
                                if (!isNaN (tmp)) {
                                    ret = tmp;
                                }
                                return ret;
                            }
                        }
                        TextLabel {
                            text: qsTr ("Sub-index");
                        }
                        TextBox {
                            id: inputSubIdx;
                            text: "0";

                            readonly property int value : {
                                var ret = -1;
                                var tmp = parseInt (text);
                                if (!isNaN (tmp)) {
                                    ret = tmp;
                                }
                                return ret;
                            }
                        }
                        TextLabel {
                            text: qsTr ("Data length");
                            visible: (comboOperation.currentKey === "WRITE");
                        }
                        TextBox {
                            id: inputLen;
                            text: "4";
                            visible: (comboOperation.currentKey === "WRITE");

                            readonly property int value : {
                                var ret = -1;
                                var tmp = parseInt (text);
                                if (!isNaN (tmp)) {
                                    ret = tmp;
                                }
                                return ret;
                            }
                        }
                        TextLabel {
                            text: qsTr ("Input type");
                            visible: (comboOperation.currentKey === "WRITE");
                        }
                        ComboList {
                            id: comboType;
                            visible: (comboOperation.currentKey === "WRITE");
                            currentIdx: 0;
                            model: ListModel {
                                ListElement { key: "NUMBER"; value: "Number";    }
                                ListElement { key: "BYTES";  value: "Raw bytes"; }
                            }
                        }
                        TextLabel {
                            text: qsTr ("Input value");
                            visible: (comboOperation.currentKey === "WRITE");
                        }
                        TextBox {
                            id: inputValue;
                            visible: (comboOperation.currentKey === "WRITE");
                        }
                        Stretcher {
                            visible: (comboOperation.currentKey === "WRITE");
                        }
                        TextLabel {
                            text: (comboType.currentKey === "NUMBER"
                                   ? qsTr ("(decimal or hex, based on prefix)")
                                   : qsTr ("(will be completed with zeroes)"));
                            visible: (comboOperation.currentKey === "WRITE");
                            horizontalAlignment: Text.AlignHCenter;
                            font.pixelSize: Style.fontSizeSmall;
                        }
                        Stretcher { }
                        TextButton {
                            text: qsTr ("Send");
                            onClicked: {
                                var operation = comboOperation.currentKey;
                                var method    = comboMethod.currentKey;
                                var nodeId    = inputNode.value;
                                var index     = inputIdx.value;
                                var subIndex  = inputSubIdx.value;
                                var dataLen   = inputLen.value;
                                var type      = comboType.currentKey;
                                var value     = inputValue.text.trim ();
                                Shared.sendSdoRequest (operation, method, nodeId, index, subIndex, dataLen, type, value);
                            }
                        }
                    }
                }
                Group {
                    title: qsTr ("NMT commands");

                    StretchColumnContainer {
                        anchors {
                            fill: parent;
                            margins: Style.spacingBig;
                        }

                        FormContainer {
                            colSpacing: Style.spacingNormal;
                            rowSpacing: Style.spacingNormal;

                            TextLabel {
                                text: qsTr ("Send NMT command");
                            }
                            TextBox {
                                id: inputNmtCommand;
                                text: "81";
                                implicitWidth: -1;

                                readonly property int value : {
                                    var ret = -1;
                                    var tmp = parseInt (text);
                                    if (!isNaN (tmp)) {
                                        ret = tmp;
                                    }
                                    return ret;
                                }
                            }
                            TextLabel {
                                text: qsTr ("to target node");
                            }
                            TextBox {
                                id: inputNmtNode;
                                text: "0";
                                implicitWidth: -1;

                                readonly property int value : {
                                    var ret = -1;
                                    var tmp = parseInt (text);
                                    if (!isNaN (tmp)) {
                                        ret = tmp;
                                    }
                                    return ret;
                                }
                            }
                            Stretcher { }
                            TextButton {
                                text: qsTr ("Send");
                                onClicked: { Shared.sendNmtCommand (inputNmtNode.value, inputNmtCommand.value); }
                            }
                        }
                        Stretcher { }
                        Balloon {
                            title: qsTr ("Memento");
                            content: [
                                qsTr ("0x01 = Start node"),
                                qsTr ("0x02 = Stop node"),
                                qsTr ("0x80 = Go pre-op mode"),
                                qsTr ("0x81 = Reset node"),
                                qsTr ("0x82 = Reset communication"),
                            ].join ("\n");
                        }
                    }
                }
                Group {
                    title: qsTr ("Intervals");

                    FormContainer {
                        colSpacing: Style.spacingNormal;
                        rowSpacing: Style.spacingNormal;
                        anchors {
                            fill: parent;
                            margins: Style.spacingBig;
                        }

                        TextLabel {
                            text: qsTr ("Change SYNC interval");
                        }
                        StretchRowContainer {
                            spacing: Style.spacingSmall;

                            TextBox {
                                id: inputSyncInterval;
                                text: "5000";
                                textAlign: TextInput.AlignHCenter;
                                implicitWidth: -1;
                                anchors.verticalCenter: parent.verticalCenter;

                                readonly property int value : {
                                    var ret = -1;
                                    var tmp = parseInt (text);
                                    if (!isNaN (tmp)) {
                                        ret = tmp;
                                    }
                                    return ret;
                                }
                            }
                            TextButton {
                                text: qsTr ("Save");
                                icon: SymbolLoader { symbol: Style.symbolCheck; }
                                anchors.verticalCenter: parent.verticalCenter;
                                onClicked: { Shared.changeSyncInterval (inputSyncInterval.value); }
                            }
                        }
                        TextLabel {
                            text: qsTr ("Change HeartBeat interval");
                        }
                        StretchRowContainer {
                            spacing: Style.spacingSmall;

                            TextBox {
                                id: inputHearbeatInterval;
                                text: "1000";
                                textAlign: TextInput.AlignHCenter;
                                implicitWidth: -1;
                                anchors.verticalCenter: parent.verticalCenter;

                                readonly property int value : {
                                    var ret = -1;
                                    var tmp = parseInt (text);
                                    if (!isNaN (tmp)) {
                                        ret = tmp;
                                    }
                                    return ret;
                                }
                            }
                            TextButton {
                                text: qsTr ("Save");
                                icon: SymbolLoader { symbol: Style.symbolCheck; }
                                anchors.verticalCenter: parent.verticalCenter;
                                onClicked: { Shared.changeHeartBeatInterval (inputHearbeatInterval.value); }
                            }
                        }
                    }
                }
            }
        }
    }
}
