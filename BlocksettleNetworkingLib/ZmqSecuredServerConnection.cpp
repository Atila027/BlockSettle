#include "ZmqSecuredServerConnection.h"
#include "ZMQHelperFunctions.h"

#include "FastLock.h"
#include "MessageHolder.h"

#include <zmq.h>
#include <spdlog/spdlog.h>


ZmqSecuredServerConnection::ZmqSecuredServerConnection(const std::shared_ptr<spdlog::logger>& logger
      , const std::shared_ptr<ZmqContext>& context)
 : ZmqServerConnection(logger, context)
{}

bool ZmqSecuredServerConnection::SetKeyPair(const std::string& publicKey, const std::string& privateKey)
{
   if (publicKey.length() != 40) {
      logger_->error("[ZmqSecuredServerConnection::SetKeyPair] invalid public key length: {}"
         , publicKey.length());
      return false;
   }

   if (privateKey.length() != 40) {
      logger_->error("[ZmqSecuredServerConnection::SetKeyPair] invalid private key length: {}"
         , privateKey.length());
      return false;
   }

   publicKey_ = publicKey;
   privateKey_ = privateKey;

   return true;
}

ZmqContext::sock_ptr ZmqSecuredServerConnection::CreateDataSocket()
{
   return context_->CreateServerSocket();
}

bool ZmqSecuredServerConnection::ConfigDataSocket(const ZmqContext::sock_ptr& dataSocket)
{
   if (publicKey_.empty() || privateKey_.empty()) {
      logger_->error("[ZmqSecuredServerConnection::ConfigDataSocket] missing key pair for {}"
         , connectionName_);
      return false;
   }

   int isServer = 1;
   int result = zmq_setsockopt (dataSocket.get(), ZMQ_CURVE_SERVER, &isServer, sizeof(isServer));
   if (result != 0) {
      logger_->error("[ZmqSecuredServerConnection::ConfigDataSocket] {} failed to config socket to be a server : {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   result = zmq_setsockopt (dataSocket.get(), ZMQ_CURVE_SECRETKEY, privateKey_.c_str(), 41);
   if (result != 0) {
      logger_->error("[ZmqSecuredServerConnection::ConfigDataSocket] {} failed to set server private key: {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   std::string serverIdentity = "bs_public_bridge";
   result = zmq_setsockopt (dataSocket.get(), ZMQ_IDENTITY
      , serverIdentity.c_str(), serverIdentity.size());
   if (result != 0) {
      logger_->error("[ZmqSecuredServerConnection::ConfigDataSocket] {} failed to set server identity {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   int lingerPeriod = 0;
   result = zmq_setsockopt (dataSocket.get(), ZMQ_LINGER, &lingerPeriod, sizeof(lingerPeriod));
   if (result != 0) {
      logger_->error("[ZmqSecuredServerConnection::ConfigDataSocket] {} failed to set linger interval: {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   return true;
}

bool ZmqSecuredServerConnection::ReadFromDataSocket()
{
   MessageHolder clientId;
   MessageHolder data;

   int result = zmq_msg_recv(&clientId, dataSocket_.get(), ZMQ_DONTWAIT);
   if (result == -1) {
      logger_->error("[ZmqSecuredServerConnection::ReadFromDataSocket] {} failed to recv header: {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   const auto &clientIdStr = clientId.ToString();
   if (clientInfo_.find(clientIdStr) == clientInfo_.end()) {
#ifdef WIN32
      SOCKET socket = 0;
#else
      int socket = 0;
#endif
      size_t sockSize = sizeof(socket);
      if (zmq_getsockopt(dataSocket_.get(), ZMQ_FD, &socket, &sockSize) == 0) {
         clientInfo_[clientIdStr] = bs::network::peerAddressString(static_cast<int>(socket));
      }
   }

   result = zmq_msg_recv(&data, dataSocket_.get(), ZMQ_DONTWAIT);
   if (result == -1) {
      logger_->error("[ZmqSecuredServerConnection::ReadFromDataSocket] {} failed to recv message data: {}"
         , connectionName_, zmq_strerror(zmq_errno()));
      return false;
   }

   if (!data.IsLast()) {
      logger_->error("[ZmqSecuredServerConnection::ReadFromDataSocket] {} broken protocol"
         , connectionName_);
      return false;
   }

   notifyListenerOnData(clientIdStr, data.ToString());
   return true;
}

bool ZmqSecuredServerConnection::SendDataToClient(const std::string& clientId, const std::string& data)
{
   return QueueDataToSend(clientId, data, false);
}
