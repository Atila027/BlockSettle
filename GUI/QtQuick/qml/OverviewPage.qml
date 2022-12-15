/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Layouts 1.12
import QtQml.Models 2

import "StyledControls"
import "BsStyles"
//import "BsControls"
//import "BsDialogs"
//import "js/helper.js" as JsHelper

Item {
    id: overview

    CreateNewWallet {
        id: createNewWalletPage
        visible: false
    }

    Column {
        spacing: 23
        anchors.fill: parent

        Row {
            spacing: 15

            ComboBox {
                id: walletsComboBox
                objectName: "walletsComboBox"
                model: bsApp.walletsList
                font.pointSize: 12
            }
            Column {
                Label {
                    text: qsTr("<font color=\"gray\">Confirmed balance</font>")
                    font.pointSize: 8
                }
                Label {
                    text: qsTr("<font color=\"white\">%1 BTC</font>").arg(bsApp.confirmedBalance)
                    font.pointSize: 12
                }
            }
            Column {
                Label {
                    text: qsTr("<font color=\"gray\">Unconfirmed balance</font>")
                    font.pointSize: 8
                }
                Label {
                    text: qsTr("<font color=\"white\">%1 BTC</font>").arg(bsApp.unconfirmedBalance)
                    font.pointSize: 12
                }
            }
            Column {
                Label {
                    text: qsTr("<font color=\"gray\">Total balance</font>")
                    font.pointSize: 8
                }
                Label {
                    text: qsTr("<font color=\"white\">%1 BTC</font>").arg(bsApp.totalBalance)
                    font.pointSize: 12
                }
            }
            Column {
                Label {
                    text: qsTr("<font color=\"gray\">#Used addresses</font>")
                    font.pointSize: 8
                }
                Label {
                    text: qsTr("<font color=\"white\">%1</font>").arg(bsApp.nbUsedWalletAddresses)
                    font.pointSize: 12
                }
            }
            Item {  // spacer item
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle { anchors.fill: parent; color: "#ffaaaa" }
            }
            Button {
                text: qsTr("Wallet Properties")
                font.pointSize: 10
            }
            Button {
                text: qsTr("Create new wallet")
                font.pointSize: 10
                onClicked: {
                    stack.push(createNewWalletPage)
                }
            }
        }

        Row {
            spacing: 15

            Label {
                text: qsTr("<font color=\"white\">Addresses</font>")
                font.pointSize: 14
            }
            Item {  // spacer item
                Layout.fillWidth: true
                Layout.fillHeight: true
                Rectangle { anchors.fill: parent; color: "#ffaaaa" }
            }
            Button {
                text: qsTr("Hide used")
                font.pointSize: 8
            }
            Button {
                text: qsTr("Hide internal")
                font.pointSize: 8
            }
            Button {
                text: qsTr("Hide external")
                font.pointSize: 8
            }
            Button {
                text: qsTr("Hide empty")
                font.pointSize: 8
            }
        }
        TableView {
            width: 1000
            height: 300
            columnSpacing: 1
            rowSpacing: 1
            clip: false
            ScrollIndicator.horizontal: ScrollIndicator { }
            ScrollIndicator.vertical: ScrollIndicator { }
            model: addressListModel
            delegate: Rectangle {
                implicitWidth: firstcol ? 550 : 150
                implicitHeight: 20
                border.color: "black"
                border.width: 1
                color: heading ? 'black' : 'darkslategrey'
                Text {
                    text: tabledata
                    font.pointSize: heading ? 8 : 10
                    color: heading ? 'darkgrey' : 'lightgrey'
                    anchors.centerIn: parent
                }
            }
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    //if ((addressListModel.column === 0) && (addressListModel.row > 0)) {
                        bsApp.copyAddressToClipboard("row: " + model.row)
                        //ibInfo.displayMessage(qsTr("address copied"))
                    //}
                }
                onDoubleClicked: {
                    //TODO: show address details
                }
            }
        }

        Label {
            text: qsTr("<font color=\"white\">Non-Settled Transactions</font>")
            font.pointSize: 14
        }
        TableView {
            width: 1000
            height: 200
            columnSpacing: 1
            rowSpacing: 1
            clip: true
            ScrollIndicator.horizontal: ScrollIndicator { }
            ScrollIndicator.vertical: ScrollIndicator { }
            model: pendingTxListModel
            delegate: Rectangle {
                implicitWidth: 125 * colWidth
                implicitHeight: 20
                border.color: "black"
                border.width: 1
                color: heading ? 'black' : 'darkslategrey'
                Text {
                    text: tableData
                    font.pointSize: heading ? 8 : 10
                    color: dataColor
                    anchors.centerIn: parent
                }
            }
        }
    }
}
