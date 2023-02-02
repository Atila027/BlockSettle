import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.15

import "../BsStyles"
import "../StyledControls"

ColumnLayout  {

    id: layout

    signal sig_save()

    height: 548
    width: 580

    spacing: 0

    CustomTitleLabel {
        id: title

        Layout.alignment: Qt.AlignCenter
        Layout.preferredHeight : title.height

        text: qsTr("Network")
    }

    CustomTextInput {
        id: host

        Layout.alignment: Qt.AlignCenter
        Layout.preferredHeight : 70
        Layout.preferredWidth: 532
        Layout.topMargin: 24

        input_topMargin: 35
        title_leftMargin: 16
        title_topMargin: 16

        title_text: qsTr("Armory host")
    }

    CustomTextInput {
        id: port

        Layout.alignment: Qt.AlignCenter
        Layout.preferredHeight : 70
        Layout.preferredWidth: 532
        Layout.topMargin: 10

        input_topMargin: 35
        title_leftMargin: 16
        title_topMargin: 16

        title_text: qsTr("Armory port")
    }

    Label {
        id: spacer
        Layout.fillWidth: true
        Layout.fillHeight: true
    }

    CustomButton {
        id: save_but

        Layout.bottomMargin: 40
        Layout.alignment: Qt.AlignBottom | Qt.AlignHCenter

        enabled: host.input_text.length && port.input_text.length

        width: 532

        text: qsTr("Save")

        Component.onCompleted: {
            save_but.preferred = true
        }

        function click_enter() {
            if (!save_but.enabled)
                return

            bsApp.settingArmoryHost = host.input_text
            bsApp.settingArmoryPort = port.input_text

            layout.sig_save()
        }
    }

    Keys.onEnterPressed: {
        save_but.click_enter()
    }

    Keys.onReturnPressed: {
        save_but.click_enter()
    }

    function init()
    {
        port.input_text = bsApp.settingArmoryPort
        host.input_text = bsApp.settingArmoryHost

        host.setActiveFocus()
    }
}
