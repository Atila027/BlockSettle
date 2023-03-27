/*

***********************************************************************************
* Copyright (C) 2022, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2
import QtQuick.Controls 2.9
import QtQuick.Layouts 1.3
import QtQuick.Window 2
import Qt.labs.platform 1.1

import "StyledControls" 1
import "BsStyles" 1
import "Receive"  1
import "Send"  1
import "CreateWallet"  1
import "Pin"  1
import "Settings"  1
import "Plugins/Common" 1

ApplicationWindow {
    id: mainWindow

    minimumWidth: 1200
    minimumHeight: 800
    x: Screen.width / 2 - width / 2
    y: Screen.height / 2 - height / 2

    visible: false
    title: qsTr("BlockSettle Terminal")

    property var currentDialog: ({})
    property bool isNoWalletsWizard: false
    readonly property int resizeAnimationDuration: 25

    FontLoader {
        source: "qrc:/fonts/Roboto-Regular.ttf"
    }

    Component.onCompleted: {
        hide()
//        qmlFactory.installEventFilterToObj(mainWindow)
//        qmlFactory.applyWindowFix(mainWindow)
    }

    CreateWallet {
        id: create_wallet
        visible: false
    }

    ReceivePopup {
        id: receive_popup
        visible: false
        onClosing: {
            btnReceive.select(false)
        }
    }

    SendPopup {
        id: send_popup
        visible: false
        onClosing: {
            btnSend.select(false)
        }
    }

    SettingsPopup {
        id: settings_popup
        visible: false
        onClosing: {
            btnSettings.select(false)
        }
    }

    PinEntriesPopup {
        id: pin_popup
        visible: false
    }

    PasswordEntryPopup {
        id: password_popup
        visible: false
    }

    CustomMessageDialog {
        id: error_dialog
        visible: false
    }

    CustomFailDialog {
        id: fail_dialog
        visible: false;
    }

    Connections
    {
        target:bsApp
        function onInvokePINentry ()
        {
            show_popup(pin_popup)
        }

        function onInvokePasswordEntry(devName, acceptOnDevice)
        {
            password_popup.device_name = devName
            password_popup.accept_on_device = acceptOnDevice
            show_popup(password_popup)
        }

        function onShowError(text)
        {
            error_dialog.error = text
            show_popup(error_dialog)
        }

        function onShowFail(header, fail)
        {
            fail_dialog.header = header
            fail_dialog.fail = fail
            show_popup(fail_dialog)
        }

        function onWalletsLoaded (nb)
        {
            if (nb === 0)
            {
                isNoWalletsWizard = true
            }
        }

        function onShowNotification(title, text) {
            trayIcon.showMessage(title, text, SystemTrayIcon.Information, 5000)
        }
    }

    Timer {
        id: timerNoWalletsWizard

        interval: 1000
        running: false
        repeat: false
        onTriggered: {
            bsApp.getArmoryServers()
            show_popup(create_wallet)
        }
    }

    onVisibleChanged: {
        if (mainWindow.visible && isNoWalletsWizard)
        {
            isNoWalletsWizard = false
            timerNoWalletsWizard.running = true
        }
    }

    color: BSStyle.backgroundColor

    overlay.modal: Rectangle {
        color: BSStyle.backgroundModalColor
    }
    overlay.modeless: Rectangle {
        color: BSStyle.backgroundModeLessColor
    }

    InfoBanner {
        id: ibSuccess
        bgColor: "darkgreen"
    }
    InfoBanner {
        id: ibFailure
        bgColor: "darkred"
    }
    InfoBanner {
        id: ibInfo
        bgColor: "darkgrey"
    }

    StackView {
        id: stack
        initialItem: swipeView
        anchors.fill: parent
    }

    header: Column {
        height: 57
        width: parent.width
        spacing: 0

        RowLayout {
            height: 56
            width: parent.width
            spacing: 0

            Image {
                width: 129
                height: 24
                source: "qrc:/images/logo.png"
                Layout.leftMargin : 18
            }

            Label {
                width: 13
            }

            Label {
                Layout.fillWidth: true
            }

            Label {
                width: 35
                text: qsTr("Connection: ")
                font.pixelSize: 12
                font.family: "Roboto"
                font.weight: Font.Normal
                color: BSStyle.titleTextColor
            }

            Image {
                id: imgEnvKind
                width: 16
                source: (bsApp.armoryState !== 7) ? "qrc:/images/bitcoin-disabled.png" :
                    ((bsApp.networkType === 0) ? "qrc:/images/bitcoin-main-net.png" : "qrc:/images/bitcoin-test-net.png")
            }

            Label {
                width: 12
            }

            CustomTitleToolButton {
                id: btnSend

                enabled: (walletBalances.rowCount > 0)

                text: qsTr("Send")
                icon.source: "qrc:/images/send_icon.png"
                font.pixelSize: 12
                font.letterSpacing: 0.3
                Layout.fillHeight: true

                onClicked: {
                    topMenuBtnClicked(btnSend)
                    show_popup(send_popup)
                }
            }

            CustomTitleToolButton {
                id: btnReceive

                enabled: (walletBalances.rowCount > 0)

                text: qsTr("Receive")
                icon.source: "qrc:/images/receive_icon.png"

                font.pixelSize: 12
                font.letterSpacing: 0.3
                Layout.fillHeight: true

                onClicked: {
                    topMenuBtnClicked(btnReceive)
                    bsApp.generateNewAddress(walletBalances.selectedWallet, true)
                    show_popup(receive_popup)
                }
            }

            CustomTitleToolButton {
                id: btnSettings
                text: qsTr("Settings")
                font.pixelSize: 12
                font.letterSpacing: 0.3
                icon.source: "qrc:/images/settings_icon.png"
                onClicked: {
                    topMenuBtnClicked(btnSettings)
                    show_popup(settings_popup)
                }
            }
        }

        Rectangle {
            width: parent.width
            height: 1
            color: BSStyle.defaultGreyColor
        }
    }

    SwipeView {
        anchors.fill: parent
        id: swipeView
        currentIndex: tabBar.currentIndex

        OverviewPage {
            id: overviewPage
            onNewWalletClicked: {
                show_popup(create_wallet)
            }

            onOpenSend: (txId, isRBF, isCPFP) => {
                send_popup.open(txId, isRBF, isCPFP)
                show_popup(send_popup, true)
            }

            onOpenExplorer: (txId) => {
                tabBar.currentIndex = 2
                explorerPage.openTransaction(txId)
            }
        }

        TransactionsPage {
            id: transactionsPage

            onOpenSend: (txId, isRBF, isCPFP) => {
                send_popup.open(txId, isRBF, isCPFP)
                show_popup(send_popup, true)
            }
            onOpenExplorer: (txId) => {
                tabBar.currentIndex = 2
                explorerPage.openTransaction(txId)
            }
        }

        ExplorerPage {
            id: explorerPage
        }

        PluginsPage {
            id: pluginPage
        }
    }

    footer: Rectangle {
        height: 56
        width: parent.width
        color :"#191E2A"

        RowLayout {
            spacing: 0
            anchors.fill: parent

            Label {
                Layout.fillWidth: true
            }

            TabBar {
                id: tabBar
                currentIndex: swipeView.currentIndex
                padding: 0
                spacing: 0
                Layout.fillWidth: false

                background: Rectangle {
                    color: "transparent"
                }

                CustomTabButton {
                    id: btnOverview
                    text: qsTr("Overview")
                    Component.onCompleted: {
                        btnOverview.setIcons ("qrc:/images/overview_icon.png", "qrc:/images/overview_icon_not_choosed.png")
                    }
                }
                CustomTabButton {
                    id: btnTransactions
                    text: qsTr("Transactions")
                    Component.onCompleted: {
                        btnTransactions.setIcons ("qrc:/images/transactions_icon.png", "qrc:/images/transactions_icon_unchoosed.png")
                    }
                }

                CustomTabButton {
                    id: btnExplorer
                    text: qsTr("Explorer")
                    Component.onCompleted: {
                        btnExplorer.setIcons ("qrc:/images/explorer_icon.png", "qrc:/images/explorer_icon_unchoosed.png")
                    }
                }

                CustomTabButton {
                    id: btnPlugins
                    text: qsTr("Apps")
                    Component.onCompleted: {
                        btnPlugins.setIcons ("qrc:/images/plugins_icon.png", "qrc:/images/plugins_icon_unchoosed.png")
                    }
                }
            }

            Label {
                Layout.fillWidth: true
            }
        }
    }

    function topMenuBtnClicked(clickedBtn)
    {
        btnSend.select(false)
        btnReceive.select(false)
        btnSettings.select(false)

        clickedBtn.select(true)
    }

    function moveMainWindowToScreenCenter() {
        mainWindow.x = Screen.virtualX + (Screen.width - mainWindow.width) / 2
        mainWindow.y = Screen.virtualY + (Screen.height - mainWindow.height) / 2
    }

    function resizeAnimated(w,h) {
        mwWidthAnimation.from = mainWindow.width
        mwWidthAnimation.to = w
        mwWidthAnimation.restart()

        mwHeightAnimation.from = mainWindow.height
        mwHeightAnimation.to = h
        mwHeightAnimation.restart()

        mwXAnimation.from = mainWindow.x
        mwXAnimation.to = Screen.virtualX + (Screen.width - w) / 2
        mwXAnimation.restart()

        mwYAnimation.from = mainWindow.y
        mwYAnimation.to = Screen.virtualY + (Screen.height - h) / 2
        mwYAnimation.restart()
    }

    NumberAnimation {
        id: mwWidthAnimation
        target: mainWindow
        property: "width"
        duration: resizeAnimationDuration
    }
    NumberAnimation {
        id: mwHeightAnimation
        target: mainWindow
        property: "height"
        duration: resizeAnimationDuration
    }

    NumberAnimation {
        id: mwXAnimation
        target: mainWindow
        property: "x"
        duration: resizeAnimationDuration
    }
    NumberAnimation {
        id: mwYAnimation
        target: mainWindow
        property: "y"
        duration: resizeAnimationDuration
    }


    //global functions
    function getWalletData (index: int, role: string)
    {
        return walletBalances.data(walletBalances.index(index, 0), role)
    }

    function getFeeSuggData (index: int, role: string)
    {
        return feeSuggestions.data(feeSuggestions.index(index, 0), role)
    }

    function show_popup (id, noInit = false)
    {
        if (typeof id.init === "function" && !noInit)
        {
            id.init()
        }
        id.show()
        id.raise()
        id.requestActivate()
    }

    SystemTrayIcon {
        id: trayIcon
        visible: true
        icon.source: "qrc:/images/terminal.ico"

        onActivated: {
            mainWindow.show()
            mainWindow.raise()
            mainWindow.requestActivate()
        }

        Component.onCompleted: {
            trayIcon.show()
        }
    }
}
