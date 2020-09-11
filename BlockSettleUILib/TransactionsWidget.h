/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef __TRANSACTIONS_WIDGET_UI_H__
#define __TRANSACTIONS_WIDGET_UI_H__

#include <memory>
#include <set>
#include <QMenu>
#include <QWidget>
#include "BinaryData.h"
#include "BSErrorCode.h"
#include "TransactionsWidgetInterface.h"

namespace spdlog {
   class logger;
}
namespace Ui {
   class TransactionsWidget;
}
namespace bs {
   namespace sync {
      class WalletsManager;
   }
   class UTXOReservationManager;
}
class ApplicationSettings;
class ArmoryConnection;
class TransactionsProxy;
class TransactionsViewModel;
class TransactionsSortFilterModel;
class WalletSignerContainer;


class TransactionsWidget : public TransactionsWidgetInterface
{
Q_OBJECT

public:
   TransactionsWidget(QWidget* parent = nullptr );
   ~TransactionsWidget() override;

   [[deprecated]] void init(const std::shared_ptr<bs::sync::WalletsManager> &
             , const std::shared_ptr<ArmoryConnection> &
             , const std::shared_ptr<bs::UTXOReservationManager> &
             , const std::shared_ptr<WalletSignerContainer> &
             , const std::shared_ptr<ApplicationSettings>&
             , const std::shared_ptr<spdlog::logger> &);
   void init(const std::shared_ptr<spdlog::logger> &
      , const std::shared_ptr<TransactionsViewModel> &);
   [[deprecated]] void SetTransactionsModel(const std::shared_ptr<TransactionsViewModel> &);

   void shortcutActivated(ShortcutType s) override;

private slots:
   void showTransactionDetails(const QModelIndex& index);
   void updateResultCount();
   void walletsChanged();
   void walletsFilterChanged(int index);
   void onEnterKeyInTrxPressed(const QModelIndex &index);
   void onDataLoaded(int count);
   void onProgressInited(int start, int end);
   void onProgressUpdated(int value);

private:
   void scheduleDateFilterCheck();
   std::unique_ptr<Ui::TransactionsWidget> ui_;

   TransactionsSortFilterModel         *  sortFilterModel_;
   
};


#endif // __TRANSACTIONS_WIDGET_UI_H__
