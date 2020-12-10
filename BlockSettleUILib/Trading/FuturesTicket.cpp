/*

***********************************************************************************
* Copyright (C) 2020 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "FuturesTicket.h"
#include "ui_FuturesTicket.h"

#include "AssetManager.h"
#include "BSMessageBox.h"
#include "CommonTypes.h"
#include "CurrencyPair.h"
#include "QuoteProvider.h"
#include "UiUtils.h"
#include "XbtAmountValidator.h"

#include <QEvent>
#include <QKeyEvent>
#include <QStyle>

#include <spdlog/spdlog.h>

namespace {
   const QString kEmptyInformationalLabelText = QStringLiteral("--");

   QString formatPrice(double price, bs::network::Asset::Type at) {
      if (price == 0) {
         return kEmptyInformationalLabelText;
      }
      return UiUtils::displayPriceForAssetType(price, at);
   }

   const auto kAmounts = std::vector<double>{1., 5., 10., 25.};

   const auto kLabelCount = kAmounts.size() + 1;

   std::string getLabel(size_t i)
   {
      if (i < kAmounts.size()) {
         return fmt::format("{:0.2f}", kAmounts.at(i));
      } else {
         return fmt::format(">{:0.2f}", kAmounts.back());
      }
   }

   size_t getSelectedLine(double amount) {
      for (size_t i = 0; i < kAmounts.size(); ++i) {
         if (amount < kAmounts[i]) {
            return i;
         }
      }
      return kAmounts.size();
   }

}


FuturesTicket::FuturesTicket(QWidget* parent)
   : QWidget(parent)
   , ui_(new Ui::FuturesTicket())
{
   ui_->setupUi(this);

   xbtAmountValidator_ = new XbtAmountValidator(this);

   ui_->lineEditAmount->installEventFilter(this);

   connect(ui_->pushButtonBuy, &QPushButton::clicked, this, [this] {
      submit(bs::network::Side::Buy);
   });
   connect(ui_->pushButtonSell, &QPushButton::clicked, this, [this] {
      submit(bs::network::Side::Sell);
   });
   connect(ui_->lineEditAmount, &QLineEdit::textEdited, this, &FuturesTicket::onAmountEdited);
   connect(ui_->pushButtonClose, &QPushButton::clicked, this, &FuturesTicket::onCloseAll);

   auto pricesLayout = new QGridLayout(ui_->widgetPrices);
   pricesLayout->setSpacing(0);
   pricesLayout->setContentsMargins(0, 0, 0, 0);

   labels_.resize(kLabelCount);
   for (size_t i = 0; i < kLabelCount; ++i) {
      auto label = new QLabel(this);
      label->setText(QString::fromStdString(getLabel(i)));
      pricesLayout->addWidget(label, int(i), 0, Qt::AlignCenter);
      for (size_t j = 0; j < 2; ++j) {
         auto label = new QLabel(this);
         label->setObjectName(QStringLiteral("labelFuturePrice"));
         pricesLayout->addWidget(label, int(i), int(j + 1), Qt::AlignCenter);
         labels_[i][j] = label;
      }
   }

   ui_->helpLabel->hide();
   ui_->toolButtonMax->hide();
}

FuturesTicket::~FuturesTicket() = default;

void FuturesTicket::resetTicket()
{
   ui_->labelProductGroup->setText(kEmptyInformationalLabelText);
   ui_->labelSecurityId->setText(kEmptyInformationalLabelText);

   ui_->lineEditAmount->setValidator(xbtAmountValidator_);
   ui_->lineEditAmount->clear();
}

void FuturesTicket::init(const std::shared_ptr<spdlog::logger> &logger
   , const std::shared_ptr<AuthAddressManager> &authAddressManager
   , const std::shared_ptr<AssetManager>& assetManager
   , const std::shared_ptr<QuoteProvider> &quoteProvider)
{
   logger_ = logger;
   authAddressManager_ = authAddressManager;
   assetManager_ = assetManager;

   connect(assetManager_.get(), &AssetManager::netDeliverableBalanceChanged, this, &FuturesTicket::updatePanel);
   connect(assetManager_.get(), &AssetManager::balanceChanged, this, &FuturesTicket::updatePanel);

   updatePanel();
}

void FuturesTicket::setType(bs::network::Asset::Type type)
{
   type_ = type;
}

void FuturesTicket::SetCurrencyPair(const QString& currencyPair)
{
   ui_->labelSecurityId->setText(currencyPair);

   CurrencyPair cp(currencyPair.toStdString());

   currentProduct_ = cp.NumCurrency() == bs::network::XbtCurrency ? cp.DenomCurrency() : cp.NumCurrency();
   security_ = currencyPair;
}

void FuturesTicket::SetProductAndSide(const QString& productGroup, const QString& currencyPair
   , const QString& bidPrice, const QString& offerPrice, bs::network::Side::Type side)
{
   resetTicket();

   if (productGroup.isEmpty() || currencyPair.isEmpty()) {
      return;
   }

   ui_->labelProductGroup->setText(productGroup);
   SetCurrencyPair(currencyPair);
   //SetCurrentIndicativePrices(bidPrice, offerPrice);

   if (side == bs::network::Side::Type::Undefined) {
      side = bs::network::Side::Buy;
   }

   productSelectionChanged();
}

void FuturesTicket::setSecurityId(const QString& productGroup, const QString& currencyPair
   , const QString& bidPrice, const QString& offerPrice)
{
   SetProductAndSide(productGroup, currencyPair, bidPrice, offerPrice, bs::network::Side::Undefined);
}

void FuturesTicket::setSecurityBuy(const QString& productGroup, const QString& currencyPair
   , const QString& bidPrice, const QString& offerPrice)
{
   SetProductAndSide(productGroup, currencyPair, bidPrice, offerPrice, bs::network::Side::Buy);
}

void FuturesTicket::setSecuritySell(const QString& productGroup, const QString& currencyPair
   , const QString& bidPrice, const QString& offerPrice)
{
   SetProductAndSide(productGroup, currencyPair, bidPrice, offerPrice, bs::network::Side::Sell);
}

std::string FuturesTicket::getProduct() const
{
   return currentProduct_;
}

double FuturesTicket::getQuantity() const
{
   const CustomDoubleValidator *validator = dynamic_cast<const CustomDoubleValidator*>(ui_->lineEditAmount->validator());
   if (validator == nullptr) {
      return 0;
   }
   return validator->GetValue(ui_->lineEditAmount->text());
}

void FuturesTicket::productSelectionChanged()
{
   const auto &mdInfo = mdInfo_[type_][security_.toStdString()];

   const auto bidPrice = formatPrice(mdInfo.bidPrice, type_);
   const auto askPrice = formatPrice(mdInfo.askPrice, type_);

   for (auto &labelPair : labels_) {
      labelPair[0]->setText(bidPrice);
      labelPair[1]->setText(askPrice);
   }

   updatePanel();
}

void FuturesTicket::updatePanel()
{
   const double balance = assetManager_ ?
      assetManager_->getBalance(currentProduct_, false, nullptr) : 0.0;
   auto amountString = UiUtils::displayCurrencyAmount(balance);
   QString text = tr("%1 %2").arg(amountString).arg(QString::fromStdString(currentProduct_));
   ui_->labelBalanceValue->setText(text);

   ui_->labelFutureBalanceValue->setText(UiUtils::displayAmount(assetManager_->netDeliverableBalanceXbt()));

   double qty = getQuantity();
   size_t selectedLine = getSelectedLine(qty);

   const auto selectedProperyName = "selectedLine";
   for (size_t lineIndex = 0; lineIndex < kLabelCount; ++lineIndex) {
      const auto &labelsLine = labels_.at(lineIndex);
      for (size_t labelIndex = 0; labelIndex < labelsLine.size(); ++labelIndex) {
         auto label = labelsLine.at(labelIndex);
         bool isSelectedOld = label->property(selectedProperyName).toBool();
         bool isSelectedNew = lineIndex == selectedLine && qty > 0;
         if (isSelectedNew != isSelectedOld) {
            label->setProperty(selectedProperyName, isSelectedNew);
            label->style()->unpolish(label);
            label->style()->polish(label);
            label->update();
         }
      }
   }
}

void FuturesTicket::onCloseAll()
{
   auto netBalance = assetManager_->netDeliverableBalanceXbt();
   auto amount = bs::XBTAmount(static_cast<uint64_t>(std::abs(netBalance)));
   auto side = netBalance < 0 ? bs::network::Side::Buy : bs::network::Side::Sell;
   sendRequest(side, amount);
}

bool FuturesTicket::eventFilter(QObject *watched, QEvent *evt)
{
   if (evt->type() == QEvent::KeyPress) {
//      auto keyID = static_cast<QKeyEvent *>(evt)->key();
//      if (ui_->pushButtonSubmit->isEnabled() && ((keyID == Qt::Key_Return) || (keyID == Qt::Key_Enter))) {
//         submitButtonClicked();
//      }
   }
   return QWidget::eventFilter(watched, evt);
}

void FuturesTicket::submit(bs::network::Side::Type side)
{
   double amount = getQuantity();
   if (amount == 0) {
      return;
   }
   sendRequest(side, bs::XBTAmount(amount));
   ui_->lineEditAmount->clear();
}

void FuturesTicket::sendRequest(bs::network::Side::Type side, bs::XBTAmount amount)
{
   if (amount.GetValue() == 0) {
      return;
   }
   // TODO: Select price depending on amount
   const auto &mdInfo = mdInfo_[type_][security_.toStdString()];
   double price = 0;
   switch (side) {
      case bs::network::Side::Buy:
         price = mdInfo.askPrice;
         break;
      case bs::network::Side::Sell:
         price = mdInfo.bidPrice;
         break;
      default:
         break;
   }
   if (price == 0) {
      return;
   }

   bs::network::FutureRequest request;
   request.side = side;
   request.price = price;
   request.amount = amount;

   emit sendFutureRequestToPB(request);
}

void FuturesTicket::onMDUpdate(bs::network::Asset::Type type, const QString &security, bs::network::MDFields mdFields)
{
   auto &mdInfo = mdInfo_[type][security.toStdString()];
   mdInfo.merge(bs::network::MDField::get(mdFields));

   if (type == type_) {
      productSelectionChanged();
   }
}

void FuturesTicket::onAmountEdited(const QString &)
{
   updatePanel();
}
