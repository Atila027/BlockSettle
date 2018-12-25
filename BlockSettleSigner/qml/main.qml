import QtQuick 2.9
import QtQuick.Controls 2.2
import QtQuick.Layouts 1.3
import QtQuick.Window 2.1
import Qt.labs.settings 1.0

import com.blocksettle.TXInfo 1.0
import com.blocksettle.WalletInfo 1.0
import com.blocksettle.AutheIDClient 1.0
import com.blocksettle.AuthSignWalletObject 1.0
import com.blocksettle.QSeed 1.0
import com.blocksettle.QPasswordData 1.0
import com.blocksettle.NsWallet.namespace 1.0

import "StyledControls"
import "BsStyles"
import "BsControls"
import "BsDialogs"
import "js/helper.js" as JsHelper


ApplicationWindow {
    id: mainWindow

    visible: true
    title: qsTr("BlockSettle Signer")
    width: 800
    height: 600
    minimumWidth: 800
    minimumHeight: 600

    background: Rectangle {
        color: BSStyle.backgroundColor
    }
    overlay.modal: Rectangle {
        color: BSStyle.backgroundModalColor
    }
    overlay.modeless: Rectangle {
        color: BSStyle.backgroundModeLessColor
    }

    Settings {
        id:         settings
        category:   "GUI"
        property alias x:       mainWindow.x
        property alias y:       mainWindow.y
        property alias width:   mainWindow.width
        property alias height:  mainWindow.height
        property alias tabIdx:  swipeView.currentIndex
    }
//    WalletExportWoDialog {

//    }

    InfoBanner {
        id: ibSuccess
        bgColor:    "darkgreen"
    }
    InfoBanner {
        id: ibFailure
        bgColor:    "darkred"
    }

    DirSelectionDialog {
        id:     ldrWoWalletDirDlg
        title:  qsTr("Select watching only wallet target directory")
    }
    DirSelectionDialog {
        id:     ldrDirDlg
        title:  qsTr("Select directory")
    }

    SwipeView {
        id: swipeView
        anchors.fill: parent
        currentIndex: tabBar.currentIndex

        StatusPage {
            id: dashboardPage
        }

        SettingsPage {
            id: settingsPage
        }

        AutoSignPage {
            id: autoSignPage
        }

        WalletsPage {
            id: walletsPage
        }
    }

    footer: TabBar {
        id: tabBar
        currentIndex: swipeView.currentIndex
        height: 50
        spacing: 2;
        background: Rectangle {
                color: "transparent"
            }

        CustomTabButton {
            id:     btnStatus
            text:   qsTr("Dashboard")

        }

        CustomTabButton {
            id:     btnSettings
            text:   qsTr("Settings")

        }

        CustomTabButton {
            id:     btnAutoSign
            text:   qsTr("Auto-Sign")

        }

        CustomTabButton {
            id:     btnWallets
            text:   qsTr("Wallets")

        }
    }

    onClosing: {
        settingsPage.storeSettings();
        autoSignPage.storeSettings();
    }

    signal passwordEntered(string walletId, string password, bool cancelledByUser)

    function createPasswordDialog(prompt, txInfo) {
        console.log("QML createPasswordDialog")
        var dlg = Qt.createQmlObject("PasswordEntryDialog {}", mainWindow, "passwordDlg")
        dlg.prompt = prompt
        dlg.txInfo = txInfo
        dlg.accepted.connect(function() {
            if (txInfo.walletInfo.encType === NsWallet.Password) {
                passwordEntered(txInfo.walletInfo.walletId, dlg.password, false)
            }
            else {
                passwordEntered(txInfo.walletInfo.walletId, dlg.seed.password, false)
            }

        })
        dlg.rejected.connect(function() {
            passwordEntered(txInfo.walletInfo.walletId, '', dlg.cancelledByUser)
        })
        mainWindow.requestActivate()
        dlg.open()
    }

}
