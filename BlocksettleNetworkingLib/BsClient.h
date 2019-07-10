#ifndef BS_CLIENT_H
#define BS_CLIENT_H

#include <string>
#include <map>
#include <memory>
#include <functional>
#include <future>
#include <unordered_set>
#include <spdlog/logger.h>
#include <QObject>
#include "DataConnectionListener.h"
#include "autheid_utils.h"

class ZmqContext;
class ZmqBIP15XDataConnection;

namespace Blocksettle { namespace Communication { namespace Proxy {
class Request;
class Response_StartLogin;
class Response_CancelLogin;
class Response_GetLoginResult;
class Response_Logout;
} } }

struct BsClientParams
{
   struct NewKey
   {
      std::string oldKey;
      std::string newKey;
      std::shared_ptr<std::promise<bool>> prompt;
   };

   using NewKeyCallback = std::function<void(const NewKey &newKey)>;

   std::shared_ptr<ZmqContext> context;

   std::string connectAddress{"127.0.0.1"};
   int connectPort{10259};

   std::string oldServerKey;

   NewKeyCallback newServerKeyCallback;
};

class BsClient : public QObject, public DataConnectionListener
{
   Q_OBJECT
public:
   BsClient(const std::shared_ptr<spdlog::logger>& logger, const BsClientParams &params
      , QObject *parent = nullptr);
   ~BsClient() override;

   const BsClientParams &params() const { return params_; }

   const autheid::PrivateKey &ephemeralPrivKey() const;

   void startLogin(const std::string &login);
   void cancelLogin();
   void getLoginResult();
   void logout();
signals:
   void startLoginDone(bool success);
   void getLoginResultDone(bool success);
   void logoutDone();

   void connected();
   void connectionFailed();
private:
   using FailedCallback = std::function<void()>;

   struct ActiveRequest
   {
      int64_t requestId{};
      FailedCallback failedCb;
   };

   void timerEvent(QTimerEvent *event) override;

   // From DataConnectionListener
   void OnDataReceived(const std::string& data) override;
   void OnConnected() override;
   void OnDisconnected() override;
   void OnError(DataConnectionError errorCode) override;

   void sendRequest(Blocksettle::Communication::Proxy::Request *request, std::chrono::milliseconds timeout
      , FailedCallback failedCb);

   void process(const Blocksettle::Communication::Proxy::Response_StartLogin &response);
   void process(const Blocksettle::Communication::Proxy::Response_CancelLogin &response);
   void process(const Blocksettle::Communication::Proxy::Response_GetLoginResult &response);
   void process(const Blocksettle::Communication::Proxy::Response_Logout &response);

   int64_t newRequestId();

   std::shared_ptr<spdlog::logger> logger_;

   BsClientParams params_;

   std::unique_ptr<ZmqBIP15XDataConnection> connection_;

   int64_t lastRequestId_{};
   FailedCallback failedCallback_;

   std::unordered_set<int64_t> activeRequestIds_;
   std::map<std::chrono::steady_clock::time_point, ActiveRequest> activeRequests_;
};

#endif
