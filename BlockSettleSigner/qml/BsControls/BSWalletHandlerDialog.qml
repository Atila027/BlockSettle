/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.9
import QtQuick.Controls 2.4
import QtQuick.Layouts 1.11

import "../StyledControls"
import "../js/helper.js" as JsHelper

CustomTitleDialogWindow {
    id: root

    property bool rejectedDialogWasShown: false

    function showWalletError(errorText) {
        if (checkShownFlag()) {
            return;
        }

        var mb = JsHelper.messageBox(BSMessageBox.Type.Critical
            , qsTr("Wallet"), errorText
            , qsTr("Wallet Name: %1\nWallet ID: %2").arg(walletInfo.name).arg(walletInfo.rootId))
        mb.bsAccepted.connect(function() { root.rejectAnimated() })
        root.setNextChainDialog(mb);
    }

    function rejectWithNoError() {
        if (checkShownFlag()) {
            return;
        }

        root.rejectedDialogWasShown = true;
        root.rejectAnimated();
    }

    function checkShownFlag() {
        if (root.rejectedDialogWasShown) {
            return true;
        }

        root.rejectedDialogWasShown = true;
        return false;
    }
}

