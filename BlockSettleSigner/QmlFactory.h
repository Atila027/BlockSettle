#ifndef QMLFACTORY_H
#define QMLFACTORY_H

#include <QObject>
#include <QQmlEngine>

#include "QWalletInfo.h"

class QmlFactory : public QObject
{
   Q_OBJECT
public:
   QmlFactory(std::shared_ptr<WalletsManager> walletsMgr
              , const std::shared_ptr<spdlog::logger> &logger
              , QObject *parent = nullptr)
      : walletsMgr_(walletsMgr)
      , logger_(logger)
      , QObject(parent) {}

   // QSeed
   Q_INVOKABLE bs::wallet::QPasswordData *createPasswordData() {
      auto pd = new bs::wallet::QPasswordData();
      QQmlEngine::setObjectOwnership(pd, QQmlEngine::JavaScriptOwnership);
      return pd;
   }

   Q_INVOKABLE bs::wallet::QSeed *createSeed(bool isTestNet){
      auto seed = new bs::wallet::QSeed(isTestNet);
      QQmlEngine::setObjectOwnership(seed, QQmlEngine::JavaScriptOwnership);
      return seed;
   }

   Q_INVOKABLE bs::wallet::QSeed *createSeedFromPaperBackup(const QString &key, bs::wallet::QNetworkType netType) {
      auto seed = new bs::wallet::QSeed(bs::wallet::QSeed::fromPaperKey(key, netType));
      QQmlEngine::setObjectOwnership(seed, QQmlEngine::JavaScriptOwnership);
      return seed;
   }

   Q_INVOKABLE bs::wallet::QSeed *createSeedFromPaperBackupT(const QString &key, bool isTestNet) {
      auto seed = new bs::wallet::QSeed(bs::wallet::QSeed::fromPaperKey(key
                                        , isTestNet ? bs::wallet::QNetworkType::TestNet : bs::wallet::QNetworkType::MainNet));
      QQmlEngine::setObjectOwnership(seed, QQmlEngine::JavaScriptOwnership);
      return seed;
   }

   Q_INVOKABLE bs::wallet::QSeed *createSeedFromDigitalBackup(const QString &filename, bs::wallet::QNetworkType netType) {
      auto seed = new bs::wallet::QSeed(bs::wallet::QSeed::fromDigitalBackup(filename, netType));
      QQmlEngine::setObjectOwnership(seed, QQmlEngine::JavaScriptOwnership);
      return seed;
   }

   Q_INVOKABLE bs::wallet::QSeed *createSeedFromDigitalBackupT(const QString &filename, bool isTestNet) {
      auto seed = new bs::wallet::QSeed(bs::wallet::QSeed::fromDigitalBackup(filename
                                                                             , isTestNet ? bs::wallet::QNetworkType::TestNet : bs::wallet::QNetworkType::MainNet));
      QQmlEngine::setObjectOwnership(seed, QQmlEngine::JavaScriptOwnership);
      return seed;
   }

   // WalletInfo
   Q_INVOKABLE bs::hd::WalletInfo *createWalletInfo() {
      auto wi = new bs::hd::WalletInfo();
      QQmlEngine::setObjectOwnership(wi, QQmlEngine::JavaScriptOwnership);
      return wi;
   }

   Q_INVOKABLE bs::hd::WalletInfo *createWalletInfo(const QString &walletId) {
      auto wi = new bs::hd::WalletInfo(walletsMgr_, walletId, this);
      QQmlEngine::setObjectOwnership(wi, QQmlEngine::JavaScriptOwnership);
      return wi;
   }

   Q_INVOKABLE bs::hd::WalletInfo *createWalletInfoFromDigitalBackup(const QString &filename) {
      auto wi = new bs::hd::WalletInfo(bs::hd::WalletInfo::fromDigitalBackup(filename));
      QQmlEngine::setObjectOwnership(wi, QQmlEngine::JavaScriptOwnership);
      return wi;
   }



   // Auth
   // used for signing
   Q_INVOKABLE AuthSignWalletObject *createAutheIDSignObject(AutheIDClient::RequestType requestType
                                                             , bs::hd::WalletInfo *walletInfo);

   // used for add new eID device
   Q_INVOKABLE AuthSignWalletObject *createActivateEidObject(const QString &userId
                                                             , bs::hd::WalletInfo *walletInfo);

   // used for remove eID device
   // index: is encKeys index which should be deleted
   Q_INVOKABLE AuthSignWalletObject *createRemoveEidObject(int index
                                                             , bs::hd::WalletInfo *walletInfo);
private:
   std::shared_ptr<WalletsManager> walletsMgr_;
   std::shared_ptr<spdlog::logger> logger_;
};


#endif // QMLFACTORY_H
