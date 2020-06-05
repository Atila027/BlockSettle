/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "AutoSignQuoteProvider.h"

#include "ApplicationSettings.h"
#include "SignContainer.h"
#include "WalletManager.h"
#include "Wallets/SyncWalletsManager.h"
#include "Wallets/SyncHDWallet.h"
#include "UserScriptRunner.h"

#include <BaseCelerClient.h>
#include <QCoreApplication>
#include <QFileDialog>
#include <QFileInfo>


AutoSignScriptProvider::AutoSignScriptProvider(const std::shared_ptr<spdlog::logger> &logger
   , UserScriptRunner *scriptRunner
   , const std::shared_ptr<ApplicationSettings> &appSettings
   , const std::shared_ptr<SignContainer> &container
   , const std::shared_ptr<BaseCelerClient> &celerClient
   , QObject *parent)
   : QObject(parent), logger_(logger), scriptRunner_(scriptRunner)
   , appSettings_(appSettings)
   , signingContainer_(container)
   , celerClient_(celerClient)
{
   scriptRunner_->setParent(this);

   if (walletsManager_) {
      scriptRunner_->setWalletsManager(walletsManager_);
   }

   if (signingContainer_) {
      connect(signingContainer_.get(), &SignContainer::ready, this
         , &AutoSignScriptProvider::onSignerStateUpdated, Qt::QueuedConnection);
      connect(signingContainer_.get(), &SignContainer::disconnected, this
         , &AutoSignScriptProvider::onSignerStateUpdated, Qt::QueuedConnection);
      connect(signingContainer_.get(), &SignContainer::AutoSignStateChanged, this
         , &AutoSignScriptProvider::onAutoSignStateChanged);
   }

   connect(scriptRunner_, &UserScriptRunner::scriptLoaded, this, &AutoSignScriptProvider::onScriptLoaded);
   connect(scriptRunner_, &UserScriptRunner::failedToLoad, this, &AutoSignScriptProvider::onScriptFailed);

   onSignerStateUpdated();

   auto botFileInfo = QFileInfo(getDefaultScriptsDir() + QStringLiteral("/RFQBot.qml"));
   if (botFileInfo.exists() && botFileInfo.isFile()) {
      auto list = appSettings_->get<QStringList>(ApplicationSettings::aqScripts);
      if (list.indexOf(botFileInfo.absoluteFilePath()) == -1) {
         list << botFileInfo.absoluteFilePath();
      }
      appSettings_->set(ApplicationSettings::aqScripts, list);
      const auto lastScript = appSettings_->get<QString>(ApplicationSettings::lastAqScript);
      if (lastScript.isEmpty()) {
         appSettings_->set(ApplicationSettings::lastAqScript, botFileInfo.absoluteFilePath());
      }

   }

   connect(celerClient_.get(), &BaseCelerClient::OnConnectedToServer, this, &AutoSignScriptProvider::onConnectedToCeler);
   connect(celerClient_.get(), &BaseCelerClient::OnConnectionClosed, this, &AutoSignScriptProvider::onDisconnectedFromCeler);
}

void AutoSignScriptProvider::onSignerStateUpdated()
{
   disableAutoSign();
   scriptRunner_->disable();

   emit autoSignQuoteAvailabilityChanged();
}

void AutoSignScriptProvider::disableAutoSign()
{
   if (!walletsManager_) {
      return;
   }

   const auto wallet = walletsManager_->getPrimaryWallet();
   if (!wallet) {
      logger_->error("Failed to obtain auto-sign primary wallet");
      return;
   }

   QVariantMap data;
   data[QLatin1String("rootId")] = QString::fromStdString(wallet->walletId());
   data[QLatin1String("enable")] = false;
   signingContainer_->customDialogRequest(bs::signer::ui::GeneralDialogType::ActivateAutoSign, data);
}

void AutoSignScriptProvider::tryEnableAutoSign()
{
   if (!walletsManager_ || !signingContainer_) {
      return;
   }

   const auto wallet = walletsManager_->getPrimaryWallet();
   if (!wallet) {
      logger_->error("Failed to obtain auto-sign primary wallet");
      return;
   }

   QVariantMap data;
   data[QLatin1String("rootId")] = QString::fromStdString(wallet->walletId());
   data[QLatin1String("enable")] = true;
   signingContainer_->customDialogRequest(bs::signer::ui::GeneralDialogType::ActivateAutoSign, data);
}

bool AutoSignScriptProvider::isReady() const
{
   logger_->debug("[{}] signCont: {}, walletsMgr: {}, celer: {}", __func__
      , signingContainer_ && !signingContainer_->isOffline()
      , walletsManager_ && walletsManager_->isReadyForTrading(), celerClient_->IsConnected());
   return signingContainer_ && !signingContainer_->isOffline()
      && walletsManager_ && walletsManager_->isReadyForTrading()
      && celerClient_->IsConnected();
}

void AutoSignScriptProvider::onAutoSignStateChanged(bs::error::ErrorCode result
   , const std::string &walletId)
{
   autoSignState_ = result;
   autoSignWalletId_ = QString::fromStdString(walletId);
   emit autoSignStateChanged();
}

void AutoSignScriptProvider::setScriptLoaded(bool loaded)
{
   scriptLoaded_ = loaded;
   if (!loaded) {
      scriptRunner_->disable();
   }
}

void AutoSignScriptProvider::init(const QString &filename)
{
   if (filename.isEmpty()) {
      return;
   }
   scriptLoaded_ = false;
   scriptRunner_->enable(filename);
}

void AutoSignScriptProvider::deinit()
{
   scriptRunner_->disable();
   scriptLoaded_ = false;
   emit scriptUnLoaded();
}

void AutoSignScriptProvider::onScriptLoaded(const QString &filename)
{
   logger_->info("[AutoSignScriptProvider::onScriptLoaded] script {} loaded"
      , filename.toStdString());
   scriptLoaded_ = true;

   auto scripts = appSettings_->get<QStringList>(ApplicationSettings::aqScripts);
   if (scripts.indexOf(filename) < 0) {
      scripts << filename;
      appSettings_->set(ApplicationSettings::aqScripts, scripts);
   }
   appSettings_->set(ApplicationSettings::lastAqScript, filename);
   emit scriptLoaded(filename);
   emit scriptHistoryChanged();
}

void AutoSignScriptProvider::onScriptFailed(const QString &filename, const QString &error)
{
   logger_->error("[AutoSignScriptProvider::onScriptLoaded] script {} loading failed: {}"
      , filename.toStdString(), error.toStdString());
   setScriptLoaded(false);

   auto scripts = appSettings_->get<QStringList>(ApplicationSettings::aqScripts);
   scripts.removeOne(filename);
   appSettings_->set(ApplicationSettings::aqScripts, scripts);
   appSettings_->reset(ApplicationSettings::lastAqScript);
   emit scriptHistoryChanged();
}

void AutoSignScriptProvider::onConnectedToCeler()
{
   emit autoSignQuoteAvailabilityChanged();
}

void AutoSignScriptProvider::onDisconnectedFromCeler()
{
   scriptRunner_->disable();
   disableAutoSign();

   emit autoSignQuoteAvailabilityChanged();
}

void AutoSignScriptProvider::setWalletsManager(std::shared_ptr<bs::sync::WalletsManager> &walletsMgr)
{
   walletsManager_ = walletsMgr;
   scriptRunner_->setWalletsManager(walletsMgr);

   connect(walletsMgr.get(), &bs::sync::WalletsManager::walletDeleted, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);
   connect(walletsMgr.get(), &bs::sync::WalletsManager::walletAdded, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);
   connect(walletsMgr.get(), &bs::sync::WalletsManager::walletsReady, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);
   connect(walletsMgr.get(), &bs::sync::WalletsManager::walletsSynchronized, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);
   connect(walletsMgr.get(), &bs::sync::WalletsManager::newWalletAdded, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);
   connect(walletsMgr.get(), &bs::sync::WalletsManager::walletImportFinished, this
      , &AutoSignScriptProvider::autoSignQuoteAvailabilityChanged);

   emit autoSignQuoteAvailabilityChanged();
}

QString AutoSignScriptProvider::getAutoSignWalletName()
{
   if (!walletsManager_ || !signingContainer_) {
      return QString();
   }

   const auto wallet = walletsManager_->getPrimaryWallet();
   if (!wallet) {
      return QString();
   }
   return QString::fromStdString(wallet->name());
}

QString AutoSignScriptProvider::getDefaultScriptsDir()
{
#if defined(Q_OS_WIN) || defined(Q_OS_MACOS)
   return QCoreApplication::applicationDirPath() + QStringLiteral("/scripts");
#else
   return QStringLiteral("/usr/share/blocksettle/scripts");
#endif
}

QStringList AutoSignScriptProvider::getScripts()
{
   return appSettings_->get<QStringList>(ApplicationSettings::aqScripts);
}

QString AutoSignScriptProvider::getLastScript()
{
   return appSettings_->get<QString>(ApplicationSettings::lastAqScript);
}

QString AutoSignScriptProvider::getLastDir()
{
   return appSettings_->get<QString>(ApplicationSettings::LastAqDir);
}

void AutoSignScriptProvider::setLastDir(const QString &path)
{
   appSettings_->set(ApplicationSettings::LastAqDir, QFileInfo(path).dir().absolutePath());
}
