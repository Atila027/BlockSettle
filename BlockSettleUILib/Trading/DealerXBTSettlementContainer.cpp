/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "DealerXBTSettlementContainer.h"

#include "AuthAddressManager.h"
#include "CheckRecipSigner.h"
#include "CurrencyPair.h"
#include "QuoteProvider.h"
#include "TradesUtils.h"
#include "TradesVerification.h"
#include "UiUtils.h"
#include "UtxoReservationManager.h"
#include "WalletSignerContainer.h"
#include "Wallets/SyncHDWallet.h"
#include "Wallets/SyncWalletsManager.h"

#include <spdlog/spdlog.h>

#include <QApplication>

Q_DECLARE_METATYPE(AddressVerificationState)

using namespace bs::sync;

DealerXBTSettlementContainer::DealerXBTSettlementContainer(const std::shared_ptr<spdlog::logger> &logger
   , const bs::network::Order &order
   , const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
   , const std::shared_ptr<bs::sync::hd::Wallet> &xbtWallet
   , const std::shared_ptr<QuoteProvider> &quoteProvider
   , const std::shared_ptr<WalletSignerContainer> &container
   , const std::shared_ptr<ArmoryConnection> &armory
   , const std::shared_ptr<AuthAddressManager> &authAddrMgr
   , const bs::Address &authAddr
   , const std::vector<UTXO> &utxosPayinFixed
   , const bs::Address &recvAddr
   , const std::shared_ptr<bs::UTXOReservationManager> &utxoReservationManager
   , std::unique_ptr<bs::hd::Purpose> walletPurpose
   , bs::UtxoReservationToken utxoRes
   , bool expandTxDialogInfo
   , uint64_t tier1XbtLimit )
   : bs::SettlementContainer(std::move(utxoRes), std::move(walletPurpose), expandTxDialogInfo)
   , order_(order)
   , weSellXbt_((order.side == bs::network::Side::Buy) != (order.product == bs::network::XbtCurrency))
   , amount_((order.product != bs::network::XbtCurrency) ? order.quantity / order.price : order.quantity)
   , logger_(logger)
   , armory_(armory)
   , walletsMgr_(walletsMgr)
   , xbtWallet_(xbtWallet)
   , signContainer_(container)
   , authAddrMgr_(authAddrMgr)
   , utxosPayinFixed_(utxosPayinFixed)
   , recvAddr_(recvAddr)
   , authAddr_(authAddr)
   , utxoReservationManager_(utxoReservationManager)
{
   qRegisterMetaType<AddressVerificationState>();

   CurrencyPair cp(security());
   fxProd_ = cp.ContraCurrency(bs::network::XbtCurrency);

   if (!xbtWallet_) {
      throw std::runtime_error("no wallet");
   }

   auto qn = quoteProvider->getSubmittedXBTQuoteNotification(order.settlementId);
   if (qn.authKey.empty() || qn.reqAuthKey.empty() || qn.settlementId.empty()) {
      throw std::invalid_argument("failed to get submitted QN for " + order.quoteId);
   }

   const auto xbtAmount = bs::XBTAmount(amount_);
   requesterAddressShouldBeVerified_ = xbtAmount > bs::XBTAmount(tier1XbtLimit);

   // BST-2545: Use price as it see Genoa (and it computes it as ROUNDED_CCY / XBT)
   const auto actualXbtPrice = UiUtils::actualXbtPrice(xbtAmount, price());

   auto side = order.product == bs::network::XbtCurrency ? order.side : bs::network::Side::invert(order.side);
   comment_ = fmt::format("{} {} @ {}", bs::network::Side::toString(side)
      , order.security, UiUtils::displayPriceXBT(actualXbtPrice).toStdString());
   authKey_ = BinaryData::CreateFromHex(qn.authKey);
   reqAuthKey_ = BinaryData::CreateFromHex(qn.reqAuthKey);
   if (authKey_.empty() || reqAuthKey_.empty()) {
      throw std::runtime_error("missing auth key");
   }

   init(armory_.get());
   settlementIdHex_ = qn.settlementId;
   settlementId_ = BinaryData::CreateFromHex(qn.settlementId);
   settlWallet_ = armory_->instantiateWallet(settlementIdHex_);
   if (!settlWallet_) {
      throw std::runtime_error("can't register settlement wallet in armory");
   }

   connect(signContainer_.get(), &SignContainer::TXSigned, this, &DealerXBTSettlementContainer::onTXSigned);
}

bool DealerXBTSettlementContainer::cancel()
{
   if (payinSignId_ != 0 || payoutSignId_ != 0) {
      signContainer_->CancelSignTx(settlementId_);
   }

   releaseUtxoRes();

   SPDLOG_LOGGER_DEBUG(logger_, "cancel on a trade : {}", settlementIdHex_);

   emit timerExpired();
   return true;
}

DealerXBTSettlementContainer::~DealerXBTSettlementContainer()
{
   settlWallet_->unregister();
   cleanup();
}

bs::sync::PasswordDialogData DealerXBTSettlementContainer::toPasswordDialogData(QDateTime timestamp) const
{
   bs::sync::PasswordDialogData dialogData = SettlementContainer::toPasswordDialogData(timestamp);

   dialogData.setValue(PasswordDialogData::IsDealer, true);
   dialogData.setValue(PasswordDialogData::Market, "XBT");
   dialogData.setValue(PasswordDialogData::AutoSignCategory, static_cast<int>(bs::signer::AutoSignCategory::SettlementDealer));

   // rfq details
   QString qtyProd = UiUtils::XbtCurrency;

   dialogData.setValue(PasswordDialogData::Title, tr("Settlement Pay-In"));
   dialogData.setValue(PasswordDialogData::Price, UiUtils::displayPriceXBT(price()));
   dialogData.setValue(PasswordDialogData::FxProduct, fxProd_);

   bool isFxProd = (product() != bs::network::XbtCurrency);

   if (isFxProd) {
      dialogData.setValue(PasswordDialogData::Quantity, tr("%1 %2")
                    .arg(UiUtils::displayAmountForProduct(quantity(), QString::fromStdString(fxProd_), bs::network::Asset::Type::SpotXBT))
                    .arg(QString::fromStdString(fxProd_)));

      dialogData.setValue(PasswordDialogData::TotalValue, tr("%1 XBT")
                    .arg(UiUtils::displayAmount(quantity() / price())));
   } else {
      dialogData.setValue(PasswordDialogData::Quantity, tr("%1 XBT")
                    .arg(UiUtils::displayAmount(amount())));

      dialogData.setValue(PasswordDialogData::TotalValue, tr("%1 %2")
                    .arg(UiUtils::displayAmountForProduct(amount() * price(), QString::fromStdString(fxProd_), bs::network::Asset::Type::SpotXBT))
                    .arg(QString::fromStdString(fxProd_)));
   }

   // settlement details
   dialogData.setValue(PasswordDialogData::SettlementId, settlementId_.toHexStr());
   dialogData.setValue(PasswordDialogData::SettlementAddress, settlAddr_.display());

   dialogData.setValue(PasswordDialogData::RequesterAuthAddress,
      bs::Address::fromPubKey(reqAuthKey_, AddressEntryType_P2WPKH).display());
   dialogData.setValue(PasswordDialogData::RequesterAuthAddressVerified, requestorAddressState_ == AddressVerificationState::Verified);
   dialogData.setValue(PasswordDialogData::SigningAllowed, requestorAddressState_ == AddressVerificationState::Verified);

   dialogData.setValue(PasswordDialogData::ResponderAuthAddress,
      bs::Address::fromPubKey(authKey_, AddressEntryType_P2WPKH).display());
   dialogData.setValue(PasswordDialogData::ResponderAuthAddressVerified, true);

   // tx details
   dialogData.setValue(PasswordDialogData::TxInputProduct, UiUtils::XbtCurrency);

   return dialogData;
}

void DealerXBTSettlementContainer::activate()
{
   startTimer(kWaitTimeoutInSec);

   addrVerificator_ = std::make_shared<AddressVerificator>(logger_, armory_
      , [this, handle = validityFlag_.handle()](const bs::Address &address, AddressVerificationState state)
   {
      QMetaObject::invokeMethod(qApp, [this, handle, address, state] {
         if (!handle.isValid()) {
            return;
         }

         SPDLOG_LOGGER_INFO(logger_, "counterparty's address verification {} for {}"
            , to_string(state), address.display());
         requestorAddressState_ = state;

         if (state == AddressVerificationState::Verified) {
            // we verify only requester's auth address
            bs::sync::PasswordDialogData dialogData;
            dialogData.setValue(PasswordDialogData::RequesterAuthAddressVerified, true);
            dialogData.setValue(PasswordDialogData::SettlementId, settlementId_.toHexStr());
            dialogData.setValue(PasswordDialogData::SigningAllowed, true);

            signContainer_->updateDialogData(dialogData);
         }
      });
   });

   addrVerificator_->SetBSAddressList(authAddrMgr_->GetBSAddresses());

   if (requesterAddressShouldBeVerified_) {
      const auto reqAuthAddrSW = bs::Address::fromPubKey(reqAuthKey_, AddressEntryType_P2WPKH);
      addrVerificator_->addAddress(reqAuthAddrSW);
      addrVerificator_->startAddressVerification();
   } else {
      requestorAddressState_ = AddressVerificationState::Verified;
   }

   const auto &authLeaf = walletsMgr_->getAuthWallet();
   signContainer_->setSettlAuthAddr(authLeaf->walletId(), settlementId_, authAddr_);
}

void DealerXBTSettlementContainer::deactivate()
{
   stopTimer();
}

void DealerXBTSettlementContainer::onTXSigned(unsigned int idReq, BinaryData signedTX
   , bs::error::ErrorCode errCode, std::string errMsg)
{
   if (payoutSignId_ && (payoutSignId_ == idReq)) {
      payoutSignId_ = 0;

      if (errCode == bs::error::ErrorCode::TxCancelled) {
         emit cancelTrade(settlementIdHex_);
         return;
      }

      if ((errCode != bs::error::ErrorCode::NoError) || signedTX.empty()) {
         SPDLOG_LOGGER_ERROR(logger_, "failed to sign pay-out: {} ({})", int(errCode), errMsg);
         failWithErrorText(tr("Failed to sign pay-out"), errCode);
         return;
      }

      bs::tradeutils::PayoutVerifyArgs verifyArgs;
      verifyArgs.signedTx = signedTX;
      verifyArgs.settlAddr = settlAddr_;
      verifyArgs.usedPayinHash = expectedPayinHash_;
      verifyArgs.amount = bs::XBTAmount(amount_);
      auto verifyResult = bs::tradeutils::verifySignedPayout(verifyArgs);
      if (!verifyResult.success) {
         SPDLOG_LOGGER_ERROR(logger_, "payout verification failed: {}", verifyResult.errorMsg);
         failWithErrorText(tr("Payin verification failed"), errCode);
         return;
      }

      for (const auto &leaf : xbtWallet_->getGroup(bs::sync::hd::Wallet::getXBTGroupType())->getLeaves()) {
         leaf->setTransactionComment(signedTX, comment_);
      }
//      settlWallet_->setTransactionComment(signedTX, comment_);   //TODO: implement later

      SPDLOG_LOGGER_DEBUG(logger_, "signed payout: {}", signedTX.toHexStr());

      emit sendSignedPayoutToPB(settlementIdHex_, signedTX);

      // ok. there is nothing this container could/should do
//      emit completed(id());
   }

   if ((payinSignId_ != 0) && (payinSignId_ == idReq)) {
      payinSignId_ = 0;

      if (errCode == bs::error::ErrorCode::TxCancelled) {
         emit cancelTrade(settlementIdHex_);
         return;
      }

      if ((errCode != bs::error::ErrorCode::NoError) || signedTX.empty()) {
         SPDLOG_LOGGER_ERROR(logger_, "Failed to sign pay-in: {} ({})", (int)errCode, errMsg);
         if (errCode == bs::error::ErrorCode::TxSpendLimitExceed) {
            failWithErrorText(tr("Auto-signing (and auto-quoting) have been disabled"
               " as your limit has been hit or elapsed"), errCode);
         }
         else {
            failWithErrorText(tr("Failed to sign Pay-in"), errCode);
         }
         return;
      }

      try {
         const Tx tx(signedTX);
         if (!tx.isInitialized()) {
            throw std::runtime_error("uninited TX");
         }

         if (tx.getThisHash() != expectedPayinHash_) {
            failWithErrorText(tr("payin hash mismatch"), bs::error::ErrorCode::TxInvalidRequest);
            return;
         }
      } catch (const std::exception &e) {
         failWithErrorText(tr("invalid signed pay-in"), bs::error::ErrorCode::TxInvalidRequest);
         return;
      }

      for (const auto &leaf : xbtWallet_->getGroup(bs::sync::hd::Wallet::getXBTGroupType())->getLeaves()) {
         leaf->setTransactionComment(signedTX, comment_);
      }
//      settlWallet_->setTransactionComment(signedTX, comment_);  //TODO: implement later

      emit sendSignedPayinToPB(settlementIdHex_, signedTX);
      logger_->debug("[DealerXBTSettlementContainer::onTXSigned] Payin sent");

      // ok. there is nothing this container could/should do
//      emit completed(id());
   }
}

void DealerXBTSettlementContainer::onZCReceived(const std::string &
   , const std::vector<bs::TXEntry> &entries)
{
   const auto &cbTX = [this](const Tx &tx)
   {
      if (tx.getNumTxIn() != 1) {   // not a pay-out
         return;
      }
      const auto &txIn = tx.getTxInCopy(0);
      const auto &op = txIn.getOutPoint();
      if (op.getTxHash() != expectedPayinHash_) {
         return;  // not our pay-out
      }
      const auto &serializedTx = tx.serialize();
      const auto &buyAuthKey = weSellXbt_ ? reqAuthKey_ : authKey_;
      const auto &sellAuthKey = weSellXbt_ ? authKey_ : reqAuthKey_;
      const auto &result = bs::TradesVerification::verifySignedPayout(serializedTx
         , buyAuthKey.toHexStr(), sellAuthKey.toHexStr(), expectedPayinHash_
         , bs::XBTAmount(amount_).GetValue(), utxoReservationManager_->feeRatePb()
         , settlementIdHex_, settlAddr_.display());
      if (result->success) {
         emit completed(id());
      }
      else {
         emit failed(id());
      }
   };
   
   for (const auto &entry : entries) {
      if (entry.walletIds.find(settlementIdHex_) == entry.walletIds.end()) {
         continue;
      }
      if (entry.txHash == expectedPayinHash_) {
         continue;   // not interested in pay-in
      }
      armory_->getTxByHash(entry.txHash, cbTX, true);
   }
}

void DealerXBTSettlementContainer::onUnsignedPayinRequested(const std::string& settlementId)
{
   if (settlementIdHex_ != settlementId) {
      // ignore
      return;
   }

   if (!weSellXbt_) {
      SPDLOG_LOGGER_ERROR(logger_, "dealer is buying. Should not create unsigned payin on {}", settlementIdHex_);
      return;
   }

   bs::tradeutils::PayinArgs args;
   initTradesArgs(args, settlementId);
   args.fixedInputs = utxosPayinFixed_;

   const auto xbtGroup = xbtWallet_->getGroup(xbtWallet_->getXBTGroupType());
   if (!xbtWallet_->canMixLeaves()) {
      assert(walletPurpose_);
      const auto leaf = xbtGroup->getLeaf(*walletPurpose_);
      args.inputXbtWallets.push_back(leaf);
   }
   else {
      for (const auto &leaf : xbtGroup->getLeaves()) {
         args.inputXbtWallets.push_back(leaf);
      }
   }

   args.utxoReservation = bs::UtxoReservation::instance();

   auto payinCb = bs::tradeutils::PayinResultCb([this, handle = validityFlag_.handle()]
      (bs::tradeutils::PayinResult result)
   {
      QMetaObject::invokeMethod(qApp, [this, handle, result = std::move(result)] {
         if (!handle.isValid()) {
            return;
         }

         if (!result.success) {
            SPDLOG_LOGGER_ERROR(logger_, "creating payin request failed: {}", result.errorMsg);
            failWithErrorText(tr("creating payin request failed"), bs::error::ErrorCode::InternalError);
            return;
         }

         settlAddr_ = result.settlementAddr;
         settlWallet_->registerAddresses({ settlAddr_.prefixed() }, true);

         unsignedPayinRequest_ = std::move(result.signRequest);
         // Reserve only automatic UTXO selection
         if (utxosPayinFixed_.empty()) {
            utxoRes_ = utxoReservationManager_->makeNewReservation(unsignedPayinRequest_.getInputs(nullptr), id());
         }

         emit sendUnsignedPayinToPB(settlementIdHex_
            , bs::network::UnsignedPayinData{ unsignedPayinRequest_.serializeState().SerializeAsString() });

         const auto &authLeaf = walletsMgr_->getAuthWallet();
         signContainer_->setSettlCP(authLeaf->walletId(), result.payinHash, settlementId_, reqAuthKey_);
      });
   });

   bs::tradeutils::createPayin(std::move(args), std::move(payinCb));
}

void DealerXBTSettlementContainer::onSignedPayoutRequested(const std::string& settlementId
   , const BinaryData& payinHash, QDateTime timestamp)
{
   if (settlementIdHex_ != settlementId) {
      // ignore
      return;
   }

   startTimer(kWaitTimeoutInSec);

   if (weSellXbt_) {
      SPDLOG_LOGGER_ERROR(logger_, "dealer is selling. Should not sign payout on {}", settlementIdHex_);
      return;
   }

   expectedPayinHash_ = payinHash;

   bs::tradeutils::PayoutArgs args;
   initTradesArgs(args, settlementId);
   args.payinTxId = payinHash;
   args.recvAddr = recvAddr_;

   const auto xbtGroup = xbtWallet_->getGroup(xbtWallet_->getXBTGroupType());
   if (!xbtWallet_->canMixLeaves()) {
      assert(walletPurpose_);
      const auto leaf = xbtGroup->getLeaf(*walletPurpose_);
      args.outputXbtWallet = leaf;
   }
   else {
      args.outputXbtWallet = xbtGroup->getLeaves().at(0);
   }

   auto payoutCb = bs::tradeutils::PayoutResultCb([this, payinHash, timestamp, handle = validityFlag_.handle()]
      (bs::tradeutils::PayoutResult result)
   {
      QMetaObject::invokeMethod(qApp, [this, payinHash, handle, timestamp, result = std::move(result)] {
         if (!handle.isValid()) {
            return;
         }

         if (!result.success) {
            SPDLOG_LOGGER_ERROR(logger_, "creating payout failed: {}", result.errorMsg);
            failWithErrorText(tr("creating payout failed"), bs::error::ErrorCode::InternalError);
            return;
         }

         settlAddr_ = result.settlementAddr;
         settlWallet_->registerAddresses({ settlAddr_.prefixed() }, true);

         bs::sync::PasswordDialogData dlgData = toPayOutTxDetailsPasswordDialogData(result.signRequest, timestamp);
         dlgData.setValue(PasswordDialogData::IsDealer, true);
         dlgData.setValue(PasswordDialogData::Market, "XBT");
         dlgData.setValue(PasswordDialogData::SettlementId, settlementId_.toHexStr());
         dlgData.setValue(PasswordDialogData::AutoSignCategory, static_cast<int>(bs::signer::AutoSignCategory::SettlementDealer));

         //note: signRequest should propably be a shared_ptr
         auto signerObj = result.signRequest;
         payoutSignId_ = signContainer_->signSettlementPayoutTXRequest(signerObj
            , { settlementId_, reqAuthKey_, true }, dlgData);
      });
   });
   bs::tradeutils::createPayout(std::move(args), std::move(payoutCb));

   const auto &authLeaf = walletsMgr_->getAuthWallet();
   signContainer_->setSettlCP(authLeaf->walletId(), payinHash, settlementId_, reqAuthKey_);
}

void DealerXBTSettlementContainer::onSignedPayinRequested(const std::string& settlementId
   , const BinaryData &, const BinaryData& payinHash, QDateTime timestamp)
{
   if (settlementIdHex_ != settlementId) {
      // ignore
      return;
   }

   if (payinHash.empty()) {
      failWithErrorText(tr("Invalid Sign Pay-In request"), bs::error::ErrorCode::InternalError);
      return;
   }

   expectedPayinHash_ = payinHash;

   startTimer(kWaitTimeoutInSec);

   SPDLOG_LOGGER_DEBUG(logger_, "start sign payin: {}", settlementId);

   if (!weSellXbt_) {
      SPDLOG_LOGGER_ERROR(logger_, "dealer is buying. Should not sign payin on {}", settlementIdHex_);
      return;
   }

   if (!unsignedPayinRequest_.isValid()) {
      SPDLOG_LOGGER_ERROR(logger_, "unsigned payin request is invalid: {}", settlementIdHex_);
      failWithErrorText(tr("Invalid unsigned pay-in"), bs::error::ErrorCode::InternalError);
      return;
   }

   bs::sync::PasswordDialogData dlgData = toPasswordDialogData(timestamp);
   dlgData.setValue(PasswordDialogData::SettlementPayInVisible, true);

   payinSignId_ = signContainer_->signSettlementTXRequest(unsignedPayinRequest_, dlgData, SignContainer::TXSignMode::Full);
}

void DealerXBTSettlementContainer::failWithErrorText(const QString &errorMessage, bs::error::ErrorCode code)
{
   SettlementContainer::releaseUtxoRes();

   emit cancelTrade(settlementIdHex_);

   emit error(id(), code, errorMessage);
   emit failed(id());
}

void DealerXBTSettlementContainer::initTradesArgs(bs::tradeutils::Args &args, const std::string &settlementId)
{
   args.amount = bs::XBTAmount{amount_};
   args.settlementId = BinaryData::CreateFromHex(settlementId);
   args.ourAuthAddress = authAddr_;
   args.cpAuthPubKey = reqAuthKey_;
   args.walletsMgr = walletsMgr_;
   args.armory = armory_;
   args.signContainer = signContainer_;
   args.feeRatePb_ = utxoReservationManager_->feeRatePb();
}
