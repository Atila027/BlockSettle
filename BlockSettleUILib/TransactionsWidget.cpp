/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "ui_TransactionsWidget.h"
#include "TransactionsWidget.h"

#include <QSortFilterProxyModel>
#include <QMenu>
#include <QClipboard>
#include <QDateTime>
#include <spdlog/spdlog.h>
#include "ApplicationSettings.h"
#include "BSMessageBox.h"
#include "CreateTransactionDialogAdvanced.h"
#include "PasswordDialogDataWrapper.h"
#include "TradesUtils.h"
#include "TransactionsViewModel.h"
#include "TransactionDetailDialog.h"
#include "Wallets/SyncHDWallet.h"
#include "Wallets/SyncWalletsManager.h"
#include "UiUtils.h"
#include "UtxoReservationManager.h"

static const QString c_allWalletsId = QLatin1String("all");
using namespace bs::sync;


class TransactionsSortFilterModel : public QSortFilterProxyModel
{
public:
   TransactionsSortFilterModel(std::shared_ptr<ApplicationSettings> &appSettings, QObject* parent)
      : QSortFilterProxyModel(parent)
      , appSettings_(appSettings)
   {
      setSortRole(TransactionsViewModel::SortRole);
   }

/*   int rowCount(const QModelIndex & parent = QModelIndex()) const override
   {     //! causes assert(last < rowCount()) to invoke when filtering by wallet
      return qMin(QSortFilterProxyModel::rowCount(parent), 500);
   }*/

   int totalRowCount() const
   {
      return QSortFilterProxyModel::rowCount();
   }

   bool filterAcceptsRow(int source_row, const QModelIndex &) const override
   {
      auto src = sourceModel();
      if (!src) {
         return false;
      }
      QModelIndex directionIndex = src->index(source_row, static_cast<int>(TransactionsViewModel::Columns::SendReceive));
      int direction = src->data(directionIndex, TransactionsViewModel::FilterRole).toInt();

      bool walletMatched = false;
      if (!walletIds.isEmpty()) {
         const QModelIndex index = src->index(source_row,
            static_cast<int>(TransactionsViewModel::Columns::Wallet));
         for (const auto &walletId : walletIds) {
            if (src->data(index, TransactionsViewModel::FilterRole).toString() == walletId) {
               walletMatched = true;
            }
         }
         if (!walletMatched) {
            return false;
         }
      }

      if (transactionDirection != bs::sync::Transaction::Unknown) {
         const auto aIdx = src->index(source_row,
            static_cast<int>(TransactionsViewModel::Columns::Amount));
         const auto wallet = static_cast<bs::sync::Wallet*>(aIdx.data(
            TransactionsViewModel::WalletRole).value<void*>());

         if (!walletIds.isEmpty() && wallet->type() == bs::core::wallet::Type::ColorCoin) {
            const auto a = aIdx.data(Qt::DisplayRole).toDouble();

            switch (transactionDirection) {
            case bs::sync::Transaction::Received : {
               if (a < 0.0) {
                  return false;
               }
            }
               break;

            case bs::sync::Transaction::Sent : {
               if (a > 0.0) {
                  return false;
               }
            }
               break;

            default :
               return false;
            }
         } else if (direction != transactionDirection) {
            return false;
         }
      }

      bool result = true;

      if ((startDate > 0) && (endDate > 0)) {
         QModelIndex index = src->index(source_row, static_cast<int>(TransactionsViewModel::Columns::Date));
         uint32_t txDate = src->data(index, TransactionsViewModel::FilterRole).toUInt();
         result = (startDate <= txDate) && (txDate < endDate);
      }

      if (result && !searchString.isEmpty()) {     // more columns can be added later
         for (const auto &col : { TransactionsViewModel::Columns::Comment, TransactionsViewModel::Columns::Address }) {
            QModelIndex index = src->index(source_row, static_cast<int>(col));
            if (src->data(index, TransactionsViewModel::FilterRole).toString().contains(searchString, Qt::CaseInsensitive)) {
               return true;
            }
         }
         return false;
      }

      return result;
   }

   bool filterAcceptsColumn(int source_column, const QModelIndex &source_parent) const override
   {
      Q_UNUSED(source_parent);
/*      const auto col = static_cast<TransactionsViewModel::Columns>(source_column);
      return (col != TransactionsViewModel::Columns::MissedBlocks);*/
      return true;   // strange, but it works properly only this way
   }

   bool lessThan(const QModelIndex &left, const QModelIndex &right) const override
   {
      if (left.column() == static_cast<int>(TransactionsViewModel::Columns::Status)) {
         QVariant leftData = sourceModel()->data(left, TransactionsViewModel::SortRole);
         QVariant rightData = sourceModel()->data(right, TransactionsViewModel::SortRole);

         if (leftData == rightData) {
            // if sorting by confirmations, and values are equal, perform sorting by date in descending order
            const QModelIndex leftDateIdx = sourceModel()->index(left.row(), static_cast<int>(TransactionsViewModel::Columns::Date));
            const QModelIndex rightDateIdx = sourceModel()->index(right.row(), static_cast<int>(TransactionsViewModel::Columns::Date));
            const auto lDate = sourceModel()->data(leftDateIdx, TransactionsViewModel::SortRole);
            const auto rDate = sourceModel()->data(rightDateIdx, TransactionsViewModel::SortRole);

            return lDate > rDate;
         }
      }

      return QSortFilterProxyModel::lessThan(left, right);
   }

   void updateFilters(const QStringList &walletIds, const QString &searchString, bs::sync::Transaction::Direction direction)
   {
      this->walletIds = walletIds;
      this->searchString = searchString;
      this->transactionDirection = direction;

      appSettings_->set(ApplicationSettings::TransactionFilter,
         QVariantList() << (this->walletIds.isEmpty() ?
            QStringList() << c_allWalletsId : this->walletIds) <<
         static_cast<int>(direction));

      invalidateFilter();
   }

   void updateDates(const QDate& start, const QDate& end)
   {
      this->startDate = start.isValid() ? QDateTime(start, QTime(), Qt::LocalTime).toTime_t() : 0;
      this->endDate = end.isValid() ? QDateTime(end, QTime(), Qt::LocalTime).addDays(1).toTime_t() : 0;
      invalidateFilter();
   }

   std::shared_ptr<ApplicationSettings> appSettings_;
   QStringList walletIds;
   QString searchString;
   bs::sync::Transaction::Direction transactionDirection = bs::sync::Transaction::Unknown;
   uint32_t startDate = 0;
   uint32_t endDate = 0;
};


TransactionsWidget::TransactionsWidget(QWidget* parent)
   : TransactionsWidgetInterface(parent)
   , ui_(new Ui::TransactionsWidget())
   , sortFilterModel_(nullptr)
{
   ui_->setupUi(this);
   connect(ui_->treeViewTransactions, &QAbstractItemView::doubleClicked, this, &TransactionsWidget::showTransactionDetails);
   ui_->treeViewTransactions->setContextMenuPolicy(Qt::CustomContextMenu);

   connect(ui_->treeViewTransactions, &QAbstractItemView::customContextMenuRequested, [=](const QPoint& p) {
      auto index = sortFilterModel_->mapToSource(ui_->treeViewTransactions->indexAt(p));
      auto addressIndex = model_->index(index.row(), static_cast<int>(TransactionsViewModel::Columns::Address));
      curAddress_ = model_->data(addressIndex).toString();

      contextMenu_.clear();

      if (sortFilterModel_) {
         const auto &sourceIndex = sortFilterModel_->mapToSource(ui_->treeViewTransactions->indexAt(p));
         const auto &txNode = model_->getNode(sourceIndex);
         if (txNode && txNode->item() && txNode->item()->initialized) {
            if (txNode->item()->isRBFeligible() && (txNode->level() < 2)) {
               contextMenu_.addAction(actionRBF_);
               actionRBF_->setData(sourceIndex);
            }
            else {
               actionRBF_->setData(-1);
            }

            if (txNode->item()->isCPFPeligible()) {
               contextMenu_.addAction(actionCPFP_);
               actionCPFP_->setData(sourceIndex);
            }
            else {
               actionCPFP_->setData(-1);
            }

            if (txNode->item()->isPayin()) {
               contextMenu_.addAction(actionRevoke_);
               actionRevoke_->setData(sourceIndex);
               actionRevoke_->setEnabled(model_->isTxRevocable(txNode->item()->tx));
            }
            else {
               actionRevoke_->setData(-1);
            }

            // save transaction id and add context menu for copying it to clipboard
            curTx_ = QString::fromStdString(txNode->item()->txEntry.txHash.toHexStr(true));
            contextMenu_.addAction(actionCopyTx_);

            // allow copy address only if there is only 1 address
            if (txNode->item()->addressCount == 1) {
               contextMenu_.addAction(actionCopyAddr_);
            }
         }
      }
      contextMenu_.popup(ui_->treeViewTransactions->mapToGlobal(p));
   });
   ui_->treeViewTransactions->setUniformRowHeights(true);
   ui_->treeViewTransactions->setItemsExpandable(true);
   ui_->treeViewTransactions->setRootIsDecorated(true);
   ui_->treeViewTransactions->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

   connect(ui_->typeFilterComboBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), [&](int index) {
      sortFilterModel_->updateFilters(sortFilterModel_->walletIds, sortFilterModel_->searchString
         , static_cast<bs::sync::Transaction::Direction>(index));
   });

   connect(ui_->walletBox, static_cast<void (QComboBox::*)(int)>(&QComboBox::currentIndexChanged), this, &TransactionsWidget::walletsFilterChanged);

   ui_->dateEditEnd->setDate(QDate::currentDate());

   connect(ui_->dateEditEnd, &QDateTimeEdit::dateTimeChanged, [=](const QDateTime& dateTime) {
      if (ui_->dateEditStart->dateTime() > dateTime) {
         ui_->dateEditStart->setDate(dateTime.date());
      }
   });
   connect(ui_->dateEditStart, &QDateTimeEdit::dateTimeChanged, [=](const QDateTime& dateTime) {
      if (ui_->dateEditEnd->dateTime() < dateTime) {
         ui_->dateEditEnd->setDate(dateTime.date());
      }
   });

   connect(ui_->treeViewTransactions, &TreeViewWithEnterKey::enterKeyPressed,
          this, &TransactionsWidget::onEnterKeyInTrxPressed);

   ui_->labelResultCount->hide();
   ui_->progressBar->hide();
}

TransactionsWidget::~TransactionsWidget() = default;

void TransactionsWidget::init(const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
                              , const std::shared_ptr<ArmoryConnection> &armory
                              , const std::shared_ptr<bs::UTXOReservationManager> &utxoReservationManager
                              , const std::shared_ptr<WalletSignerContainer> &signContainer
                              , const std::shared_ptr<ApplicationSettings> &appSettings
                              , const std::shared_ptr<spdlog::logger> &logger)

{
   TransactionsWidgetInterface::init(walletsMgr, armory, utxoReservationManager, signContainer, appSettings, logger);

   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletChanged, this, &TransactionsWidget::walletsChanged);
   connect(walletsManager_.get(), &bs::sync::WalletsManager::walletDeleted, [this](std::string) { walletsChanged(); });

   scheduleDateFilterCheck();
}

void TransactionsWidget::SetTransactionsModel(const std::shared_ptr<TransactionsViewModel>& model)
{
   model_ = model;
   connect(model_.get(), &TransactionsViewModel::dataLoaded, this, &TransactionsWidget::onDataLoaded, Qt::QueuedConnection);
   connect(model_.get(), &TransactionsViewModel::initProgress, this, &TransactionsWidget::onProgressInited);
   connect(model_.get(), &TransactionsViewModel::updateProgress, this, &TransactionsWidget::onProgressUpdated);

   sortFilterModel_ = new TransactionsSortFilterModel(appSettings_, this);
   sortFilterModel_->setSourceModel(model.get());
   sortFilterModel_->setDynamicSortFilter(true);


   connect(sortFilterModel_, &TransactionsSortFilterModel::rowsInserted, this, &TransactionsWidget::updateResultCount);
   connect(sortFilterModel_, &TransactionsSortFilterModel::rowsRemoved, this, &TransactionsWidget::updateResultCount);
   connect(sortFilterModel_, &TransactionsSortFilterModel::modelReset, this, &TransactionsWidget::updateResultCount);

   walletsChanged();

   auto updateDateTimes = [this]() {
      sortFilterModel_->updateDates(ui_->dateEditStart->date(), ui_->dateEditEnd->date());
   };
   connect(ui_->dateEditStart, &QDateTimeEdit::dateTimeChanged, updateDateTimes);
   connect(ui_->dateEditEnd, &QDateTimeEdit::dateTimeChanged, updateDateTimes);

   connect(ui_->searchField, &QLineEdit::textChanged, [=](const QString& text) {
      sortFilterModel_->updateFilters(sortFilterModel_->walletIds, text, sortFilterModel_->transactionDirection);
   });

   ui_->treeViewTransactions->setSortingEnabled(true);
   ui_->treeViewTransactions->setModel(sortFilterModel_);
   ui_->treeViewTransactions->hideColumn(static_cast<int>(TransactionsViewModel::Columns::TxHash));

   ui_->treeViewTransactions->sortByColumn(static_cast<int>(TransactionsViewModel::Columns::Date), Qt::DescendingOrder);
   ui_->treeViewTransactions->sortByColumn(static_cast<int>(TransactionsViewModel::Columns::Status), Qt::AscendingOrder);

//   ui_->treeViewTransactions->hideColumn(static_cast<int>(TransactionsViewModel::Columns::MissedBlocks));
}

void TransactionsWidget::onDataLoaded(int count)
{
   ui_->progressBar->hide();
   ui_->progressBar->setMaximum(0);
   ui_->progressBar->setMinimum(0);

   if ((count <= 0) || (ui_->dateEditStart->dateTime().date().year() > 2009)) {
      return;
   }
   const auto &item = model_->getOldestItem();
   if (item) {
      ui_->dateEditStart->setDateTime(QDateTime::fromTime_t(item->txEntry.txTime));
   }
}

void TransactionsWidget::onProgressInited(int start, int end)
{
   ui_->progressBar->show();
   ui_->progressBar->setMinimum(start);
   ui_->progressBar->setMaximum(end);
}

void TransactionsWidget::onProgressUpdated(int value)
{
   ui_->progressBar->setValue(value);
}

void TransactionsWidget::shortcutActivated(ShortcutType s)
{
   if (s == ShortcutType::Alt_1)
      ui_->treeViewTransactions->activate();
}

static inline QStringList walletLeavesIds(bs::sync::WalletsManager::HDWalletPtr wallet)
{
   QStringList allLeafIds;

   for (const auto &leaf : wallet->getLeaves()) {
      const QString id = QString::fromStdString(leaf->walletId());
      allLeafIds << id;
   }

   return allLeafIds;
}

static inline bool exactlyThisLeaf(const QStringList &ids, const QStringList &walletIds)
{
   if (ids.size() != walletIds.size()) {
      return false;
   }

   int count = 0;

   count = std::accumulate(ids.cbegin(), ids.cend(), count,
      [&](int value, const QString &id) {
         if (walletIds.contains(id)) {
            return ++value;
         } else {
            return value;
         }
   });

   return (count == walletIds.size());
}

void TransactionsWidget::walletsChanged()
{
   QStringList walletIds;
   int direction;

   const auto varList = appSettings_->get(ApplicationSettings::TransactionFilter).toList();
   walletIds = varList.first().toStringList();
   direction = varList.last().toInt();

   int currentIndex = -1;
   int primaryWalletIndex = 0;

   ui_->walletBox->clear();
   ui_->walletBox->addItem(tr("All Wallets"));
   int index = 1;
   for (const auto &hdWallet : walletsManager_->hdWallets()) {
      ui_->walletBox->addItem(QString::fromStdString(hdWallet->name()));
      QStringList allLeafIds = walletLeavesIds(hdWallet);

      if (exactlyThisLeaf(walletIds, allLeafIds)) {
         currentIndex = index;
      }

      if (hdWallet == walletsManager_->getPrimaryWallet()) {
         primaryWalletIndex = index;
      }

      ui_->walletBox->setItemData(index++, allLeafIds, UiUtils::WalletIdRole);

      for (const auto &group : hdWallet->getGroups()) {
         if (group->type() == bs::core::wallet::Type::Settlement) {
            continue;
         }
         ui_->walletBox->addItem(QString::fromStdString("   " + group->name()));
         const auto groupIndex = index++;
         QStringList groupLeafIds;
         for (const auto &leaf : group->getLeaves()) {
            groupLeafIds << QString::fromStdString(leaf->walletId());
            ui_->walletBox->addItem(QString::fromStdString("      " + leaf->shortName()));

            const auto id = QString::fromStdString(leaf->walletId());
            QStringList ids;
            ids << id;

            ui_->walletBox->setItemData(index, ids, UiUtils::WalletIdRole);

            if (exactlyThisLeaf(walletIds, ids)) {
               currentIndex = index;
            }

            index++;
         }
         if (groupLeafIds.isEmpty()) {
            groupLeafIds << QLatin1String("non-existent");
         }
         ui_->walletBox->setItemData(groupIndex, groupLeafIds, UiUtils::WalletIdRole);
      }
   }

   ui_->typeFilterComboBox->setCurrentIndex(direction);

   if (currentIndex >= 0) {
      ui_->walletBox->setCurrentIndex(currentIndex);
   } else {
      if (walletIds.contains(c_allWalletsId)) {
         ui_->walletBox->setCurrentIndex(0);
      } else {
         const auto primaryWallet = walletsManager_->getPrimaryWallet();

         if (primaryWallet) {
            ui_->walletBox->setCurrentIndex(primaryWalletIndex);
         } else {
            ui_->walletBox->setCurrentIndex(0);
         }
      }
   }
}

void TransactionsWidget::walletsFilterChanged(int index)
{
   if (index < 0) {
      return;
   }
   const auto &walletIds = ui_->walletBox->itemData(index, UiUtils::WalletIdRole).toStringList();
   sortFilterModel_->updateFilters(walletIds, sortFilterModel_->searchString, sortFilterModel_->transactionDirection);
}

void TransactionsWidget::onEnterKeyInTrxPressed(const QModelIndex &index)
{
   showTransactionDetails(index);
}

void TransactionsWidget::showTransactionDetails(const QModelIndex& index)
{
   const auto &txItem = model_->getItem(sortFilterModel_->mapToSource(index));
   if (!txItem) {
      SPDLOG_LOGGER_ERROR(logger_, "item not found");
      return;
   }

   TransactionDetailDialog transactionDetailDialog(txItem, walletsManager_, armory_, this);
   transactionDetailDialog.exec();
}

void TransactionsWidget::updateResultCount()
{
   auto shown = sortFilterModel_->rowCount();
   auto total = model_->itemsCount();
   ui_->labelResultCount->setText(tr("Displaying %L1 transactions (of %L2 total).")
      .arg(shown).arg(total));
   ui_->labelResultCount->show();
}

void TransactionsWidget::scheduleDateFilterCheck()
{
   const auto delay = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::hours(24)
      - std::chrono::milliseconds(QDateTime::currentDateTime().time().msecsSinceStartOfDay())
      + std::chrono::seconds(10));

   // Update end date at midnight filter if it was not changed manually
   QTimer::singleShot(delay, this, [this] {
      auto currentDate = QDate::currentDate();
      if (ui_->dateEditEnd->date().daysTo(currentDate) == 1) {
         ui_->dateEditEnd->setDate(currentDate);
      }
      scheduleDateFilterCheck();
   });
}
