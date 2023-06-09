/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.9
import QtQuick.Layouts 1.0
import QtQuick.Controls 2.4

import com.blocksettle.TXInfo 1.0
import com.blocksettle.PasswordDialogData 1.0
import com.blocksettle.AutheIDClient 1.0
import com.blocksettle.AuthSignWalletObject 1.0
import com.blocksettle.WalletInfo 1.0
import com.blocksettle.QSeed 1.0
import com.blocksettle.QPasswordData 1.0

import "../StyledControls"
import "../BsControls"
import "../BsStyles"
import "../js/helper.js" as JsHelper

TxSignSettlementBaseDialog {
    id: root
    readonly property string inputProduct: " XBT"
    readonly property string fxProduct: passwordDialogData.FxProduct
    readonly property bool is_payOut: passwordDialogData.PayOutType
    readonly property bool is_payIn: !is_payOut

    signingAllowed: passwordDialogData.RequesterAuthAddressVerified && passwordDialogData.ResponderAuthAddressVerified
    errorMessage: qsTr("Authentication Address could not be verified")
    validationTitle: qsTr("Counterparty")

    function getInputValue() {
        if (is_payOut) {
            return txInfo.inputAmountFull
        } else {
            return txInfo.inputAmount
        }
    }

    function getTotalValue() {
        if (is_payOut) {
            return txInfo.outputAmountFull
        } else {
            return txInfo.total
        }
    }

    function getChangeValue() {
        if (is_payOut) {
            return 0
        } else {
            return txInfo.changeAmount
        }
    }

    quantity: passwordDialogData.hasQuantity() ? passwordDialogData.Quantity : ""
    totalValue:  passwordDialogData.hasTotalValue() ? passwordDialogData.TotalValue : ""
    priceString: price + " " + fxProduct + " / 1 XBT"

    settlementDetailsItem: GridLayout {
        id: gridSettlementDetails
        columns: 2
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        rowSpacing: 0

        CustomHeader {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            text: qsTr("Settlement Details")
            Layout.preferredHeight: 25
        }

        // SettlementAddress
        CustomLabel {
            visible: passwordDialogData.hasSettlementAddress()
            Layout.fillWidth: true
            text: qsTr("Settlement Address")
        }
        CustomLabelCopyableValue {
            id: settlementAddress
            visible: passwordDialogData.hasSettlementAddress()
            text: passwordDialogData.SettlementAddress
                .truncString(passwordDialogData.hasRequesterAuthAddress() ? passwordDialogData.RequesterAuthAddress.length : 30)
            Layout.alignment: Qt.AlignRight
            textForCopy: passwordDialogData.SettlementAddress

            ToolTip.text: passwordDialogData.SettlementAddress
            ToolTip.delay: 150
            ToolTip.timeout: 5000
            ToolTip.visible: settlementAddress.mouseArea.containsMouse
        }

        // SettlementId
        CustomLabel {
            visible: passwordDialogData.hasSettlementId()
            Layout.fillWidth: true
            text: qsTr("Settlement Id")
        }
        CustomLabelCopyableValue {
            id: settlementId
            visible: passwordDialogData.hasSettlementId()
            text: passwordDialogData.SettlementId
                .truncString(passwordDialogData.hasRequesterAuthAddress() ? passwordDialogData.RequesterAuthAddress.length : 30)
            Layout.alignment: Qt.AlignRight
            textForCopy: passwordDialogData.SettlementId

            ToolTip.text: passwordDialogData.SettlementId
            ToolTip.delay: 150
            ToolTip.timeout: 5000
            ToolTip.visible: settlementId.mouseArea.containsMouse
        }

        // Requester Authentication Address
        CustomLabel {
            visible: passwordDialogData.hasRequesterAuthAddress() && passwordDialogData.IsDealer
            Layout.fillWidth: true
            text: qsTr("Counterparty")
        }
        CustomLabelCopyableValue {
            visible: passwordDialogData.hasRequesterAuthAddress() && passwordDialogData.IsDealer
            text: passwordDialogData.RequesterAuthAddress
            Layout.alignment: Qt.AlignRight
            color: getValidationColor(passwordDialogData.RequesterAuthAddressVerified)
        }

        // Responder Authentication Address = dealer
        CustomLabel {
            visible: passwordDialogData.hasResponderAuthAddress() && !passwordDialogData.IsDealer
            Layout.fillWidth: true
            text: qsTr("Counterparty")
        }
        CustomLabelCopyableValue {
            visible: passwordDialogData.hasResponderAuthAddress() && !passwordDialogData.IsDealer
            text: passwordDialogData.ResponderAuthAddress
            Layout.alignment: Qt.AlignRight
            color: getValidationColor(passwordDialogData.ResponderAuthAddressVerified)
        }
    }

    txDetailsItem: GridLayout {
        id: gridTxDetails
        columns: 2
        Layout.leftMargin: 10
        Layout.rightMargin: 10
        rowSpacing: 0

        CustomHeader {
            Layout.fillWidth: true
            Layout.columnSpan: 2
            text: qsTr("Transaction Details")
            Layout.preferredHeight: 25
        }

        // Input Amount
        CustomLabel {
            Layout.fillWidth: true
            text: qsTr("Input Amount")
        }
        CustomLabelValue {
            text: minus_string + getInputValue().toFixed(8) + inputProduct

            Layout.alignment: Qt.AlignRight
        }

        // Output Amount
        CustomLabel {
            Layout.fillWidth: true
            text: qsTr("Output Amount ")
        }
        CustomLabelValue {
            text: plus_string + getTotalValue().toFixed(8) + inputProduct
            Layout.alignment: Qt.AlignRight
        }

        // Return Amount
        CustomLabel {
            Layout.fillWidth: true
            text: qsTr("Return Amount")
        }
        CustomLabelValue {
            text: plus_string + getChangeValue().toFixed(8) + inputProduct
            Layout.alignment: Qt.AlignRight
        }

        // Network Fee
        CustomLabel {
            Layout.fillWidth: true
            text: qsTr("Network Fee")
        }
        CustomLabelValue {
            text: minus_string + txInfo.fee.toFixed(8) + inputProduct
            Layout.alignment: Qt.AlignRight
        }

        // Settlement Pay-In
        CustomLabel {
            visible: is_payIn
            Layout.fillWidth: true
            text: qsTr("Settlement Pay-In")
        }
        CustomLabelValue {
            visible: is_payIn
            text: minus_string + txInfo.amount.toFixed(8) + inputProduct
            Layout.alignment: Qt.AlignRight
        }

        // Settlement Pay-Out
        CustomLabel {
            visible: is_payOut
            Layout.fillWidth: true
            text: qsTr("Settlement Pay-Out")
        }
        CustomLabelValue {
            visible: is_payOut
            text: plus_string + getTotalValue().toFixed(8) + inputProduct
            Layout.alignment: Qt.AlignRight
        }
     }
}
