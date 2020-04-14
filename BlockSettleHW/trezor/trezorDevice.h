/*

***********************************************************************************
* Copyright (C) 2016 - , BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef TREZORDEVICE_H
#define TREZORDEVICE_H

#include "trezorStructure.h"
#include "hwdeviceinterface.h"
#include <QObject>
#include <QNetworkReply>
#include <QPointer>


// Trezor interface (source - https://github.com/trezor/trezor-common/tree/master/protob)
#include "trezor/generated_proto/messages-management.pb.h"
#include "trezor/generated_proto/messages-common.pb.h"
#include "trezor/generated_proto/messages-bitcoin.pb.h"
#include "trezor/generated_proto/messages.pb.h"


class ConnectionManager;
class QNetworkRequest;
class TrezorClient;

namespace bs {
   namespace core {
      namespace wallet {
         struct TXSignRequest;
      }
   }
   namespace sync {
      class WalletsManager;
   }
}

class TrezorDevice : public HwDeviceInterface
{
   Q_OBJECT

public:
   TrezorDevice(const std::shared_ptr<ConnectionManager> &, std::shared_ptr<bs::sync::WalletsManager> walletManager, bool testNet
      , const QPointer<TrezorClient> &, QObject* parent = nullptr);
   ~TrezorDevice() override;

   DeviceKey key() const override;
   DeviceType type() const override;


   // lifecycle
   void init(AsyncCallBack&& cb = nullptr) override;
   void cancel() override;

   // operation
   void getPublicKey(AsyncCallBackCall&& cb = nullptr) override;
   void signTX(const QVariant& reqTX, AsyncCallBackCall&& cb = nullptr) override;

   // Management
   void setMatrixPin(const std::string& pin) override;
   void setPassword(const std::string& password) override;

private:
   void makeCall(const google::protobuf::Message &msg);

   void handleMessage(const MessageData& data);
   bool parseResponse(google::protobuf::Message &msg, const MessageData& data);

   // callbacks
   void resetCaches();

   void setCallbackNoData(hw::trezor::messages::MessageType, AsyncCallBack&& cb);
   void callbackNoData(hw::trezor::messages::MessageType);

   void setDataCallback(hw::trezor::messages::MessageType, AsyncCallBackCall&& cb);
   void dataCallback(hw::trezor::messages::MessageType, QVariant&& response);

   void handleTxRequest(const MessageData& data);
   void sendTxMessage(const QString& status);

private:
   std::shared_ptr<ConnectionManager> connectionManager_{};
   std::shared_ptr<bs::sync::WalletsManager> walletManager_{};

   QPointer<TrezorClient> client_{};
   hw::trezor::messages::management::Features features_{};
   bool testNet_{};
   std::unique_ptr<bs::core::wallet::TXSignRequest> currentTxSignReq_;
   HWSignedTx awaitingTransaction_;
   HwWalletWrapper awaitingWalletInfo_;

   std::unordered_map<int, AsyncCallBack> awaitingCallbackNoData_;
   std::unordered_map<int, AsyncCallBackCall> awaitingCallbackData_;
};

#endif // TREZORDEVICE_H