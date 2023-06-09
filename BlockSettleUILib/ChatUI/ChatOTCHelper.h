/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef CHATOTCHELPER_H
#define CHATOTCHELPER_H

#include <QObject>
#include <set>
#include "OtcTypes.h"
#include "ChatProtocol/Message.h"
#include "ChatProtocol/ClientParty.h"

namespace spdlog {
   class logger;
}

namespace bs {
   namespace sync {
      class WalletsManager;
   }
   namespace network {
      namespace otc {
         enum class Env : int;
         struct Offer;
         struct Peer;
         struct PeerId;
         struct QuoteRequest;
         struct QuoteResponse;
      }
   }
   class UTXOReservationManager;
}

namespace Blocksettle {
   namespace Communication {
      namespace ProxyTerminalPb {
         class Response;
      }
   }
}

class ApplicationSettings;
class ArmoryConnection;
class AuthAddressManager;
class OtcClient;
class WalletSignerContainer;

class ChatOTCHelper : public QObject {
   Q_OBJECT
public:
   ChatOTCHelper(QObject* parent = nullptr);
   ~ChatOTCHelper() override = default;

   void init(bs::network::otc::Env env
      , const std::shared_ptr<spdlog::logger>& loggerPtr
      , const std::shared_ptr<bs::sync::WalletsManager>& walletsMgr
      , const std::shared_ptr<ArmoryConnection>& armory
      , const std::shared_ptr<WalletSignerContainer>& signContainer
      , const std::shared_ptr<AuthAddressManager> &authAddressManager
      , const std::shared_ptr<bs::UTXOReservationManager> &utxoReservationManager
      , const std::shared_ptr<ApplicationSettings>& applicationSettings);

   OtcClient* client() const;

   void setCurrentUserId(const std::string& ownUserId);

   void setGlobalOTCEntryTimeStamp(QDateTime timeStamp);
   QDateTime selectedGlobalOTCEntryTimeStamp() const;

public slots:
   void onLogout();
   void onProcessOtcPbMessage(const Blocksettle::Communication::ProxyTerminalPb::Response &response);

   void onOtcRequestSubmit(const bs::network::otc::PeerPtr &peer, const bs::network::otc::Offer& offer);
   void onOtcPullOrReject(const bs::network::otc::PeerPtr &peer);
   void onOtcResponseAccept(const bs::network::otc::PeerPtr &peer, const bs::network::otc::Offer& offer);
   void onOtcResponseUpdate(const bs::network::otc::PeerPtr &peer, const bs::network::otc::Offer& offer);
   void onOtcResponseReject(const bs::network::otc::PeerPtr &peer);

   void onOtcQuoteRequestSubmit(const bs::network::otc::QuoteRequest &request);
   void onOtcQuoteResponseSubmit(const bs::network::otc::PeerPtr &peer, const bs::network::otc::QuoteResponse &response);

   void onMessageArrived(const Chat::MessagePtrList& messagePtr);
   void onPartyStateChanged(const Chat::ClientPartyPtr& clientPartyPtr);

private:
   OtcClient* otcClient_{};
   std::set<std::string> connectedContacts_;
   std::shared_ptr<spdlog::logger> loggerPtr_;
   QDateTime selectedGlobalEntryTimeStamp_{};
};

#endif // CHATOTCHELPER_H
