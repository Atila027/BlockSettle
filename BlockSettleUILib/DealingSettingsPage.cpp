#include "DealingSettingsPage.h"
#include "ui_DealingSettingsPage.h"

#include "ApplicationSettings.h"
#include "AssetManager.h"
#include "SecuritiesModel.h"


DealingSettingsPage::DealingSettingsPage(QWidget* parent)
   : QWidget{parent}
   , ui_{new Ui::DealingSettingsPage{}}
{
   ui_->setupUi(this);

   connect(ui_->pushButtonResetCnt, &QPushButton::clicked, this, &DealingSettingsPage::onResetCountes);
}

DealingSettingsPage::~DealingSettingsPage()
{}

void DealingSettingsPage::setAppSettings(const std::shared_ptr<ApplicationSettings>& appSettings)
{
   appSettings_ = appSettings;
}

static inline int limitIndex(int limit)
{
   switch(limit) {
      case 1 :
         return 0;

      case 3 :
         return 1;

      case 5 :
         return 2;

      case 10 :
         return 3;

      case -1 :
         return 4;

      default :
         return 4;
   }
}

static inline int limit(int index)
{
   switch(index) {
      case 0 :
         return 1;

      case 1 :
         return 3;

      case 2 :
         return 5;

      case 3 :
         return 10;

      case 4 :
         return -1;

      default :
         return 5;
   }
}

static inline int priceUpdateIndex(int timeout)
{
   if (timeout <= 0) {
      return 0;
   } else if (timeout <= 1000) {
      return 1;
   } else if (timeout <= 3000) {
      return 2;
   } else {
      return 3;
   }
}

static inline int priceUpdateTimeout(int index)
{
   switch (index) {
      case 0 :
         return -1;

      case 1 :
         return 1000;

      case 2 :
         return 3000;

      case 3 :
         return 5000;

      default :
         return -1;
   }
}

void DealingSettingsPage::displaySettings(const std::shared_ptr<AssetManager> &assetMgr
   , bool displayDefault)
{
   ui_->checkBoxDrop->setChecked(appSettings_->get<bool>(ApplicationSettings::dropQN,
      displayDefault));
   ui_->showQuoted->setChecked(appSettings_->get<bool>(ApplicationSettings::ShowQuoted,
      displayDefault));
   ui_->fx->setCurrentIndex(limitIndex(appSettings_->get<int>(ApplicationSettings::FxRfqLimit,
      displayDefault)));
   ui_->xbt->setCurrentIndex(limitIndex(appSettings_->get<int>(ApplicationSettings::XbtRfqLimit,
      displayDefault)));
   ui_->pm->setCurrentIndex(limitIndex(appSettings_->get<int>(ApplicationSettings::PmRfqLimit,
      displayDefault)));
   ui_->disableBlueDot->setChecked(appSettings_->get<bool>(
      ApplicationSettings::DisableBlueDotOnTabOfRfqBlotter, displayDefault));
   ui_->priceUpdateTimeout->setCurrentIndex(priceUpdateIndex(appSettings_->get<int>(
      ApplicationSettings::PriceUpdateInterval, displayDefault)));
}

void DealingSettingsPage::applyChanges()
{
   appSettings_->set(ApplicationSettings::dropQN, ui_->checkBoxDrop->isChecked());
   appSettings_->set(ApplicationSettings::ShowQuoted, ui_->showQuoted->isChecked());
   appSettings_->set(ApplicationSettings::DisableBlueDotOnTabOfRfqBlotter,
      ui_->disableBlueDot->isChecked());
   appSettings_->set(ApplicationSettings::FxRfqLimit, limit(ui_->fx->currentIndex()));
   appSettings_->set(ApplicationSettings::XbtRfqLimit, limit(ui_->xbt->currentIndex()));
   appSettings_->set(ApplicationSettings::PmRfqLimit, limit(ui_->pm->currentIndex()));
   appSettings_->set(ApplicationSettings::PriceUpdateInterval, priceUpdateTimeout(
      ui_->priceUpdateTimeout->currentIndex()));
}

void DealingSettingsPage::onResetCountes()
{
   appSettings_->reset(ApplicationSettings::Filter_MD_QN_cnt);
   ui_->pushButtonResetCnt->setEnabled(false);
}
