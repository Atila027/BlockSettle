/*

***********************************************************************************
* Copyright (C) 2020 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.9
import QtQuick.Layouts 1.3
import QtQuick.Controls 2.2
import Qt.labs.platform 1.1

import com.blocksettle.HwDeviceManager 1.0

import "../BsControls"
import "../StyledControls"
import "../js/helper.js" as JsHelper
import "../BsStyles"

Item {
    id: root

    property var hwWalletInfo
    property bool readyForImport: hwList.deviceIndex !== -1
    property bool isNoDevice: hwDeviceManager.devices.rowCount() === 0
    property bool isScanning: hwDeviceManager.isScanning
    property bool isImporting: false

    signal pubKeyReady();
    signal failed(string reason);

    Connections {
        target: hwDeviceManager
        onPublicKeyReady: {
            hwWalletInfo = walletInfo;
            root.pubKeyReady()
        }
        onRequestPinMatrix: JsHelper.showHwPinMatrix(deviceIndex);
        onRequestHWPass: JsHelper.showHwPassphrase(deviceIndex, allowedOnDevice);
        onOperationFailed: {
            hwWalletInfo = {};
            isImporting = false;
            root.failed(reason);
        }
    }

    function release() {
        hwDeviceManager.hwOperationDone();
    }

    function init() {
        rescan();
    }

    function importXpub() {
        isImporting = true;
        hwDeviceManager.requestPublicKey(hwList.deviceIndex);
    }

    function rescan() {
        hwDeviceManager.scanDevices();
    }


    ListView {
        id: hwList
        visible: !isNoDevice
        anchors.fill: parent

        model: hwDeviceManager.devices

        property int deviceIndex: -1

        highlight: Rectangle {
            color: BSStyle.comboBoxItemBgHighlightedColor
            anchors.leftMargin: 5
            anchors.rightMargin: 5
        }

        delegate: Rectangle {
            id: delegateRoot

            width: parent.width
            height: 20

            color: index % 2 === 0 ? "transparent" : "#8027363b"
            property color textColor: hwList.currentIndex === index ? "white" :
                                        enabled ? BSStyle.labelsTextColor : BSStyle.disabledColor

            RowLayout {
                anchors.fill: parent
                Text {
                    Layout.fillWidth: true
                    height: parent.height

                    leftPadding: 10
                    text: model.label + "(" + model.vendor + ")"
                    enabled: model.pairedWallet.length === 0 && model.status.length === 0
                    color: textColor
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignLeft
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }

                Text {
                    Layout.fillWidth: true
                    height: parent.height

                    rightPadding: 10
                    text: if (model.status.length)
                              status;
                          else if (model.pairedWallet.length)
                              "Imported(" + model.pairedWallet + ")";
                          else
                              "New Device";
                    enabled: model.pairedWallet.length === 0 && model.status.length === 0
                    color: textColor
                    font.pixelSize: 12
                    horizontalAlignment: Text.AlignRight
                    verticalAlignment: Text.AlignVCenter
                    elide: Text.ElideRight
                }
            }

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    if (hwList.currentIndex === index) {
                        return;
                    }

                    hwList.currentIndex = index
                }
            }

            Connections {
                target: hwList
                onCurrentIndexChanged: checkReadyForImport()
            }

            Connections {
                target: hwDeviceManager.devices
                onModelReset: {
                    checkReadyForImport();
                }
            }

            function checkReadyForImport() {
                if (hwList.currentIndex === index) {
                    hwList.deviceIndex = (
                                (typeof model.pairedWallet !== 'undefined' && model.pairedWallet.length === 0)
                                && (typeof model.status !== 'undefined' && model.status.length === 0)
                                ) ? index : -1
                    root.readyForImport = (hwList.deviceIndex !== -1);
                }

            }
        }

        onCountChanged: {
            if (count !== 0 && hwList.currentIndex === -1)
                hwList.currentIndex = 0;
        }

        Connections {
            target: hwDeviceManager.devices
            onToppestImportChanged: {
                if (!root.readyForImport) {
                    hwList.currentIndex = hwDeviceManager.devices.toppestImport;
                }
            }
        }

    }


}
