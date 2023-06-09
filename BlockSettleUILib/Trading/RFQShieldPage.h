/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef RFQREPLYLOGINREQUIREDSHIELD_H
#define RFQREPLYLOGINREQUIREDSHIELD_H

#include "WalletShieldBase.h"

class RFQShieldPage : public WalletShieldBase
{
   Q_OBJECT

public:
   explicit RFQShieldPage(QWidget *parent = nullptr);
   ~RFQShieldPage() noexcept override;

   void showShieldLoginToSubmitRequired();
   void showShieldLoginToResponseRequired();
   void showShieldReservedTradingParticipant();
   void showShieldReservedDealingParticipant();
   void showShieldSelectTargetTrade();
   void showShieldSelectTargetDealing();

   void setLoginEnabled(bool enabled);

private:
   bool loginEnabled_{};
};

#endif // RFQREPLYLOGINREQUIREDSHIELD_H
