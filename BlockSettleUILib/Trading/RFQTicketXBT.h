/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef __RFQ_TICKET_XBT_H__
#define __RFQ_TICKET_XBT_H__

#include <QFont>
#include <QWidget>

#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>

#include "BSErrorCode.h"
#include "CommonTypes.h"
#include "UtxoReservationToken.h"
#include "XBTAmount.h"
#include "UiUtils.h"

QT_BEGIN_NAMESPACE
class QPushButton;
class QLineEdit;
QT_END_NAMESPACE

namespace spdlog {
   class logger;
}
namespace Ui {
    class RFQTicketXBT;
}
namespace bs {
   namespace sync {
      namespace hd {
         class Leaf;
         class Wallet;
      }
      class Wallet;
      class WalletsManager;
   }
   class UTXOReservationManager;
}
class ArmoryConnection;
class AssetManager;
class AuthAddressManager;
class CCAmountValidator;
class FXAmountValidator;
class QuoteProvider;
class SelectedTransactionInputs;
class SignContainer;
class XbtAmountValidator;


class RFQTicketXBT : public QWidget
{
Q_OBJECT

public:
   RFQTicketXBT(QWidget* parent = nullptr);
   ~RFQTicketXBT() override;

   void init(const std::shared_ptr<spdlog::logger> &logger
      , const std::shared_ptr<AuthAddressManager> &
      , const std::shared_ptr<AssetManager> &assetManager
      , const std::shared_ptr<QuoteProvider> &quoteProvider
      , const std::shared_ptr<SignContainer> &
      , const std::shared_ptr<ArmoryConnection> &
      , const std::shared_ptr<bs::UTXOReservationManager> &);
   void setWalletsManager(const std::shared_ptr<bs::sync::WalletsManager> &);

   void resetTicket();

   bs::FixedXbtInputs fixedXbtInputs();

   QPushButton* submitButton() const;
   QLineEdit* lineEditAmount() const;
   QPushButton* buyButton() const;
   QPushButton* sellButton() const;
   QPushButton* numCcyButton() const;
   QPushButton* denomCcyButton() const;

   bs::Address selectedAuthAddress() const;
   // returns empty address if automatic selected
   bs::Address recvXbtAddressIfSet() const;

   using SubmitRFQCb = std::function<void(const std::string &id
      , const bs::network::RFQ& rfq, bs::UtxoReservationToken ccUtxoRes)>;
   void setSubmitRFQ(SubmitRFQCb);
   using CancelRFQCb = std::function<void(const std::string &id)>;
   void setCancelRFQ(CancelRFQCb);

   std::shared_ptr<bs::sync::hd::Wallet> xbtWallet() const;
   UiUtils::WalletsTypes xbtWalletType() const;

   void onParentAboutToHide();

public slots:
   void SetProductAndSide(const QString& productGroup, const QString& currencyPair
      , const QString& bidPrice, const QString& offerPrice, bs::network::Side::Type side);
   void setSecurityId(const QString& productGroup, const QString& currencyPair
      , const QString& bidPrice, const QString& offerPrice);
   void setSecurityBuy(const QString& productGroup, const QString& currencyPair
      , const QString& bidPrice, const QString& offerPrice);
   void setSecuritySell(const QString& productGroup, const QString& currencyPair
      , const QString& bidPrice, const QString& offerPrice);

   void enablePanel();
   void disablePanel();

   void onSendRFQ(const std::string &id, const QString &symbol, double amount, bool buy);
   void onCancelRFQ(const std::string &id);

   void onMDUpdate(bs::network::Asset::Type, const QString &security, bs::network::MDFields);

private slots:
   void updateBalances();
   void onSignerReady();
   void walletsLoaded();

   void onNumCcySelected();
   void onDenomCcySelected();

   void onSellSelected();
   void onBuySelected();

   void showCoinControl();
   void walletSelectedRecv(int index);
   void walletSelectedSend(int index);

   void updateSubmitButton();
   void submitButtonClicked();

   void onHDLeafCreated(const std::string& ccName);
   void onCreateHDWalletError(const std::string& ccName, bs::error::ErrorCode result);

   void onMaxClicked();
   void onAmountEdited(const QString &);

   void onCreateWalletClicked();

   void onAuthAddrChanged(int);
   void onSettlLeavesLoaded(unsigned int);

   void onUTXOReservationChanged(const std::string& walletId);

protected:
   bool eventFilter(QObject *watched, QEvent *evt) override;

private:
   enum class ProductGroupType
   {
      GroupNotSelected,
      FXGroupType,
      XBTGroupType,
      CCGroupType
   };

   struct BalanceInfoContainer
   {
      double            amount;
      QString           product;
      ProductGroupType  productType;
   };

private:
   void showHelp(const QString& helpText);
   void clearHelp();
   void sendRFQ(const std::string &id);

   void updatePanel();

   void fillRecvAddresses();

   bool preSubmitCheck();

   BalanceInfoContainer getBalanceInfo() const;
   QString getProduct() const;
   std::shared_ptr<bs::sync::Wallet> getCCWallet(const std::string &cc) const;
   bool checkBalance(double qty) const;
   bool checkAuthAddr(double qty) const;
   bs::network::Side::Type getSelectedSide() const;
   std::string authKey() const;

   void putRFQ(const bs::network::RFQ &);
   bool existsRFQ(const bs::network::RFQ &);

   static std::string mkRFQkey(const bs::network::RFQ &);

   void SetProductGroup(const QString& productGroup);
   void SetCurrencyPair(const QString& currencyPair);

   void saveLastSideSelection(const std::string& product, const std::string& currencyPair, bs::network::Side::Type side);
   bs::network::Side::Type getLastSideSelection(const std::string& product, const std::string& currencyPair);

   void HideRFQControls();

   void initProductGroupMap();
   ProductGroupType getProductGroupType(const QString& productGroup);

   double getQuantity() const;
   double getOfferPrice() const;

   void SetCurrentIndicativePrices(const QString& bidPrice, const QString& offerPrice);
   void updateIndicativePrice();
   double getIndicativePrice() const;

   void productSelectionChanged();

   std::shared_ptr<bs::sync::hd::Wallet> getSendXbtWallet() const;
   std::shared_ptr<bs::sync::hd::Wallet> getRecvXbtWallet() const;
   bs::XBTAmount getXbtBalance() const;
   QString getProductToSpend() const;
   QString getProductToRecv() const;
   bs::XBTAmount expectedXbtAmountMin() const;
   bs::XBTAmount getXbtReservationAmountForCc(double quantity, double offerPrice) const;

   void reserveBestUtxoSetAndSubmit(const std::string &id
      , const std::shared_ptr<bs::network::RFQ>& rfq);

private:
   std::unique_ptr<Ui::RFQTicketXBT> ui_;

   std::shared_ptr<spdlog::logger>     logger_;
   std::shared_ptr<AssetManager>       assetManager_;
   std::shared_ptr<AuthAddressManager> authAddressManager_;

   std::shared_ptr<bs::sync::WalletsManager> walletsManager_;
   std::shared_ptr<SignContainer>      signingContainer_;
   std::shared_ptr<ArmoryConnection>   armory_;
   std::shared_ptr<bs::UTXOReservationManager>  utxoReservationManager_;

   mutable bs::Address authAddr_;
   mutable std::string authKey_;

   unsigned int      leafCreateReqId_ = 0;

   std::unordered_map<std::string, double>      rfqMap_;
   std::unordered_map<std::string, std::shared_ptr<bs::network::RFQ>>   pendingRFQs_;

   std::unordered_map<std::string, bs::network::Side::Type>         lastSideSelection_;

   QFont    invalidBalanceFont_;

   CCAmountValidator                            *ccAmountValidator_{};
   FXAmountValidator                            *fxAmountValidator_{};
   XbtAmountValidator                           *xbtAmountValidator_{};

   std::unordered_map<std::string, ProductGroupType> groupNameToType_;
   ProductGroupType     currentGroupType_ = ProductGroupType::GroupNotSelected;

   QString currentProduct_;
   QString contraProduct_;

   QString currentBidPrice_;
   QString currentOfferPrice_;

   SubmitRFQCb submitRFQCb_{};
   CancelRFQCb cancelRFQCb_{};

   bs::FixedXbtInputs fixedXbtInputs_;

   bool  autoRFQenabled_{ false };
   std::vector<std::string>   deferredRFQs_;

   std::unordered_map<std::string, bs::network::MDInfo>  mdInfo_;
};

#endif // __RFQ_TICKET_XBT_H__
