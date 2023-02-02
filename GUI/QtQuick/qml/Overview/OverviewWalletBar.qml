/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
import QtQuick 2.15
import wallet.balance 1.0

import "." as OverviewControls
import "../StyledControls" as Controls

Rectangle {
    id: control

    width: 1200
    height: 100
    color: "transparent"

    signal requestWalletProperties()
    signal createNewWallet()
    signal walletIndexChanged(index : int)

    Row {
        anchors.fill:parent
        spacing: 20

        Controls.CustomComboBox {
            id: wallet_selection_combobox
            anchors.verticalCenter: parent.verticalCenter

            width: 263
            height: 53

            objectName: "walletsComboBox"
            model: walletBalances
            textRole: "name"
            valueRole: "name"

            Component.onCompleted: {
                control.walletIndexChanged(0)
            }

            onActivated: (ind) => {
                control.walletIndexChanged(ind)
            }
        }

        OverviewControls.BalanceBar {
            id: balance_bar
            anchors.verticalCenter: parent.verticalCenter

            confirmed_balance_value: walletBalances.data(walletBalances.index(wallet_selection_combobox.currentIndex, 0),
                                                         WalletBalance.ConfirmedRole)
            uncorfirmed_balance_value: walletBalances.data(walletBalances.index(wallet_selection_combobox.currentIndex, 0),
                                                           WalletBalance.UnconfirmedRole)
            total_balance_value: walletBalances.data(walletBalances.index(wallet_selection_combobox.currentIndex, 0),
                                                     WalletBalance.TotalRole)
            used_addresses_value: walletBalances.data(walletBalances.index(wallet_selection_combobox.currentIndex, 0),
                                                WalletBalance.NbAddrRole)
        }
    }

    Row {
        spacing: 10
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right

        Controls.CustomMediumButton {
            text: "Wallet properties"
            onClicked: control.requestWalletProperties()
        }

        Controls.CustomMediumButton {
            text: "Create new wallet"
            onClicked: control.createNewWallet()
        }
    }
}
