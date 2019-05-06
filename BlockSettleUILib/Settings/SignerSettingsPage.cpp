#include <QFileDialog>
#include <QStandardPaths>
#include "SignerSettingsPage.h"
#include "ui_SignerSettingsPage.h"
#include "ApplicationSettings.h"
#include "BtcUtils.h"
#include "BSMessageBox.h"
#include "SignContainer.h"
#include "SignersManageWidget.h"


enum RunModeIndex {
   Local = 0,
   Remote,
};


SignerSettingsPage::SignerSettingsPage(QWidget* parent)
   : SettingsPage{parent}
   , ui_{new Ui::SignerSettingsPage{}}
{
   ui_->setupUi(this);

   connect(ui_->comboBoxRunMode, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &SignerSettingsPage::runModeChanged);
   connect(ui_->spinBoxAsSpendLimit, QOverload<double>::of(&QDoubleSpinBox::valueChanged), this, &SignerSettingsPage::onAsSpendLimitChanged);
   connect(ui_->pushButtonManageSignerKeys, &QPushButton::clicked, this, &SignerSettingsPage::onManageSignerKeys);

   ui_->widgetTwoWayAuth->hide();
   ui_->checkBoxTwoWayAuth->hide();

   ui_->widgetTwoWayAuth->setMaximumHeight(0);
   ui_->checkBoxTwoWayAuth->setMaximumHeight(0);
}

SignerSettingsPage::~SignerSettingsPage() = default;

void SignerSettingsPage::runModeChanged(int index)
{
   onModeChanged(index);
}

void SignerSettingsPage::onModeChanged(int index)
{
   switch (static_cast<RunModeIndex>(index)) {
   case Local:
      showHost(false);
      showPort(true);
      ui_->spinBoxPort->setValue(appSettings_->get<int>(ApplicationSettings::localSignerPort));
      showLimits(true);
      showSignerKeySettings(false);
      ui_->spinBoxAsSpendLimit->setValue(appSettings_->get<double>(ApplicationSettings::autoSignSpendLimit));
      ui_->formLayoutConnectionParams->setSpacing(3);
      break;

   case Remote:
      showHost(true);
      ui_->comboBoxRemoteSigner->setCurrentIndex(appSettings_->get<int>(ApplicationSettings::signerIndex));
      showPort(true);
      ui_->spinBoxPort->setValue(appSettings_->get<int>(ApplicationSettings::localSignerPort));
      showLimits(false);
      showSignerKeySettings(true);
      ui_->formLayoutConnectionParams->setSpacing(6);
      break;

   default:    break;
   }
}

void SignerSettingsPage::display()
{
   const auto modeIndex = appSettings_->get<int>(ApplicationSettings::signerRunMode) - 1;
   onModeChanged(modeIndex);
   ui_->comboBoxRunMode->setCurrentIndex(modeIndex);
   ui_->checkBoxTwoWayAuth->setChecked(appSettings_->get<bool>(ApplicationSettings::twoWayAuth));
}

void SignerSettingsPage::reset()
{
   for (const auto &setting : {ApplicationSettings::signerRunMode
      , ApplicationSettings::localSignerPort, ApplicationSettings::signerOfflineDir
      , ApplicationSettings::remoteSigners, ApplicationSettings::autoSignSpendLimit
      , ApplicationSettings::twoWayAuth}) {
      appSettings_->reset(setting, false);
   }
   display();
}

void SignerSettingsPage::showHost(bool show)
{
   ui_->labelHost->setVisible(show);
   ui_->comboBoxRemoteSigner->setVisible(show);
}

void SignerSettingsPage::showPort(bool show)
{
   ui_->labelPort->setVisible(show);
   ui_->spinBoxPort->setVisible(show);
}

void SignerSettingsPage::showLimits(bool show)
{
   ui_->groupBoxAutoSign->setVisible(show);
   ui_->labelAsSpendLimit->setVisible(show);
   ui_->spinBoxAsSpendLimit->setVisible(show);
   onAsSpendLimitChanged(ui_->spinBoxAsSpendLimit->value());
}

void SignerSettingsPage::showSignerKeySettings(bool show)
{
   ui_->widgetTwoWayAuth->setVisible(show);
   ui_->checkBoxTwoWayAuth->setVisible(show);
   ui_->widgetSignerKeyComboBox->setVisible(show);
}

void SignerSettingsPage::onAsSpendLimitChanged(double value)
{
   if (value > 0) {
      ui_->labelAsSpendLimit->setText(tr("Spend Limit:"));
   }
   else {
      ui_->labelAsSpendLimit->setText(tr("Spend Limit - unlimited"));
   }
}

void SignerSettingsPage::onManageSignerKeys()
{
   // workaround here - wrap widget by QDialog
   // TODO: fix stylesheet to support popup widgets

   QDialog *d = new QDialog(this);
   QVBoxLayout *l = new QVBoxLayout(d);
   l->setContentsMargins(0,0,0,0);
   d->setLayout(l);
   d->setWindowTitle(tr("Import Signer Keys"));

   SignerKeysWidget *signerKeysWidget = new SignerKeysWidget(signersProvider_, appSettings_, this);
   d->resize(signerKeysWidget->size());

   l->addWidget(signerKeysWidget);

   connect(signerKeysWidget, &SignerKeysWidget::needClose, this, [d](){
      d->reject();
   });

   d->exec();

   emit signersChanged();
}

void SignerSettingsPage::apply()
{
   switch (static_cast<RunModeIndex>(ui_->comboBoxRunMode->currentIndex())) {
   case Local:
      appSettings_->set(ApplicationSettings::localSignerPort, ui_->spinBoxPort->value());
      appSettings_->set(ApplicationSettings::autoSignSpendLimit, ui_->spinBoxAsSpendLimit->value());
      break;

   case Remote:
      appSettings_->set(ApplicationSettings::localSignerPort, ui_->spinBoxPort->value());
      signersProvider_->setupSigner(ui_->comboBoxRemoteSigner->currentIndex());
      break;

   default:    break;
   }

   // first comboBoxRunMode index is '--Select--' placeholder
   appSettings_->set(ApplicationSettings::signerRunMode, ui_->comboBoxRunMode->currentIndex() + 1);
   appSettings_->set(ApplicationSettings::twoWayAuth, ui_->checkBoxTwoWayAuth->isChecked());
}

void SignerSettingsPage::initSettings()
{
   signersModel_ = new SignersModel(signersProvider_);
   signersModel_->setSingleColumnMode(true);
   signersModel_->setHighLightSelectedServer(false);
   ui_->comboBoxRemoteSigner->setModel(signersModel_);

   connect(signersProvider_.get(), &SignersProvider::dataChanged, this, &SignerSettingsPage::display);
}
