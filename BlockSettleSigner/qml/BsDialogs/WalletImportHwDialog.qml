/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.platform 1.1

import com.blocksettle.WalletInfo 1.0
import com.blocksettle.HwDeviceManager 1.0

import "../BsControls"
import "../StyledControls"
import "../BsStyles"
import "../BsHw"
import "../js/helper.js" as JsHelper


CustomTitleDialogWindow {
    id: root

    property WalletInfo walletInfo: WalletInfo{}

    property bool acceptable: !hwDeviceList.isScanning && !hwDeviceList.isImporting &&
                              (hwDeviceList.readyForImport || hwDeviceList.isNoDevice) && !scanUpdateDelay.running

    property int inputLabelsWidth: 110

    title: qsTr("Import Wallet")
    width: 480
    height: 320
    abortBoxType: BSAbortBox.AbortType.WalletImport

    onAboutToShow: hwDeviceList.init()
    onAboutToHide: hwDeviceList.release();

    onEnterPressed: {
        if (btnAccept.enabled) btnAccept.onClicked()
    }

    Timer {
        id: scanUpdateDelay
        interval: 2000
        repeat: false
    }

    runSpinner: hwDeviceList.isImporting || hwDeviceList.isScanning || scanUpdateDelay.running

    cContentItem: Item {
        width: parent.width

        ColumnLayout {
            id: mainLayout
            spacing: 5
            anchors.fill: parent

            CustomHeader {
                id: headerText
                text: qsTr("Hardware Device")
                Layout.fillWidth: true
                Layout.preferredHeight: 25
                Layout.topMargin: 5
                Layout.leftMargin: 10
                Layout.rightMargin: 10
            }

            StackLayout {
                currentIndex: hwDeviceList.isNoDevice ? 1 : 0
                Layout.fillWidth: true

                ColumnLayout {
                    id: fullImportTab

                    RowLayout {
                        Layout.topMargin: 0
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10
                        Layout.fillWidth: true

                        ColumnLayout {
                            id: selectLayout
                            Layout.fillWidth: true

                            // HARDWARE DEVICES
                            HwAvailableDevices {
                                id: hwDeviceList

                                Layout.fillWidth: true
                                Layout.fillHeight: true

                                onPubKeyReady: {
                                    importWoWallet();
                                }

                                onFailed: {
                                    JsHelper.messageBox(BSMessageBox.Type.Critical
                                        , qsTr("Import Failed"), qsTr("Import HW-wallet failed:\n") + reason)
                                }
                            }
                        }
                    }
                }

                ColumnLayout {
                    id: noDevicesAvailable

                    RowLayout {
                        Layout.leftMargin: 10
                        Layout.rightMargin: 10

                        CustomLabel {
                            Layout.fillWidth: true
                            text: qsTr(
                             "No hardware device detected.\n\n" +
                             "If your device cannot be detected, please consider the following steps before consulting your hardware wallet manufacturer:\n\n" +
                             (Qt.platform.os === "linux" ?
                                  "• If you are a Linux user, your device must be added to the udev rule-set to communicate with it. Please ensure your device can be detected by the system.\n"
                                  : "") +
                             "• If you are a Trezor user, ensure you have the Trezor Bridge installed (if not install and press \"Rescan\")\n" +
                             "• If you are a Ledger user, ensure your PIN has been entered and that your device displays \"Application is Ready\"")
                        }
                    }
                }

            }
        }
    }

    cFooterItem: RowLayout {
        CustomButtonBar {
            Layout.fillWidth: true

            CustomButton {
                anchors.left: parent.left
                anchors.bottom: parent.bottom
                text: qsTr("Cancel")
                onClicked: {
                    JsHelper.openAbortBox(root, abortBoxType)
                }
            }

            CustomButton {
                primary: true
                anchors.right: btnAccept.left
                anchors.bottom: parent.bottom
                text: qsTr("Rescan")

                onClicked: {
                    scanUpdateDelay.start();
                    hwDeviceList.rescan();
                }
            }

            CustomButton {
                id: btnAccept
                primary: true
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                text: qsTr("Import")
                enabled: acceptable && hwDeviceList.readyForImport

                onClicked:  hwDeviceList.importXpub();
            }
        }
    }

    function applyDialogClosing() {
        JsHelper.openAbortBox(root, abortBoxType);
        return false;
    }

    function importWoWallet() {
        var importCallback = function(success, id, name, desc) {
            if (success) {
                let walletInfo = qmlFactory.createWalletInfo()
                walletInfo.walletId = id
                walletInfo.name = name
                walletInfo.desc = desc

                let type = BSResultBox.ResultType.HwWallet

                var mb = JsHelper.resultBox(type, true, walletInfo)
                mb.bsAccepted.connect(acceptAnimated)
            }
            else {
                hwDeviceList.isImporting = false
                JsHelper.messageBox(BSMessageBox.Type.Critical
                    , qsTr("Import Failed"), qsTr("Import HW-wallet failed:\n") + desc)
            }
        }

        walletsProxy.importHwWallet(hwDeviceList.hwWalletInfo, importCallback)
    }
}
