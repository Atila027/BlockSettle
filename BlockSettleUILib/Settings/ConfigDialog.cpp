/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "ConfigDialog.h"

#include "ArmoryServersProvider.h"
#include "AssetManager.h"
#include "GeneralSettingsPage.h"
#include "NetworkSettingsPage.h"
#include "SignersProvider.h"
#include "WalletSignerContainer.h"
#include "Wallets/SyncHDWallet.h"
#include "Wallets/SyncWalletsManager.h"
#include "autheid_utils.h"

#include "ui_ConfigDialog.h"

#include <QPushButton>


SettingsPage::SettingsPage(QWidget *parent)
   : QWidget(parent)
{
}

void SettingsPage::init(const std::shared_ptr<ApplicationSettings> &appSettings
   , const std::shared_ptr<ArmoryServersProvider> &armoryServersProvider
   , const std::shared_ptr<SignersProvider> &signersProvider
   , const std::shared_ptr<SignContainer> &signContainer
   , const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr) {
   appSettings_ = appSettings;
   armoryServersProvider_ = armoryServersProvider;
   signersProvider_ = signersProvider;
   signContainer_ = signContainer;
   walletsMgr_ = walletsMgr;
   initSettings();
   display();
}


ConfigDialog::ConfigDialog(const std::shared_ptr<ApplicationSettings>& appSettings
      , const std::shared_ptr<ArmoryServersProvider> &armoryServersProvider
      , const std::shared_ptr<SignersProvider> &signersProvider
      , const std::shared_ptr<SignContainer> &signContainer
      , const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
      , QWidget* parent)
 : QDialog(parent)
 , ui_(new Ui::ConfigDialog)
 , appSettings_(appSettings)
 , armoryServersProvider_(armoryServersProvider)
 , signersProvider_(signersProvider)
 , signContainer_(signContainer)
{
   ui_->setupUi(this);

   if (!appSettings_->get<bool>(ApplicationSettings::initialized)) {
      appSettings_->SetDefaultSettings(true);
      ui_->pushButtonCancel->setEnabled(false);
   }
   prevState_ = appSettings_->getState();

   pages_ = {ui_->pageGeneral, ui_->pageNetwork, ui_->pageSigner, ui_->pageDealing
      , ui_->pageAPI };

   for (const auto &page : pages_) {
      page->init(appSettings_, armoryServersProvider_, signersProvider_, signContainer_, walletsMgr);
      connect(page, &SettingsPage::illformedSettings, this, &ConfigDialog::illformedSettings);
   }

   ui_->listWidget->setCurrentRow(0);
   ui_->stackedWidget->setCurrentIndex(0);

   connect(ui_->listWidget, &QListWidget::currentRowChanged, this, &ConfigDialog::onSelectionChanged);
   connect(ui_->pushButtonSetDefault, &QPushButton::clicked, this, &ConfigDialog::onDisplayDefault);
   connect(ui_->pushButtonCancel, &QPushButton::clicked, this, &ConfigDialog::reject);
   connect(ui_->pushButtonOk, &QPushButton::clicked, this, &ConfigDialog::onAcceptSettings);

   connect(ui_->pageNetwork, &NetworkSettingsPage::reconnectArmory, this, [this](){
      emit reconnectArmory();
   });

   // armory servers should be saved even if whole ConfigDialog rejected
   // we overwriting prevState_ with new vales once ArmoryServersWidget closed
   connect(ui_->pageNetwork, &NetworkSettingsPage::armoryServerChanged, this, [this](){
      for (ApplicationSettings::Setting s : {
           ApplicationSettings::armoryServers,
           ApplicationSettings::runArmoryLocally,
           ApplicationSettings::netType,
           ApplicationSettings::armoryDbName,
           ApplicationSettings::armoryDbIp,
           ApplicationSettings::armoryDbPort}) {
         prevState_[s] = appSettings_->get(s);
      }
   });

   connect(ui_->pageSigner, &SignerSettingsPage::signersChanged, this, [this](){
      for (ApplicationSettings::Setting s : {
           ApplicationSettings::remoteSigners,
           ApplicationSettings::signerIndex}) {
         prevState_[s] = appSettings_->get(s);
      }
   });

   connect(ui_->pageGeneral, &GeneralSettingsPage::requestDataEncryption, this, [this]() {
      signContainer_->customDialogRequest(bs::signer::ui::GeneralDialogType::ManagePublicDataEncryption);
   });
}

ConfigDialog::~ConfigDialog() = default;

void ConfigDialog::popupNetworkSettings()
{
   ui_->stackedWidget->setCurrentWidget(ui_->pageNetwork);
   ui_->listWidget->setCurrentRow(ui_->stackedWidget->indexOf(ui_->pageNetwork));
}

QString ConfigDialog::encryptErrorStr(EncryptError error)
{
   switch (error) {
      case EncryptError::NoError:            return tr("No error");
      case EncryptError::NoPrimaryWallet:    return tr("No primary wallet");
      case EncryptError::NoEncryptionKey:    return tr("No encryption key");
      case EncryptError::EncryptError:       return tr("Encryption error");
   }
   assert(false);
   return tr("Unknown error");
}

void ConfigDialog::encryptData(const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
   , const std::shared_ptr<SignContainer> &signContainer
   , const SecureBinaryData &data
   , const ConfigDialog::EncryptCb &cb)
{
   getChatPrivKey(walletsMgr, signContainer, [data, cb](EncryptError error, const SecureBinaryData &privKey) {
      if (error != EncryptError::NoError) {
         cb(error, {});
         return;
      }
      auto pubKey = autheid::getPublicKey(autheid::PrivateKey(privKey.getPtr(), privKey.getPtr() + privKey.getSize()));
      auto encrypted = autheid::encryptData(data.getPtr(), data.getSize(), pubKey);
      if (encrypted.empty()) {
         cb(EncryptError::EncryptError, {});
         return;
      }
      cb(EncryptError::NoError, SecureBinaryData(encrypted.data(), encrypted.size()));
   });
}

void ConfigDialog::decryptData(const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
   , const std::shared_ptr<SignContainer> &signContainer
   , const SecureBinaryData &data, const ConfigDialog::EncryptCb &cb)
{
   getChatPrivKey(walletsMgr, signContainer, [data, cb](EncryptError error, const SecureBinaryData &privKey) {
      if (error != EncryptError::NoError) {
         cb(error, {});
         return;
      }
      auto privKeyCopy = autheid::PrivateKey(privKey.getPtr(), privKey.getPtr() + privKey.getSize());
      auto decrypted = autheid::decryptData(data.getPtr(), data.getSize(), privKeyCopy);
      if (decrypted.empty()) {
         cb(EncryptError::EncryptError, {});
         return;
      }
      cb(EncryptError::NoError, SecureBinaryData(decrypted.data(), decrypted.size()));
   });
}

void ConfigDialog::getChatPrivKey(const std::shared_ptr<bs::sync::WalletsManager> &walletsMgr
   , const std::shared_ptr<SignContainer> &signContainer
   , const ConfigDialog::EncryptCb &cb)
{
   const auto &primaryWallet = walletsMgr->getPrimaryWallet();
   if (!primaryWallet) {
      cb(EncryptError::NoPrimaryWallet, {});
      return;
   }
   auto walletSigner = std::dynamic_pointer_cast<WalletSignerContainer>(signContainer);
   walletSigner->getChatNode(primaryWallet->walletId(), [cb](const BIP32_Node &node) {
      if (node.getPrivateKey().empty()) {
         cb(EncryptError::NoEncryptionKey, {});
         return;
      }
      cb(EncryptError::NoError, node.getPrivateKey());
   });
}

void ConfigDialog::onDisplayDefault()
{  // reset only currently selected page - maybe a subject to change
   pages_[ui_->stackedWidget->currentIndex()]->reset();
}

void ConfigDialog::onAcceptSettings()
{
   for (const auto &page : pages_) {
      page->apply();
   }

   appSettings_->SaveSettings();
   accept();
}

void ConfigDialog::onSelectionChanged(int currentRow)
{
   ui_->stackedWidget->setCurrentIndex(currentRow);
}

void ConfigDialog::illformedSettings(bool illformed)
{
   ui_->pushButtonOk->setEnabled(!illformed);
}

void ConfigDialog::reject()
{
   appSettings_->setState(prevState_);
   QDialog::reject();
}
