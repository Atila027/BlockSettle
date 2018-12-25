import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import QtQuick.Controls 1.4

import com.blocksettle.AuthSignWalletObject 1.0
import com.blocksettle.WalletInfo 1.0
import com.blocksettle.AutheIDClient 1.0
import com.blocksettle.QPasswordData 1.0
import com.blocksettle.NsWallet.namespace 1.0

import "../StyledControls"

ColumnLayout {
    id: root
    property WalletInfo walletInfo : WalletInfo {}
    property alias password: passwordInput.text
    property int inputsWidth

    CustomHeader {
        text:   qsTr("Wallet Details")
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
            text:   qsTr("Wallet name")
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
            text:   qsTr("Wallet ID")
        }
        CustomLabel {
            Layout.fillWidth: true
            text: walletInfo.walletId
        }
    }

    //                    CustomHeader {
    //                        text:   qsTr("Current Encryption")
    //                        Layout.fillWidth: true
    //                        Layout.preferredHeight: 25
    //                        Layout.topMargin: 5
    //                        Layout.leftMargin: 10
    //                        Layout.rightMargin: 10
    //                    }

    RowLayout {
        spacing: 5
        Layout.fillWidth: true
        Layout.leftMargin: 10
        Layout.rightMargin: 10

        CustomLabel {
            visible: walletInfo.encType === NsWallet.Password
            elide: Label.ElideRight
            text: qsTr("Current password")
            wrapMode: Text.WordWrap
            Layout.minimumWidth: 110
            Layout.preferredWidth: 110
            Layout.maximumWidth: 110
            Layout.fillWidth: true
        }
        CustomPasswordTextInput {
            id: passwordInput
            visible: walletInfo.encType === NsWallet.Password
            focus: true
            placeholderText: qsTr("Old Password")
            Layout.fillWidth: true
            Layout.preferredWidth: inputsWidth
        }

        CustomLabel {
            id: labelAuth
            Layout.preferredWidth: 110
            visible: walletInfo.encType === NsWallet.Auth
            text: qsTr("Encryption")
        }
        CustomLabel {
            id: labelAuthStatus
            visible: walletInfo.encType === NsWallet.Auth
            text: "Auth eID"
        }
    }

}

