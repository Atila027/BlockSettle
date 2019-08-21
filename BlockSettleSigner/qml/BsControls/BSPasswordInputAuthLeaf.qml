import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import com.blocksettle.PasswordDialogData 1.0
import com.blocksettle.WalletInfo 1.0

import "../StyledControls"
import "../BsStyles"

BSPasswordInput {
    id: root

    property WalletInfo walletInfo: WalletInfo{}
    property PasswordDialogData passwordDialogData: PasswordDialogData {}

    title: passwordDialogData.value("Title")

    CustomHeader {
        text: qsTr("Wallet Details")
        Layout.fillWidth: true
        Layout.preferredHeight: 25
        Layout.topMargin: 5
        Layout.leftMargin: 10
        Layout.rightMargin: 10
    }

    RowLayout {
        spacing: 5
        Layout.topMargin: 5
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10

        CustomLabel {
            Layout.minimumWidth: 110
            Layout.preferredWidth: 110
            Layout.maximumWidth: 110
            Layout.fillWidth: true
            text: qsTr("Wallet name")
        }
        CustomLabel {
            Layout.fillWidth: true
            Layout.preferredWidth: 110
            text: walletInfo.name
        }
    }

    RowLayout {
        spacing: 5
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10

        CustomLabel {
            Layout.minimumWidth: 110
            Layout.preferredWidth: 110
            Layout.maximumWidth: 110
            Layout.fillWidth: true
            text: qsTr("Wallet ID")
        }
        CustomLabel {
            Layout.fillWidth: true
            text: walletInfo.walletId
        }
    }

    CustomHeader {
        text: qsTr("Auth Leaf Details")
        Layout.fillWidth: true
        Layout.preferredHeight: 25
        Layout.topMargin: 5
        Layout.leftMargin: 10
        Layout.rightMargin: 10
    }

    RowLayout {
        spacing: 5
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10

        CustomLabel {
            Layout.minimumWidth: 110
            Layout.preferredWidth: 110
            Layout.maximumWidth: 110
            Layout.fillWidth: true
            text: qsTr("Email")
        }
        CustomLabel {
            Layout.fillWidth: true
            text: passwordDialogData.value("Email")
        }
    }
}
