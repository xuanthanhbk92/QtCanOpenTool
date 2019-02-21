import QtQuick 2.0
import QtCAN.Utils 2.0;
import QtQmlTricks.UiElements 2.0;

StretchColumnContainer {
    spacing: Style.spacingBig;
    visible: (driverWrapper.driverName !== "" && !driverWrapper.driverLoaded);

    property CanDriverWrapper driverWrapper : null;

    signal driverInit ();

    signal driverLoaded ();
    signal driverError  ();

    function cancel () {
        driverInit ();
        driverWrapper.selectDriver ("");
    }

    function accept () {
        var tmp = {};
        for (var idx = 0; idx < driverWrapper.driverOptions.length; ++idx) {
            var driverOpt = driverWrapper.driverOptions [idx];
            var item = repeaterOptions.itemAt ((idx * 2) +1);
            if (driverOpt && item) {
                tmp [driverOpt.key] = item.value;
            }
        }
        driverInit ();
        driverWrapper.loadDriver (tmp);
        if (driverWrapper.driverLoaded) {
            driverLoaded ();
        }
        else {
            driverError ();
        }
    }

    TextLabel {
        text: qsTr ("Set CAN driver options");
        horizontalAlignment: Text.AlignHCenter;
        font {
            weight: Font.Light;
            pixelSize: Style.fontSizeTitle;
        }
    }
    Line { }
    Stretcher {
        implicitWidth: Style.realPixels (450);
        implicitHeight: layoutOptions.implicitHeight;

        Component {
            id: compoLabel;

            TextLabel {
                text: (driverOpt ? driverOpt.label + " :" : "");
                font {
                    weight: Font.Light;
                    pixelSize: Style.fontSizeNormal;
                }

                property CanDriverOption driverOpt : null;
            }
        }
        Component {
            id: compoTextBox;

            TextBox {
                id: fieldTextBox;
                text: driverOpt.fallback;
                textAlign: TextInput.AlignHCenter;
                textFont {
                    weight: Font.Light;
                    pixelSize: Style.fontSizeNormal;
                }
                implicitWidth: -1;
                onAccepted: { accept (); }

                property alias value : fieldTextBox.text;

                property CanDriverOption driverOpt : null;
            }
        }
        Component {
            id: compoCheckBox;

            StretchRowContainer {
                Stretcher { }
                CheckableBox {
                    id: fieldCheckBox;
                    size: (Style.fontSizeNormal * 1.5);
                    value: parent.driverOpt.fallback;
                    implicitWidth: implicitHeight;
                }

                property alias value : fieldCheckBox.value;

                property CanDriverOption driverOpt : null;
            }
        }
        Component {
            id: compoComboBox;

            ComboList {
                id: fieldComboBox;
                model: (driverOpt ? driverOpt.list : []);
                delegate: ComboListDelegateForSimpleVar { }
                currentIdx: -1;
                onModelChanged: { sync (); }
                onDriverOptChanged: { sync (); }

                property alias value : fieldComboBox.currentKey;

                property CanDriverOption driverOpt : null;

                function sync () {
                    currentIdx = (driverOpt ? driverOpt.list.indexOf (driverOpt.fallback) : -1);
                }
            }
        }
        FormContainer {
            id: layoutOptions;
            rowSpacing: Style.spacingNormal;
            colSpacing: Style.spacingNormal;
            anchors.fill: parent;

            Repeater {
                id: repeaterOptions;
                model: {
                    var ret = [];
                    for (var idx = 0; idx < driverWrapper.driverOptions.length; ++idx) {
                        var driverOpt = driverWrapper.driverOptions [idx];
                        ret.push (compoLabel);
                        switch (driverOpt.type) {
                        case CanDriverOption.ListChoice:
                            ret.push (compoComboBox);
                            break;
                        case CanDriverOption.BooleanFlag:
                            ret.push (compoCheckBox);
                            break;
                        case CanDriverOption.NameString:
                        case CanDriverOption.SocketPort:
                        default:
                            ret.push (compoTextBox);
                            break;
                        }
                    }
                    return ret;
                }
                delegate: Loader {
                    sourceComponent: modelData;
                    onItemChanged: {
                        if (item) {
                            item ["driverOpt"] = driverOpt;
                        }
                    }

                    readonly property CanDriverOption driverOpt : driverWrapper.driverOptions [Math.floor (model.index / 2)];

                    readonly property var value : (item && "value" in item ? item ["value"] : undefined);
                }
            }
        }
    }
    Line { }
    StretchRowContainer {
        spacing: 0;

        Stretcher {}
        GridContainer {
            cols: capacity;
            capacity: 2;
            colSpacing: Style.spacingNormal;
            anchors.horizontalCenter: parent.horizontalCenter;

            TextButton {
                text: qsTr ("Cancel");
                icon: SymbolLoader {
                    color: Style.colorForeground;
                    symbol: Style.symbolCross;
                }
                onClicked: { cancel (); }
            }
            TextButton {
                text: qsTr ("OK");
                icon: SymbolLoader {
                    color: Style.colorForeground;
                    symbol: Style.symbolCheck;
                }
                onClicked: { accept (); }
            }
        }
        Stretcher {}
    }
}
