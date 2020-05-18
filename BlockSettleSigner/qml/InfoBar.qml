import QtQuick 2.12
import QtQuick.Layouts 1.3

import "BsStyles"

Item {
    id: infoBarRoot
    height: 30

    property bool showChangeApplyMessage: false

    RowLayout {
        anchors.fill: parent
        spacing: 10

        Item {
            Layout.fillWidth: true
            Layout.fillHeight: true

            Text {
                visible: infoBarRoot.showChangeApplyMessage
                anchors {
                    fill: parent
                    leftMargin: 10
                }
                horizontalAlignment: Text.AlignLeft
                verticalAlignment: Text.AlignVCenter

                text: qsTr("Changes will take effect after the application is restarted.")
                color: BSStyle.inputsPendingColor
            }
        }

        Rectangle {
            visible: showTestNet
            radius: 5
            color: BSStyle.testNetColor
            width: 100
            height: 20
            Layout.alignment: Qt.AlignVCenter

            Text {
                text: qsTr("Test environment")
                anchors.fill: parent
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
            }

            Component.onCompleted: {
                visible = signerSettings.testNet
            }
        }
    }

    Rectangle {
        height: 1
        width: parent.width
        color: Qt.rgba(1, 1, 1, 0.1)
        anchors.bottom: parent.bottom
    }
}
