/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include <QClipboard>
#include <QFileDialog>
#include <QStandardPaths>
#include <QMetaEnum>
#include <QTimer>

#include "NetworkSettingsPage.h"
#include "ui_NetworkSettingsPage.h"
#include "ApplicationSettings.h"
#include "ArmoryServersWidget.h"
#include "WebSocketClient.h"
#include "HeadlessContainer.h"
#include "ArmoryServersViewModel.h"
#include "Settings/SignerSettings.h"
#include "SignersProvider.h"


struct EnvSettings
{
   QString  pubHost;
   int      pubPort;
};

bool operator == (const EnvSettings& l, const EnvSettings& r)
{
   return l.pubHost == r.pubHost
      && l.pubPort == r.pubPort;
}

NetworkSettingsPage::NetworkSettingsPage(QWidget* parent)
   : SettingsPage{parent}
   , ui_{new Ui::NetworkSettingsPage}
{
   ui_->setupUi(this);

   QMetaEnum metaEnumEnvConfiguration = QMetaEnum::fromType<ApplicationSettings::EnvConfiguration>();
   for (int i = 0; i < metaEnumEnvConfiguration.keyCount(); ++i) {
      ui_->comboBoxEnvironment->addItem(tr(metaEnumEnvConfiguration.valueToKey(i)));
   }
   ui_->comboBoxEnvironment->setCurrentIndex(-1);

   connect(ui_->pushButtonManageArmory, &QPushButton::clicked, this, [this](){
      // workaround here - wrap widget by QDialog
      // TODO: fix stylesheet to support popup widgets

      QDialog *d = new QDialog(this);
      QVBoxLayout *l = new QVBoxLayout(d);
      l->setContentsMargins(0,0,0,0);
      d->setLayout(l);
      d->setWindowTitle(tr("BlockSettleDB connection"));
      d->resize(847, 593);

      ArmoryServersWidget *armoryServersWidget = new ArmoryServersWidget(armoryServersProvider_, appSettings_, this);

//      armoryServersWidget->setWindowModality(Qt::ApplicationModal);
//      armoryServersWidget->setWindowFlags(Qt::Dialog);
      l->addWidget(armoryServersWidget);

      connect(armoryServersWidget, &ArmoryServersWidget::reconnectArmory, this, [this](){
         emit reconnectArmory();
      });
      connect(armoryServersWidget, &ArmoryServersWidget::needClose, this, [d](){
         d->reject();
      });

      d->exec();
      emit armoryServerChanged();
      // Switch env if needed
      onArmorySelected(ui_->comboBoxArmoryServer->currentIndex());
   });

   connect(ui_->pushButtonArmoryServerKeyCopy, &QPushButton::clicked, this, [this](){
      qApp->clipboard()->setText(ui_->labelArmoryServerKey->text());
      ui_->pushButtonArmoryServerKeyCopy->setEnabled(false);
      ui_->pushButtonArmoryServerKeyCopy->setText(tr("Copied"));
      QTimer::singleShot(2000, [this](){
         ui_->pushButtonArmoryServerKeyCopy->setEnabled(true);
         ui_->pushButtonArmoryServerKeyCopy->setText(tr("Copy"));
      });
   });
   connect(ui_->pushButtonArmoryServerKeySave, &QPushButton::clicked, this, [this](){
      QString fileName = QFileDialog::getSaveFileName(this
                                   , tr("Save BlockSettleDB Public Key")
                                   , QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation) + QDir::separator() + QStringLiteral("Armory_Server_Public_Key.pub")
                                   , tr("Key files (*.pub)"));

      QFile file(fileName);
      if (file.open(QIODevice::WriteOnly)) {
         file.write(ui_->labelArmoryServerKey->text().toLatin1());
      }
   });
}

void NetworkSettingsPage::initSettings()
{
   armoryServerModel_ = new ArmoryServersViewModel(armoryServersProvider_);
   armoryServerModel_->setSingleColumnMode(true);
   armoryServerModel_->setHighLightSelectedServer(false);
   ui_->comboBoxArmoryServer->setModel(armoryServerModel_);

   connect(armoryServersProvider_.get(), &ArmoryServersProvider::dataChanged, this, &NetworkSettingsPage::displayArmorySettings);
   connect(ui_->comboBoxEnvironment, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkSettingsPage::onEnvSelected);
   connect(ui_->comboBoxArmoryServer, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &NetworkSettingsPage::onArmorySelected);
}

NetworkSettingsPage::~NetworkSettingsPage() = default;

void NetworkSettingsPage::display()
{
   displayArmorySettings();
   displayEnvironmentSettings();

   disableSettingUpdate_ = false;
}

void NetworkSettingsPage::displayArmorySettings()
{
   // set index of selected server
   ArmoryServer selectedServer = armoryServersProvider_->getArmorySettings();
   int selectedServerIndex = armoryServersProvider_->indexOfCurrent();

   // Prevent NetworkSettingsPage::onArmorySelected call
   auto oldBlock = ui_->comboBoxArmoryServer->blockSignals(true);
   ui_->comboBoxArmoryServer->setCurrentIndex(selectedServerIndex);
   ui_->comboBoxArmoryServer->blockSignals(oldBlock);

   // display info of connected server
   ArmorySettings connectedServerSettings = armoryServersProvider_->connectedArmorySettings();
   int connectedServerIndex = armoryServersProvider_->indexOfConnected();

   ui_->labelArmoryServerNetwork->setText(connectedServerSettings.netType == NetworkType::MainNet ? tr("MainNet") : tr("TestNet"));
   ui_->labelArmoryServerAddress->setText(connectedServerSettings.armoryDBIp);
   ui_->labelArmoryServerPort->setText(QString::number(connectedServerSettings.armoryDBPort));
   ui_->labelArmoryServerKey->setText(connectedServerSettings.armoryDBKey);

   // display tip if configuration was changed
   if (selectedServerIndex != connectedServerIndex
       || selectedServer != static_cast<ArmoryServer>(connectedServerSettings)) {
      ui_->labelConfChanged->setVisible(true);
   }
   else {
      ui_->labelConfChanged->setVisible(false);
   }
}

void NetworkSettingsPage::displayEnvironmentSettings()
{
   auto env = appSettings_->get<int>(ApplicationSettings::envConfiguration);
   ui_->comboBoxEnvironment->setCurrentIndex(env);
   onEnvSelected(env);
}

void NetworkSettingsPage::applyLocalSignerNetOption()
{
   NetworkType networkType = static_cast<NetworkType>(appSettings_->get(ApplicationSettings::netType).toInt());
   SignerSettings settings;
   settings.setTestNet(networkType == NetworkType::TestNet);
}

void NetworkSettingsPage::reset()
{
   for (const auto &setting : {
        ApplicationSettings::runArmoryLocally,
        ApplicationSettings::netType,
        ApplicationSettings::envConfiguration,
        ApplicationSettings::armoryDbIp,
        ApplicationSettings::armoryDbPort}) {
      appSettings_->reset(setting, false);
   }
   display();
}

void NetworkSettingsPage::apply()
{
   armoryServersProvider_->setupServer(ui_->comboBoxArmoryServer->currentIndex());

   appSettings_->set(ApplicationSettings::envConfiguration, ui_->comboBoxEnvironment->currentIndex());

   if (signersProvider_->currentSignerIsLocal()) {
      applyLocalSignerNetOption();
   }
}

void NetworkSettingsPage::onEnvSelected(int envIndex)
{
   auto env = ApplicationSettings::EnvConfiguration(envIndex);

   if (disableSettingUpdate_) {
      return;
   }

   auto armoryServers = armoryServersProvider_->servers();
   auto armoryIndex = ui_->comboBoxArmoryServer->currentIndex();
   if (armoryIndex < 0 || armoryIndex >= armoryServers.count()) {
      return;
   }
   auto armoryServer = armoryServers[armoryIndex];

   if ((armoryServer.netType == NetworkType::MainNet) != (env == ApplicationSettings::EnvConfiguration::Production)) {
      if (env == ApplicationSettings::EnvConfiguration::Production) {
         ui_->comboBoxArmoryServer->setCurrentIndex(armoryServersProvider_->getIndexOfMainNetServer());
      }
      else {
         ui_->comboBoxArmoryServer->setCurrentIndex(armoryServersProvider_->getIndexOfTestNetServer());
      }
   }
}

void NetworkSettingsPage::onArmorySelected(int armoryIndex)
{
   auto armoryServers = armoryServersProvider_->servers();
   if (armoryIndex < 0 || armoryIndex >= armoryServers.count()) {
      return;
   }
   auto armoryServer = armoryServers[armoryIndex];
   auto envSelected = static_cast<ApplicationSettings::EnvConfiguration>(ui_->comboBoxEnvironment->currentIndex());

   if ((armoryServer.netType == NetworkType::MainNet) != (envSelected == ApplicationSettings::EnvConfiguration::Production)) {
      if (armoryServer.netType == NetworkType::MainNet) {
         ui_->comboBoxEnvironment->setCurrentIndex(static_cast<int>(ApplicationSettings::EnvConfiguration::Production));
      } else {
         ui_->comboBoxEnvironment->setCurrentIndex(static_cast<int>(ApplicationSettings::EnvConfiguration::Test));
      }
   }
}
