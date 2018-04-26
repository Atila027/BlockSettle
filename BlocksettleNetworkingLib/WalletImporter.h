#ifndef __WALLET_IMPORTER_H__
#define __WALLET_IMPORTER_H__

#include <functional>
#include <memory>
#include <string>
#include <QObject>
#include <QStringList>
#include "HDWallet.h"


class ApplicationSettings;
class AssetManager;
class AuthAddressManager;
class SignContainer;
class WalletsManager;


class WalletImporter : public QObject
{
   Q_OBJECT

public:
   WalletImporter(const std::shared_ptr<SignContainer> &, const std::shared_ptr<WalletsManager> &
      , const std::shared_ptr<PyBlockDataManager> &, const std::shared_ptr<AssetManager> &
      , const std::shared_ptr<AuthAddressManager> &
      , const bs::hd::Wallet::cb_scan_read_last &, const bs::hd::Wallet::cb_scan_write_last &);

   void Import(const std::string& name, const std::string& description
      , bs::wallet::Seed seed, bool primary = false, const SecureBinaryData &password = {});

signals:
   void walletCreated(const std::string &rootWalletId);
   void error(const QString &errMsg);

private slots:
   void onHDWalletCreated(unsigned int id, std::shared_ptr<bs::hd::Wallet>);
   void onHDLeafCreated(unsigned int id, BinaryData pubKey, BinaryData chainCode, std::string walletId);
   void onHDWalletError(unsigned int id, std::string error);
   void onWalletScanComplete(bs::hd::Group *, bs::hd::Path::Elem wallet, bool isValid);

private:
   std::shared_ptr<SignContainer>      signingContainer_;
   std::shared_ptr<WalletsManager>     walletsMgr_;
   std::shared_ptr<PyBlockDataManager> bdm_;
   std::shared_ptr<AssetManager>       assetMgr_;
   std::shared_ptr<AuthAddressManager> authMgr_;
   const bs::hd::Wallet::cb_scan_read_last   cbReadLast_;
   const bs::hd::Wallet::cb_scan_write_last  cbWriteLast_;
   std::shared_ptr<bs::hd::Wallet>     rootWallet_;
   std::map<unsigned int, std::string> createCCWalletReqs_;
   unsigned int      createWalletReq_ = 0;
   SecureBinaryData  password_;
   std::unordered_map<unsigned int, bs::hd::Path>     createNextWalletReqs_;
};

#endif // __WALLET_IMPORTER_H__
