/*

***********************************************************************************
* Copyright (C) 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef BS_SERVER_ADAPTER_H
#define BS_SERVER_ADAPTER_H

#include "ApplicationSettings.h"
#include "BsClient.h"
#include "FutureValue.h"
#include "Message/Adapter.h"

namespace spdlog {
   class logger;
}
namespace Blocksettle {
   namespace Communication {
      namespace ProxyTerminalPb {
         class Response_UpdateOrders;
      }
   }
}
namespace BlockSettle {
   namespace Terminal {
      class SettingsMessage_SettingsResponse;
   }
}
class ConnectionManager;
class RequestReplyCommand;

class BsServerAdapter : public bs::message::Adapter, public BsClientCallbackTarget
{
public:
   BsServerAdapter(const std::shared_ptr<spdlog::logger> &);
   ~BsServerAdapter() override = default;

   bool process(const bs::message::Envelope &) override;

   std::set<std::shared_ptr<bs::message::User>> supportedReceivers() const override {
      return { user_ };
   }
   std::string name() const override { return "BS Servers"; }

private:
   void start();
   bool processOwnRequest(const bs::message::Envelope &);
   bool processLocalSettings(const BlockSettle::Terminal::SettingsMessage_SettingsResponse &);
   bool processPuBKeyResponse(bool);
   bool processTimeout(const std::string& id);
   bool processOpenConnection();
   bool processStartLogin(const std::string&);
   bool processCancelLogin();
   bool processSubmitAuthAddr(const bs::message::Envelope&, const std::string &addr);
   void processUpdateOrders(const Blocksettle::Communication::ProxyTerminalPb::Response_UpdateOrders&);

   //BCT callbacks
   void startTimer(std::chrono::milliseconds timeout, const std::function<void()>&) override;
   void onStartLoginDone(bool success, const std::string& errorMsg) override;
   void onGetLoginResultDone(const BsClientLoginResult& result) override;
//   void onAuthorizeDone(AuthorizeError authErr, const std::string& email) override;
   void onCelerRecv(CelerAPI::CelerMessageType messageType, const std::string& data) override;
   void onProcessPbMessage(const Blocksettle::Communication::ProxyTerminalPb::Response&) override;
   void Connected() override;
   void Disconnected() override;
   void onConnectionFailed() override;
/*   void onEmailHashReceived(const std::string& email, const std::string& hash) override;
   void onBootstrapDataUpdated(const std::string& data) override;
   void onAccountStateChanged(bs::network::UserType userType, bool enabled) override;
   void onFeeRateReceived(float feeRate) override;
   void onBalanceLoaded() override;*/
   void onBalanceUpdated(const std::string& currency, double balance) override;
   void onTradingStatusChanged(bool tradingEnabled) override;

private:
   std::shared_ptr<spdlog::logger>     logger_;
   std::shared_ptr<bs::message::User>  user_;
   std::shared_ptr<ConnectionManager>  connMgr_;
   std::unique_ptr<BsClient>           bsClient_;
   ApplicationSettings::EnvConfiguration  envConfig_{ ApplicationSettings::EnvConfiguration::Unknown };
   bool  connected_{ false };
   std::string currentLogin_;

   std::shared_ptr<FutureValue<bool>>     futPuBkey_;
   std::unordered_map<std::string, std::function<void()>>   timeouts_;
};


#endif	// BS_SERVER_ADAPTER_H
