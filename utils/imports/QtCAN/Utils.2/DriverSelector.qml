import QtQuick 2.0
import QtCAN.Utils 2.0;
import QtQmlTricks.UiElements 2.0;

StretchColumnContainer {
    spacing: Style.spacingBig;
    visible: (!driverWrapper.driverLoaded && driverWrapper.driverName === "");

    property CanDriverWrapper driverWrapper : null;

    TextLabel {
        text: qsTr ("Choose CAN driver");
        horizontalAlignment: Text.AlignHCenter;
        font {
            weight: Font.Light;
            pixelSize: Style.fontSizeTitle;
        }
    }
    Line { }
    GridContainer {
        cols: 1;
        rowSpacing: Style.spacingSmall;
        anchors.horizontalCenter: parent.horizontalCenter;

        Repeater {
            id: repeaterDrivers;
            model: driverWrapper.driversList;
            delegate: TextButton {
                text: modelData;
                enabled: (driverWrapper.driverName !== text);
                onClicked: { driverWrapper.selectDriver (text); }
            }
        }
    }
}
