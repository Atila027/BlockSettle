#ifndef SIGNER_ADAPTER_H
#define SIGNER_ADAPTER_H

#include <memory>
#include <QObject>
#include "CoreWallet.h"
#include "SignContainer.h"

namespace bs {
   namespace sync {
      class WalletsManager;
   }
}
namespace spdlog {
   class logger;
}
class SignContainer;
class SignerInterfaceListener;

class SignerAdapter : public QObject
{
   Q_OBJECT
   friend class SignerInterfaceListener;

public:
   SignerAdapter(const std::shared_ptr<spdlog::logger> &, NetworkType);
   ~SignerAdapter() override;

   SignerAdapter(const SignerAdapter&) = delete;
   SignerAdapter& operator = (const SignerAdapter&) = delete;
   SignerAdapter(SignerAdapter&&) = delete;
   SignerAdapter& operator = (SignerAdapter&&) = delete;

   std::shared_ptr<bs::sync::WalletsManager> getWalletsManager();
   void reloadWallets(const QString &, const std::function<void()> &);

   void setOnline(bool);
   void reconnect(const QString &address, const QString &port);
   void setLimits(SignContainer::Limits);
   void passwordReceived(const std::string &walletId, const SecureBinaryData &, bool cancelledByUser);

   void signTxRequest(const bs::core::wallet::TXSignRequest &, const SecureBinaryData &password
      , const std::function<void(const BinaryData &)> &);
   void createWatchingOnlyWallet(const QString &walletId, const SecureBinaryData &password
      , const std::function<void(const bs::sync::WatchingOnlyWallet &)> &);
   void getDecryptedRootNode(const std::string &walletId, const SecureBinaryData &password
      , const std::function<void(const SecureBinaryData &privKey, const SecureBinaryData &chainCode)> &);

   void addPendingAutoSignReq(const std::string &walletId);
   void deactivateAutoSign();

   NetworkType netType() const { return netType_; }

signals:
   void ready() const;
   void peerConnected(const QString &ip);
   void peerDisconnected(const QString &ip);
   void requestPassword(const bs::core::wallet::TXSignRequest &, const QString &prompt);
   void autoSignRequiresPwd(const std::string &walletId);
   void cancelTxSign(const BinaryData &txHash);
   void txSigned(const BinaryData &);
   void xbtSpent(const qint64 value, bool autoSign);
   void autoSignActivated(const std::string &walletId);
   void autoSignDeactivated(const std::string &walletId);
   void customDialogRequest(const QString &dialogName, const QVariantMap &data);

private:
   std::shared_ptr<spdlog::logger>  logger_;
   NetworkType netType_;
   std::shared_ptr<SignContainer>   signContainer_;
   std::shared_ptr<bs::sync::WalletsManager> walletsMgr_;
   std::shared_ptr<SignerInterfaceListener>  listener_;
};


#endif // SIGNER_ADAPTER_H