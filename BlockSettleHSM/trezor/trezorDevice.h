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

class TrezorDevice : public QObject
{
   Q_OBJECT

public:
   TrezorDevice(const std::shared_ptr<ConnectionManager> &, bool testNet
      , const QPointer<TrezorClient> &, QObject* parent = nullptr);
   ~TrezorDevice() override;

   DeviceKey deviceKey() const;

   void init(AsyncCallBack&& cb = nullptr);
   void getPublicKey(AsyncCallBackCall&& cb = nullptr);
   void setMatrixPin(const std::string& pin);
   void cancel();

   void signTX(int outputCount, int inputCount, AsyncCallBackCall&& cb = nullptr);

signals:
   void publicKeyReady();
   void requestPinMatrix();
   void requestHSMPass();

private:
   void makeCall(const google::protobuf::Message &msg);

   void handleMessage(const MessageData& data);
   bool parseResponse(google::protobuf::Message &msg, const MessageData& data);

   // callbacks
   void resetCallbacks();

   void setCallbackNoData(hw::trezor::messages::MessageType, AsyncCallBack&& cb);
   void callbackNoData(hw::trezor::messages::MessageType);

   void setDataCallback(hw::trezor::messages::MessageType, AsyncCallBackCall&& cb);
   void dataCallback(hw::trezor::messages::MessageType, QByteArray&& response);

private:
   std::shared_ptr<ConnectionManager> connectionManager_{};
   QPointer<TrezorClient> client_{};
   hw::trezor::messages::management::Features features_{};
   bool testNet_{};

   std::unordered_map<int, AsyncCallBack> awaitingCallbackNoData_;
   std::unordered_map<int, AsyncCallBackCall> awaitingCallbackData_;
};

#endif // TREZORDEVICE_H
