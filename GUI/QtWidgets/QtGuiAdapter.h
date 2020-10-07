/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef QT_GUI_ADAPTER_H
#define QT_GUI_ADAPTER_H

#include <set>
#include <QObject>
#include "Address.h"
#include "ApiAdapter.h"
#include "SignContainer.h"
#include "ThreadSafeClasses.h"
#include "UiUtils.h"

namespace bs {
   namespace gui {
      namespace qt {
         class MainWindow;
      }
   }
}
namespace BlockSettle {
   namespace Common {
      class ArmoryMessage_AddressHistory;
      class ArmoryMessage_FeeLevelsResponse;
      class ArmoryMessage_LedgerEntries;
      class ArmoryMessage_ZCInvalidated;
      class ArmoryMessage_ZCReceived;
      class SignerMessage_SignTxResponse;
      class WalletsMessage_TXDetailsResponse;
      class WalletsMessage_UtxoListResponse;
      class WalletsMessage_WalletBalances;
      class WalletsMessage_WalletsListResponse;
   }
   namespace Terminal {
      class BsServerMessage_LoginResult;
      class BsServerMessage_StartLoginResult;
      class MatchingMessage_LoggedIn;
      class SettingsMessage_ArmoryServers;
      class SettingsMessage_SettingsResponse;
      class SettingsMessage_SignerServers;
   }
}
class BSTerminalSplashScreen;
class GuiThread;


using GuiQueue = ArmoryThreading::TimedQueue<bs::message::Envelope>;

class QtGuiAdapter : public QObject, public ApiBusAdapter, public bs::MainLoopRuner
{
   Q_OBJECT
   friend class GuiThread;
public:
   QtGuiAdapter(const std::shared_ptr<spdlog::logger> &);
   ~QtGuiAdapter() override;

   bool process(const bs::message::Envelope &) override;

   std::set<std::shared_ptr<bs::message::User>> supportedReceivers() const override {
      return { user_ };
   }
   std::string name() const override { return "QtGUI"; }

   void run(int &argc, char **argv) override;

private:
   bool processSettings(const bs::message::Envelope &);
   bool processSettingsGetResponse(const BlockSettle::Terminal::SettingsMessage_SettingsResponse&);
   bool processSettingsState(const BlockSettle::Terminal::SettingsMessage_SettingsResponse&);
   bool processArmoryServers(const BlockSettle::Terminal::SettingsMessage_ArmoryServers&);
   bool processSignerServers(const BlockSettle::Terminal::SettingsMessage_SignerServers&);
   bool processAdminMessage(const bs::message::Envelope &);
   bool processBlockchain(const bs::message::Envelope &);
   bool processSigner(const bs::message::Envelope &);
   bool processWallets(const bs::message::Envelope &);
   bool processAuthEid(const bs::message::Envelope &);
   bool processOnChainTrack(const bs::message::Envelope &);

   void requestInitialSettings();
   void updateSplashProgress();
   void splashProgressCompleted();
   void updateStates();

   void createWallet(bool primary);
   void makeMainWinConnections();

   void processWalletLoaded(const bs::sync::WalletInfo &);
   bool processWalletBalances(const bs::message::Envelope &
      , const BlockSettle::Common::WalletsMessage_WalletBalances &);
   bool processTXDetails(uint64_t msgId, const BlockSettle::Common::WalletsMessage_TXDetailsResponse &);
   bool processLedgerEntries(const BlockSettle::Common::ArmoryMessage_LedgerEntries &);
   bool processAddressHist(const BlockSettle::Common::ArmoryMessage_AddressHistory&);
   bool processFeeLevels(const BlockSettle::Common::ArmoryMessage_FeeLevelsResponse&);
   bool processWalletsList(const BlockSettle::Common::WalletsMessage_WalletsListResponse&);
   bool processUTXOs(const BlockSettle::Common::WalletsMessage_UtxoListResponse&);
   bool processSignTX(const BlockSettle::Common::SignerMessage_SignTxResponse&);
   bool processZC(const BlockSettle::Common::ArmoryMessage_ZCReceived&);
   bool processZCInvalidated(const BlockSettle::Common::ArmoryMessage_ZCInvalidated&);

   bool processBsServer(const bs::message::Envelope&);
   bool processStartLogin(const BlockSettle::Terminal::BsServerMessage_StartLoginResult&);
   bool processLogin(const BlockSettle::Terminal::BsServerMessage_LoginResult&);

   bool processMatching(const bs::message::Envelope&);

private slots:
   void onGetSettings(const std::vector<ApplicationSettings::Setting>&);
   void onPutSetting(ApplicationSettings::Setting, const QVariant &value);
   void onResetSettings(const std::vector<ApplicationSettings::Setting>&);
   void onResetSettingsToState(const ApplicationSettings::State&);
   void onNeedSettingsState();
   void onNeedArmoryServers();
   void onSetArmoryServer(int);
   void onAddArmoryServer(const ArmoryServer&);
   void onDelArmoryServer(int);
   void onUpdArmoryServer(int, const ArmoryServer&);
   void onNeedArmoryReconnect();
   void onNeedSigners();
   void onSetSigner(int);
   void onNeedHDWalletDetails(const std::string &walletId);
   void onNeedWalletBalances(const std::string &walletId);
   void onNeedExtAddresses(const std::string &walletId);
   void onNeedIntAddresses(const std::string &walletId);
   void onNeedUsedAddresses(const std::string &walletId);
   void onNeedAddrComments(const std::string &walletId, const std::vector<bs::Address> &);
   void onSetAddrComment(const std::string &walletId, const bs::Address &
      , const std::string &comment);
   void onNeedLedgerEntries(const std::string &filter);
   void onNeedTXDetails(const std::vector<bs::sync::TXWallet> &, bool useCache
      , const bs::Address &);
   void onNeedAddressHistory(const bs::Address&);
   void onNeedWalletsList(UiUtils::WalletsTypes, const std::string &id);
   void onNeedFeeLevels(const std::vector<unsigned int>&);
   void onNeedUTXOs(const std::string& id, const std::string& walletId
      , bool confOnly, bool swOnly);
   void onNeedSignTX(const std::string& id, const bs::core::wallet::TXSignRequest&
      , bool keepDupRecips, SignContainer::TXSignMode);
   void onNeedBroadcastZC(const std::string& id, const BinaryData&);
   void onNeedSetTxComment(const std::string& walletId, const BinaryData& txHash
      , const std::string& comment);
   void onNeedOpenBsConnection();
   void onNeedCloseBsConnection();
   void onNeedStartLogin(const std::string& login);
   void onNeedCancelLogin();
   void onBootstrapDataLoaded(const std::string&);
   void onNeedMatchingLogin(const std::string& mtchLogin, const std::string& bsLogin);
   void onNeedMatchingLogout();
   void onNeedSetUserId(const std::string&);
   void onSetRecommendedFeeRate(float);

private:
   std::shared_ptr<spdlog::logger>        logger_;
   std::shared_ptr<bs::message::UserTerminal>   userSettings_, userWallets_;
   std::shared_ptr<bs::message::UserTerminal>   userBlockchain_, userSigner_;
   std::shared_ptr<bs::message::UserTerminal>   userBS_, userMatch_;
   bs::gui::qt::MainWindow * mainWindow_{ nullptr };
   BSTerminalSplashScreen  * splashScreen_{ nullptr };

   std::set<int>  createdComponents_;
   std::set<int>  loadingComponents_;
   int         armoryState_{ -1 };
   uint32_t    blockNum_{ 0 };
   int         signerState_{ -1 };
   std::string signerDetails_;
   bool  walletsReady_{ false };

   std::unordered_map<std::string, bs::sync::WalletInfo> hdWallets_;
   std::set<uint64_t>   newZCs_;
};


#endif	// QT_GUI_ADAPTER_H
