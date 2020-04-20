/*

***********************************************************************************
* Copyright (C) 2016 - , BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "UtxoReservationManager.h"

#include <cassert>
#include <spdlog/spdlog.h>

#include "UtxoReservation.h"
#include "UtxoReservationToken.h"
#include "Wallets/SyncHDWallet.h"
#include "Wallets/SyncWalletsManager.h"
#include "TradesUtils.h"
#include "ArmoryObject.h"
#include "WalletUtils.h"

using namespace bs;

UTXOReservationManager::UTXOReservationManager(const std::shared_ptr<bs::sync::WalletsManager>& walletsManager,
   const std::shared_ptr<ArmoryObject>& armory, const std::shared_ptr<spdlog::logger>& logger, QObject* parent /*= nullptr*/)
   : walletsManager_(walletsManager)
   , armory_(armory)
   , logger_(logger)
{
   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletsSynchronized,
      this, &UTXOReservationManager::refreshAvailableUTXO, Qt::QueuedConnection);
   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletAdded,
      this, &UTXOReservationManager::onWalletsAdded);
   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletDeleted,
      this, &UTXOReservationManager::onWalletsDeleted);
   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletBalanceUpdated,
      this, &UTXOReservationManager::onWalletsBalanceChanged);
}

UTXOReservationManager::~UTXOReservationManager() = default;

bs::UtxoReservationToken UTXOReservationManager::makeNewReservation(const std::vector<UTXO> &utxos, const std::string &reserveId)
{
   auto onReleaseCb = [mngr = QPointer<UTXOReservationManager>(this)]() {
      if (!mngr) {
         return;
      }
      mngr->availableUtxoChanged({});
   };

   auto reservation = bs::UtxoReservationToken::makeNewReservation(logger_, utxos, reserveId, onReleaseCb);
   // #ReservationMngr: could be optimized by updating only needed wallet
   availableUtxoChanged({});
   return reservation;
}

bs::UtxoReservationToken bs::UTXOReservationManager::makeNewReservation(const std::vector<UTXO> &utxos)
{
   auto reserveId = fmt::format("rfq_reserve_{}", CryptoPRNG::generateRandom(8).toHexStr());
   return makeNewReservation(utxos, reserveId);
}

void UTXOReservationManager::reserveBestXbtUtxoSet(const HDWalletId& walletId, BTCNumericTypes::satoshi_type quantity, bool partial,
   std::function<void(FixedXbtInputs&&)>&& cb, bool checkPbFeeFloor)
{
   auto bestUtxoSetCb = [mgr = QPointer<bs::UTXOReservationManager>(this), walletId, partial
         , cbFixedXBT = std::move(cb)](std::vector<UTXO>&& utxos) {
      if (!mgr) {
         return;
      }

      FixedXbtInputs fixedXbtInputs;
      if (partial) {
         fixedXbtInputs = std::move(mgr->convertUtxoToPartialFixedInput(walletId, utxos));
      }
      else {
         fixedXbtInputs = std::move(mgr->convertUtxoToFixedInput(walletId, utxos));
      }
      fixedXbtInputs.utxoRes = mgr->makeNewReservation(utxos);

      cbFixedXBT(std::move(fixedXbtInputs));
   };

   getBestXbtUtxoSet(walletId, quantity, bestUtxoSetCb, checkPbFeeFloor);
}

BTCNumericTypes::satoshi_type bs::UTXOReservationManager::getAvailableXbtUtxoSum(const HDWalletId& walletId) const
{
   BTCNumericTypes::satoshi_type sum = 0;
   auto const availableUtxos = getAvailableXbtUTXOs(walletId);
   for (const auto &utxo : availableUtxos) {
      sum += utxo.getValue();
   }

   return sum;
}

std::vector<UTXO> bs::UTXOReservationManager::getAvailableXbtUTXOs(const HDWalletId& walletId) const
{
   auto const availableUtxos = availableXbtUTXOs_.find(walletId);
   if (availableUtxos == availableXbtUTXOs_.end()) {
      return {};
   }

   std::vector<UTXO> utxos;
   utxos = availableUtxos->second.availableUtxo_;
   UtxoReservation::instance()->filter(utxos);
   return utxos;
}

void bs::UTXOReservationManager::getBestXbtUtxoSet(const HDWalletId& walletId,
   BTCNumericTypes::satoshi_type quantity, std::function<void(std::vector<UTXO>&&)>&& cb, bool checkPbFeeFloor)
{
   auto walletUtxos = getAvailableXbtUTXOs(walletId);
   std::vector<UTXO> selectedUtxos = bs::selectUtxoForAmount(std::move(walletUtxos), quantity);

   // Here we calculating fee based on chosen utxos, if total price with fee will cover by all utxo sum - then we good and could continue
   // otherwise let's try to find better set of utxo again till the moment we will cover the difference or use all available utxos from wallet
   auto feeCb = [mgr = QPointer<bs::UTXOReservationManager>(this), quantity, walletId
         , utxos = std::move(selectedUtxos), cbCopy = std::move(cb), checkPbFeeFloor](float fee) mutable {
      if (!mgr) {
         return; // main thread die, nothing to do
      }

      QMetaObject::invokeMethod(mgr, [mgr, quantity, walletId, fee, utxos = std::move(utxos), cb = std::move(cbCopy), checkPbFeeFloor]() mutable {
         float feePerByte = ArmoryConnection::toFeePerByte(fee);
         if (checkPbFeeFloor) {
            feePerByte = std::max(mgr->feeRatePb(), feePerByte);
         }
         BTCNumericTypes::satoshi_type total = 0;
         for (const auto &utxo : utxos) {
            total += utxo.getValue();
         }
         const auto fee = bs::tradeutils::estimatePayinFeeWithoutChange(utxos, feePerByte);

         const BTCNumericTypes::satoshi_type spendableQuantity = quantity + fee;
         if (spendableQuantity > total) {
            mgr->getBestXbtUtxoSet(walletId, spendableQuantity, std::move(cb), checkPbFeeFloor);
         }
         else {
            cb(std::move(utxos));
         }
      });
   };
   armory_->estimateFee(bs::tradeutils::feeTargetBlockCount(), feeCb);
}

BTCNumericTypes::balance_type bs::UTXOReservationManager::getAvailableCCUtxoSum(const CCProductName& CCProduct) const
{
   const auto& ccWallet = walletsManager_->getCCWallet(CCProduct);
   if (!ccWallet) {
      return {};
   }
   
   BTCNumericTypes::satoshi_type sum = 0;
   auto const availableUtxos = getAvailableCCUTXOs(ccWallet->walletId());
   for (const auto &utxo : availableUtxos) {
      sum += utxo.getValue();
   }

   return ccWallet->getTxBalance(sum);
}

std::vector<UTXO> bs::UTXOReservationManager::getAvailableCCUTXOs(const CCWalletId& walletId) const
{
   std::vector<UTXO> utxos;
   auto const availableUtxos = availableCCUTXOs_.find(walletId);
   if (availableUtxos == availableCCUTXOs_.end()) {
      return {};
   }

   utxos = availableUtxos->second;
   UtxoReservation::instance()->filter(utxos);
   return utxos;
}

bs::FixedXbtInputs UTXOReservationManager::convertUtxoToFixedInput(const HDWalletId& walletId, const std::vector<UTXO>& utxos)
{
   FixedXbtInputs fixedXbtInputs;
   for (auto utxo : utxos) {
      fixedXbtInputs.inputs.insert({ utxo, walletId });
   }
   return fixedXbtInputs;
}

bs::FixedXbtInputs bs::UTXOReservationManager::convertUtxoToPartialFixedInput(const HDWalletId& walletId, const std::vector<UTXO>& utxos)
{
   auto const availableUtxos = availableXbtUTXOs_.find(walletId);
   if (availableUtxos == availableXbtUTXOs_.end()) {
      return {};
   }

   const auto &utxoLookup = availableUtxos->second.utxosLookup_;
   FixedXbtInputs fixedXbtInputs;
   for (auto utxo : utxos) {
      fixedXbtInputs.inputs.insert({ utxo, utxoLookup.at(utxo) });
   }
   return fixedXbtInputs;
}

void UTXOReservationManager::setFeeRatePb(float feeRate)
{
   feeRatePb_.store(feeRate);
}

float UTXOReservationManager::feeRatePb() const
{
   return feeRatePb_.load();
}

void bs::UTXOReservationManager::refreshAvailableUTXO()
{
   availableXbtUTXOs_.clear();
   for (auto &wallet : walletsManager_->hdWallets()) {
      resetHdWallet(wallet->walletId());
   }
}

void bs::UTXOReservationManager::onWalletsDeleted(const std::string& walledId)
{
   availableXbtUTXOs_.erase(walledId);
   availableCCUTXOs_.erase(walledId);
   if (!walletsManager_->hasPrimaryWallet()) {
      availableCCUTXOs_.clear();
   }
}

void bs::UTXOReservationManager::onWalletsAdded(const std::string& walledId)
{
   if (resetHdWallet(walledId)) {
      return;
   }

   const auto wallet = walletsManager_->getWalletById(walledId);
   if (!wallet) {
      return;
   }


   switch (wallet->type())
   {
   case bs::core::wallet::Type::ColorCoin:
      resetSpendableCC(wallet);
      break;
   case bs::core::wallet::Type::Bitcoin:
   {
      auto hdWallet = walletsManager_->getHDRootForLeaf(walledId);
      if (hdWallet) {
         resetSpendableXbt(hdWallet);
      }
   }
      break;
   default:
      break;
   }
}

void bs::UTXOReservationManager::onWalletsBalanceChanged(const std::string& walledId)
{
   onWalletsDeleted(walledId);
   onWalletsAdded(walledId);
}

bool bs::UTXOReservationManager::resetHdWallet(const std::string& hdWalledId)
{
   auto hdWallet = walletsManager_->getHDWalletById(hdWalledId);
   if (!hdWallet) {
      return false;
   }

   resetSpendableXbt(hdWallet);

   if (hdWallet->isPrimary()) {
      resetAllSpendableCC(hdWallet);
   }

   return true;
}

void bs::UTXOReservationManager::resetSpendableXbt(const std::shared_ptr<bs::sync::hd::Wallet>& hdWallet)
{
   assert(hdWallet);
   auto leaves = hdWallet->getGroup(hdWallet->getXBTGroupType())->getLeaves();
   std::vector<bs::sync::WalletsManager::WalletPtr> wallets;
   for (const auto &leaf : leaves) {
      auto purpose = leaf->purpose();
      // Filter non-segwit leaves (for HW wallets)
      if (purpose == bs::hd::Purpose::Native || purpose == bs::hd::Purpose::Nested) {
         wallets.push_back(leaf);
      }
   }

   bs::tradeutils::getSpendableTxOutList(wallets, [mgr = QPointer<bs::UTXOReservationManager>(this)
      , walletId = hdWallet->walletId(), leaves]
         (const std::map<UTXO, std::string> &utxos) {
      if (!mgr) {
         return; // manager thread die, nothing to do
      }

      XBTUtxoContainer utxosContainer;
      utxosContainer.utxosLookup_ = utxos;
      for (const auto &utxo : utxos) {
         utxosContainer.availableUtxo_.push_back(utxo.first);
      }
      QMetaObject::invokeMethod(mgr, [mgr, container = std::move(utxosContainer), id = walletId]{
         mgr->availableXbtUTXOs_[id] = std::move(container);
         emit mgr->availableUtxoChanged(id);
         });
   }, false);
}

void bs::UTXOReservationManager::resetSpendableCC(const std::shared_ptr<bs::sync::Wallet>& leaf)
{
   assert(leaf && leaf->type() == bs::core::wallet::Type::ColorCoin);
   if (!leaf->isBalanceAvailable()) {
      return;
   }

   bs::tradeutils::getSpendableTxOutList({ leaf }, [mgr = QPointer<bs::UTXOReservationManager>(this),
      walletId = leaf->walletId()](const std::map<UTXO, std::string> &utxos) {
      if (!mgr) {
         return; // manager thread die, nothing to do
      }

      std::vector<UTXO> walletUtxos;
      for (const auto &utxo : utxos) {
         walletUtxos.push_back(utxo.first);
      }

      QMetaObject::invokeMethod(mgr, [mgr, utxos = std::move(walletUtxos), id = walletId]{
         mgr->availableCCUTXOs_[id] = std::move(utxos);

         emit mgr->availableUtxoChanged(id);
      });
   }, false);
}

void bs::UTXOReservationManager::resetAllSpendableCC(const std::shared_ptr<bs::sync::hd::Wallet>& hdWallet)
{
   auto ccGroup = hdWallet->getGroup(bs::hd::CoinType::BlockSettle_CC);
   if (!ccGroup) {
      return;
   }

   for (const auto &leaf : ccGroup->getLeaves()) {
      resetSpendableCC(leaf);
   }
}
