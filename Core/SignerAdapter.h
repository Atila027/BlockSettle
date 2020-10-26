/*

***********************************************************************************
* Copyright (C) 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef SIGNER_ADAPTER_H
#define SIGNER_ADAPTER_H

#include "FutureValue.h"
#include "HeadlessContainer.h"
#include "Message/Adapter.h"

namespace spdlog {
   class logger;
}
namespace BlockSettle {
   namespace Common {
      class SignerMessage;
      class SignerMessage_ExtendAddrChain;
      class SignerMessage_SetSettlementId;
      class SignerMessage_SignTxRequest;
      class SignerMessage_SyncAddresses;
      class SignerMessage_SyncAddressComment;
      class SignerMessage_SyncNewAddresses;
      class SignerMessage_SyncTxComment;
   }
   namespace Terminal {
      class SettingsMessage_SignerServer;
   }
}
class SignerClient;
class WalletSignerContainer;

class SignerAdapter : public bs::message::Adapter, public HeadlessCallbackTarget
{
public:
   SignerAdapter(const std::shared_ptr<spdlog::logger> &);
   ~SignerAdapter() override = default;

   bool process(const bs::message::Envelope &) override;

   std::set<std::shared_ptr<bs::message::User>> supportedReceivers() const override {
      return { user_ };
   }
   std::string name() const override { return "Signer"; }

   std::unique_ptr<SignerClient> createClient() const;

private:
   void start();

   // HCT overrides
   void connError(SignContainer::ConnectionError, const QString &) override;
   void connTorn() override;
   void authLeafAdded(const std::string &walletId) override;
   void walletsChanged() override;
   void onReady() override;
   void walletsReady() override;
   void newWalletPrompt() override;
   void windowIsVisible(bool) override;

   bool processOwnRequest(const bs::message::Envelope &
      , const BlockSettle::Common::SignerMessage &);
   bool processSignerSettings(const BlockSettle::Terminal::SettingsMessage_SignerServer &);
   bool processNewKeyResponse(bool);
   std::shared_ptr<WalletSignerContainer> makeRemoteSigner(
      const BlockSettle::Terminal::SettingsMessage_SignerServer &);
   bool sendComponentLoading();

   bool processStartWalletSync(const bs::message::Envelope &);
   bool processSyncAddresses(const bs::message::Envelope &
      , const BlockSettle::Common::SignerMessage_SyncAddresses &);
   bool processSyncNewAddresses(const bs::message::Envelope &
      , const BlockSettle::Common::SignerMessage_SyncNewAddresses &);
   bool processExtendAddrChain(const bs::message::Envelope &
      , const BlockSettle::Common::SignerMessage_ExtendAddrChain &);
   bool processSyncWallet(const bs::message::Envelope &, const std::string &walletId);
   bool processSyncHdWallet(const bs::message::Envelope &, const std::string &walletId);
   bool processSyncAddrComment(const BlockSettle::Common::SignerMessage_SyncAddressComment &);
   bool processSyncTxComment(const BlockSettle::Common::SignerMessage_SyncTxComment &);
   bool processSetSettlId(const bs::message::Envelope &
      , const BlockSettle::Common::SignerMessage_SetSettlementId &);
   bool processGetRootPubKey(const bs::message::Envelope &, const std::string &walletId);
   bool processDelHdRoot(const std::string &walletId);
   bool processDelHdLeaf(const std::string &walletId);
   bool processSignTx(const bs::message::Envelope&
      , const BlockSettle::Common::SignerMessage_SignTxRequest&);
   bool processSetUserId(const std::string& userId, const std::string& walletId);
   bool processCreateSettlWallet(const bs::message::Envelope&, const std::string &);

private:
   std::shared_ptr<spdlog::logger>        logger_;
   std::shared_ptr<bs::message::User>     user_;
   std::shared_ptr<WalletSignerContainer> signer_;

   std::shared_ptr<FutureValue<bool>>  connFuture_;
   std::string    curServerId_;
   std::string    connKey_;

   std::map<uint64_t, std::shared_ptr<bs::message::User>>   requests_;
};


#endif	// SIGNER_ADAPTER_H
