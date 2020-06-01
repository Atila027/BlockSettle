/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "TransactionDetailDialog.h"
#include "ui_TransactionDetailDialog.h"
#include "TransactionsViewModel.h"

#include <BTCNumericTypes.h>
#include <TxClasses.h>
#include "Wallets/SyncHDWallet.h"
#include "Wallets/SyncWalletsManager.h"
#include "UiUtils.h"

#include <QDateTime>
#include <QLabel>
#include <QMenu>
#include <QClipboard>

#include <spdlog/spdlog.h>

#include <limits>


TransactionDetailDialog::TransactionDetailDialog(const TransactionPtr &tvi
   , const std::shared_ptr<bs::sync::WalletsManager> &walletsManager
   , const std::shared_ptr<ArmoryConnection> &armory, QWidget* parent)
 : QDialog(parent)
 , ui_(new Ui::TransactionDetailDialog())
 , walletsManager_(walletsManager)
{
   ui_->setupUi(this);
   itemSender_ = new QTreeWidgetItem(QStringList(tr("Sender")));
   itemReceiver_ = new QTreeWidgetItem(QStringList(tr("Receiver")));

   const auto &cbInit = [this, armory, handle = validityFlag_.handle()](const TransactionPtr &item) mutable {
      ValidityGuard guard(handle);
      if (!handle.isValid()) {
         return;
      }
      ui_->labelAmount->setText(item->amountStr);
      ui_->labelDirection->setText(tr(bs::sync::Transaction::toString(item->direction)));
      ui_->labelAddress->setText(item->mainAddress);

      if (item->confirmations > 0) {
         ui_->labelHeight->setText(QString::number(item->txEntry.blockNum));
      }
      else {
         if (item->txEntry.isRBF) {
            ui_->labelFlag->setText(tr("RBF eligible"));
         } else if (item->isCPFP) {
            ui_->labelFlag->setText(tr("CPFP eligible"));
         }
      }

      if (item->tx.isInitialized()) {
         ui_->labelSize->setText(QString::number(item->tx.getTxWeight()));

         std::set<BinaryData> txHashSet;
         std::map<BinaryData, std::set<uint32_t>> txOutIndices;

         for (size_t i = 0; i < item->tx.getNumTxIn(); ++i) {
            TxIn in = item->tx.getTxInCopy(i);
            OutPoint op = in.getOutPoint();
            txHashSet.insert(op.getTxHash());
            txOutIndices[op.getTxHash()].insert(op.getTxOutIndex());
         }

         auto cbTXs = [this, item, txOutIndices, handle]
            (const AsyncClient::TxBatchResult &txs, std::exception_ptr exPtr) mutable
         {
            ValidityGuard guard(handle);
            if (!handle.isValid()) {
               return;
            }

            if (exPtr != nullptr) {
               ui_->labelComment->setText(tr("Failed to get TX details"));
            }

            ui_->treeAddresses->addTopLevelItem(itemSender_);
            ui_->treeAddresses->addTopLevelItem(itemReceiver_);

            for (const auto &wallet : item->wallets) {
               if (wallet->type() == bs::core::wallet::Type::ColorCoin) {
                  ccLeaf_ = std::dynamic_pointer_cast<bs::sync::hd::CCLeaf>(wallet);
                  break;
               }
            }

            if (!ccLeaf_) {
               for (size_t i = 0; i < item->tx.getNumTxOut(); ++i) {
                  const TxOut out = item->tx.getTxOutCopy(i);
                  const auto txType = out.getScriptType();
                  if (txType == TXOUT_SCRIPT_OPRETURN || txType == TXOUT_SCRIPT_NONSTANDARD) {
                     continue;
                  }
                  const auto addr = bs::Address::fromTxOut(out);
                  const auto addressWallet = walletsManager_->getWalletByAddress(addr);
                  if (addressWallet && (addressWallet->type() == bs::core::wallet::Type::ColorCoin)) {
                     ccLeaf_ = std::dynamic_pointer_cast<bs::sync::hd::CCLeaf>(addressWallet);
                     break;
                  }
               }
            }
            if (ccLeaf_) {
               ui_->labelFlag->setText(tr("CC: %1").arg(ccLeaf_->displaySymbol()));
            }

            uint64_t value = 0;
            bool initialized = true;

            std::set<bs::sync::WalletsManager::WalletPtr> inputWallets;

            const bool isInternalTx = item->direction == bs::sync::Transaction::Internal;
            for (const auto &prevTx : txs) {
               if (!prevTx.second || !prevTx.second->isInitialized()) {
                  continue;
               }
               const auto &itTxOut = txOutIndices.find(prevTx.first);
               if (itTxOut == txOutIndices.end()) {
                  continue;
               }
               for (const auto &txOutIdx : itTxOut->second) {
                  TxOut prevOut = prevTx.second->getTxOutCopy(txOutIdx);
                  value += prevOut.getValue();
                  const auto addr = bs::Address::fromTxOut(prevOut);
                  const auto addressWallet = walletsManager_->getWalletByAddress(addr);
                  if (addressWallet) {
                     const auto &rootWallet = walletsManager_->getHDRootForLeaf(addressWallet->walletId());
                     if (rootWallet) {
                        const auto &xbtLeaves = rootWallet->getGroup(rootWallet->getXBTGroupType())->getLeaves();
                        bool isXbtLeaf = false;
                        for (const auto &leaf : xbtLeaves) {
                           if (*leaf == *addressWallet) {
                              isXbtLeaf = true;
                              break;
                           }
                        }
                        if (isXbtLeaf) {
                           inputWallets.insert(xbtLeaves.cbegin(), xbtLeaves.cend());
                        }
                        else {
                           inputWallets.insert(addressWallet);
                        }
                     }
                     else {
                        inputWallets.insert(addressWallet);
                     }
                     if (!ccLeaf_ && (addressWallet->type() == bs::core::wallet::Type::ColorCoin)) {
                        ccLeaf_ = std::dynamic_pointer_cast<bs::sync::hd::CCLeaf>(addressWallet);
                     }
                  }
                  addAddress(prevOut, false, prevTx.first, {});
               }
            }

            std::vector<TxOut> allOutputs;
            for (size_t i = 0; i < item->tx.getNumTxOut(); ++i) {
               const TxOut out = item->tx.getTxOutCopy(i);
               value -= out.getValue();
               allOutputs.push_back(out);
            }

            for (size_t i = 0; i < item->tx.getNumTxOut(); ++i) {
               const TxOut out = item->tx.getTxOutCopy(i);
               addAddress(out, true, item->tx.getThisHash(), inputWallets
                  , allOutputs);
            }

            if (!item->wallets.empty()) {
               std::string comment;
               for (const auto &wallet : item->wallets) {
                  comment = wallet->getTransactionComment(item->tx.getThisHash());
                  if (!comment.empty()) {
                     break;
                  }
               }
               ui_->labelComment->setText(QString::fromStdString(comment));
            }

            if (initialized) {
               ui_->labelFee->setText(UiUtils::displayAmount(value));
               ui_->labelSb->setText(
                  QString::number((float)value / (float)item->tx.getTxWeight()));
            }

            ui_->treeAddresses->expandItem(itemSender_);
            ui_->treeAddresses->expandItem(itemReceiver_);

            for (int i = 0; i < ui_->treeAddresses->columnCount(); ++i) {
               ui_->treeAddresses->resizeColumnToContents(i);
               ui_->treeAddresses->setColumnWidth(i,
                  ui_->treeAddresses->columnWidth(i) + extraTreeWidgetColumnMargin);
            }
            adjustSize();
         };
         if (txHashSet.empty()) {
            cbTXs({}, nullptr);
         }
         else {
            armory->getTXsByHash(txHashSet, cbTXs, true);
         }
      }

      ui_->labelConfirmations->setText(QString::number(item->confirmations));
   };
   TransactionsViewItem::initialize(tvi, armory.get(), walletsManager, cbInit);

   bool bigEndianHash = true;
   ui_->labelHash->setText(QString::fromStdString(tvi->txEntry.txHash.toHexStr(bigEndianHash)));
   ui_->labelTime->setText(UiUtils::displayDateTime(QDateTime::fromTime_t(tvi->txEntry.txTime)));

   ui_->labelWalletName->setText(tvi->walletName.isEmpty() ? tr("Unknown") : tvi->walletName);

   /* disabled the context menu for copy to clipboard functionality, it can be removed later
   ui_->treeAddresses->setContextMenuPolicy(Qt::CustomContextMenu);
   connect(ui_->treeAddresses, &QTreeView::customContextMenuRequested, [=](const QPoint& p) {
      const auto address = ui_->treeAddresses->itemAt(p)->data(0, Qt::UserRole).toString();

      if (!address.isEmpty()) {
         QMenu* menu = new QMenu(this);
         QAction* copyAction = menu->addAction(tr("&Copy Address"));
         connect(copyAction, &QAction::triggered, [=]() {
            qApp->clipboard()->setText(address);
         });
         menu->popup(ui_->treeAddresses->mapToGlobal(p));
      }
   });*/
   // allow address column to be copied to clipboard with right click
   ui_->treeAddresses->copyToClipboardColumns_.append(2);

   setMinimumHeight(minHeightAtRendering);
   resize(minimumSize());
}

TransactionDetailDialog::~TransactionDetailDialog() = default;

QSize TransactionDetailDialog::minimumSize() const
{
   int minWidth = 2 * extraTreeWidgetColumnMargin;

   for(int i = 0; i < ui_->treeAddresses->columnCount(); ++i) {
      minWidth += ui_->treeAddresses->columnWidth(i) + extraTreeWidgetColumnMargin;
   }

   return QSize(minWidth, minimumHeight());
}

QSize TransactionDetailDialog::minimumSizeHint() const
{
   return minimumSize();
}

// Add an address to the dialog.
// IN:  The TxOut to check the address against. (const TxOut&)
//      Indicator for whether the TxOut is sourced against output. (bool)
//      Indicator for whether the Tx type is outgoing. (bool)
//      The TX hash. (const BinaryData&)
// OUT: None
// RET: None
void TransactionDetailDialog::addAddress(TxOut out    // can't use const ref due to TxOut::getIndex()
   , bool isOutput, const BinaryData& txHash
   , const WalletsSet &inputWallets, const std::vector<TxOut> &allOutputs)
{
   QString addressType;
   QString displayedAddress;
   bs::sync::WalletsManager::WalletPtr addressWallet;
   bool isChange = false;
   try {
      const auto &addr = bs::Address::fromTxOut(out);
      addressWallet = walletsManager_->getWalletByAddress(addr);

      TxOut lastChange;
      const auto &setLastChange = [&lastChange, inputWallets, this]
         (TxOut out)
      {
         const auto &addr = bs::Address::fromTxOut(out);
         const auto &addrWallet = walletsManager_->getWalletByAddress(addr);
         if (inputWallets.find(addrWallet) != inputWallets.end()) {
            lastChange = out;
         }
      };

      if (isOutput) {
         if (!allOutputs.empty()) {
            for (auto output : allOutputs) {
               setLastChange(output);
            }
         } else {
            setLastChange(out);
         }

         if (lastChange.isInitialized()) {
            if (out.getScript() == lastChange.getScript()) {
               isChange = true;
            }
         }
      }

      addressType = isChange ? tr("Change") : (isOutput ? tr("Output") : tr("Input"));
      displayedAddress = QString::fromStdString(addr.display());
   }
   catch (const std::exception &) {    // Likely OP_RETURN
      addressType = tr("Unknown");
      displayedAddress = tr("empty");
   }

   // Inputs should be negative, outputs positive, and change positive
   QString valueStr = isOutput ? QString() : QLatin1String("-");
   QString walletName;

   const auto parent = (!isOutput || isChange) ? itemSender_ : itemReceiver_;

   if (addressWallet) {
      valueStr += addressWallet->displayTxValue(int64_t(out.getValue()));
      walletName = QString::fromStdString(addressWallet->name());
   }
   else {
      bool isCCaddress = false;
      if (ccLeaf_) {
         if (walletsManager_->isValidCCOutpoint(ccLeaf_->shortName(), txHash, out.getIndex(), out.getValue())) {
            isCCaddress = true;
         }
      }
      if (isCCaddress) {
         valueStr += ccLeaf_->displayTxValue(int64_t(out.getValue()));
         walletName = QString::fromStdString(ccLeaf_->shortName());
      }
      else {
         valueStr += UiUtils::displayAmount(out.getValue());
      }
   }
   QStringList items;
   items << addressType;
   items << valueStr;
   if (walletName.isEmpty()) {
      items << displayedAddress;
   } else {
      items << displayedAddress << walletName;
   }

   auto item = new QTreeWidgetItem(items);
   item->setData(0, Qt::UserRole, displayedAddress);
   item->setData(1, Qt::UserRole, (qulonglong)out.getValue());
   parent->addChild(item);
   const auto txHashStr = QString::fromStdString(txHash.toHexStr(true));
   auto txItem = new QTreeWidgetItem(QStringList() << getScriptType(out)
                                     << QString::number(out.getValue())
                                     << txHashStr);
   txItem->setData(0, Qt::UserRole, txHashStr);
   item->addChild(txItem);
}

QString TransactionDetailDialog::getScriptType(const TxOut &out)
{
   switch (out.getScriptType()) {
      case TXOUT_SCRIPT_STDHASH160:    return tr("hash160");
      case TXOUT_SCRIPT_STDPUBKEY65:   return tr("pubkey65");
      case TXOUT_SCRIPT_STDPUBKEY33:   return tr("pubkey33");
      case TXOUT_SCRIPT_MULTISIG:      return tr("multisig");
      case TXOUT_SCRIPT_P2SH:          return tr("p2sh");
      case TXOUT_SCRIPT_NONSTANDARD:   return tr("non-std");
      case TXOUT_SCRIPT_P2WPKH:        return tr("p2wpkh");
      case TXOUT_SCRIPT_P2WSH:         return tr("p2wsh");
      case TXOUT_SCRIPT_OPRETURN:      return tr("op-return");
   }
   return tr("unknown");
}
