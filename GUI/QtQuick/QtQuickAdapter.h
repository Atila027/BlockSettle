/*

***********************************************************************************
* Copyright (C) 2020 - 2022, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef QT_QUICK_ADAPTER_H
#define QT_QUICK_ADAPTER_H

#include <set>
#include <QObject>
#include "Address.h"
#include "AddressListModel.h"
#include "ApiAdapter.h"
#include "Wallets/SignContainer.h"
#include "ThreadSafeClasses.h"
#include "TxListModel.h"
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
      class ArmoryMessage_ZCInvalidated;
      class ArmoryMessage_ZCReceived;
      class LedgerEntries;
      class OnChainTrackMessage_AuthAddresses;
      class OnChainTrackMessage_AuthState;
      class SignerMessage_SignTxResponse;
      class WalletsMessage_AuthKey;
      class WalletsMessage_ReservedUTXOs;
      class WalletsMessage_TXDetailsResponse;
      class WalletsMessage_UtxoListResponse;
      class WalletsMessage_WalletBalances;
      class WalletsMessage_WalletData;
      class WalletsMessage_WalletsListResponse;
   }
   namespace Terminal {
      class AssetsMessage_Balance;
      class AssetsMessage_SubmittedAuthAddresses;
      class BsServerMessage_LoginResult;
      class BsServerMessage_Orders;
      class BsServerMessage_StartLoginResult;
      class MatchingMessage_LoggedIn;
      class Quote;
      class QuoteCancelled;
      class IncomingRFQ;
      class MatchingMessage_Order;
      class MktDataMessage_Prices;
      class SettlementMessage_FailedSettlement;
      class SettlementMessage_MatchedQuote;
      class SettlementMessage_PendingSettlement;
      class SettlementMessage_SettlementIds;
      class SettingsMessage_ArmoryServers;
      class SettingsMessage_SettingsResponse;
      class SettingsMessage_SignerServers;
   }
}
class BSTerminalSplashScreen;
class QQmlContext;
class QmlWalletsList;


class QtQuickAdapter : public QObject, public ApiBusAdapter, public bs::MainLoopRuner
{
   Q_OBJECT
   friend class GuiThread;
public:
   QtQuickAdapter(const std::shared_ptr<spdlog::logger> &);
   ~QtQuickAdapter() override;

   bs::message::ProcessingResult process(const bs::message::Envelope &) override;
   bool processBroadcast(const bs::message::Envelope&) override;

   Users supportedReceivers() const override { return { user_ }; }
   std::string name() const override { return "QtQuick"; }

   void run(int &argc, char **argv) override;

   Q_PROPERTY(QStringList walletsList READ walletsList NOTIFY walletsListChanged)
   QStringList walletsList() const { return walletsList_; }
   Q_PROPERTY(QStringList txWalletsList READ txWalletsList NOTIFY walletsListChanged)
   QStringList txWalletsList() const;
   Q_PROPERTY(QStringList txTypesList READ txTypesList)
   QStringList txTypesList() const { return txTypes_; }

   Q_PROPERTY(QString confirmedBalance READ confirmedBalance NOTIFY walletBalanceChanged)
   QString confirmedBalance() const { return QString::number(confWalletBalance_, 'f', 8); }
   Q_PROPERTY(QString unconfirmedBalance READ unconfirmedBalance NOTIFY walletBalanceChanged)
   QString unconfirmedBalance() const { return QString::number(unconfWalletBalance_, 'f', 8); }
   Q_PROPERTY(QString totalBalance READ totalBalance NOTIFY walletBalanceChanged)
   QString totalBalance() const { return QString::number(totalWalletBalance_, 'f', 8); }
   Q_PROPERTY(quint32 nbUsedWalletAddresses READ nbUsedWalletAddresses NOTIFY walletBalanceChanged)
   quint32 nbUsedWalletAddresses() const { return nbUsedWalletAddresses_; }

   Q_PROPERTY(quint32 nbTransactions READ nbTransactions NOTIFY nbTransactionsChanged)
   quint32 nbTransactions() const { return nbTransactions_; }

   Q_PROPERTY(QString generatedAddress READ generatedAddress NOTIFY addressGenerated)
   QString generatedAddress() const { return QString::fromStdString(generatedAddress_.display()); }

   Q_INVOKABLE QStringList newSeedPhrase();
   Q_INVOKABLE void copySeedToClipboard(const QStringList&);
   Q_INVOKABLE void createWallet(const QString& name, const QStringList& seed
      , const QString& password);
   Q_INVOKABLE void importWallet(const QString& name, const QStringList& seed
      , const QString& password);
   Q_INVOKABLE void generateNewAddress(int walletIndex, bool isNative);
   Q_INVOKABLE void copyAddressToClipboard(const QString& addr);

signals:
   void walletsListChanged();
   void walletBalanceChanged();
   void nbTransactionsChanged();
   void addressGenerated();

private slots:
   void walletSelected(int);

private:
   bs::message::ProcessingResult processSettings(const bs::message::Envelope &);
   bs::message::ProcessingResult processSettingsGetResponse(const BlockSettle::Terminal::SettingsMessage_SettingsResponse&);
   bs::message::ProcessingResult processSettingsState(const BlockSettle::Terminal::SettingsMessage_SettingsResponse&);
   bs::message::ProcessingResult processArmoryServers(const BlockSettle::Terminal::SettingsMessage_ArmoryServers&);
   bs::message::ProcessingResult processAdminMessage(const bs::message::Envelope &);
   bs::message::ProcessingResult processBlockchain(const bs::message::Envelope &);
   bs::message::ProcessingResult processSigner(const bs::message::Envelope &);
   bs::message::ProcessingResult processWallets(const bs::message::Envelope &);

   void requestInitialSettings();
   void updateSplashProgress();
   void splashProgressCompleted();
   void updateStates();

   void createWallet(bool primary);
   std::string hdWalletIdByIndex(int);

   void processWalletLoaded(const bs::sync::WalletInfo &);
   bs::message::ProcessingResult processWalletData(const uint64_t msgId
      , const BlockSettle::Common::WalletsMessage_WalletData&);
   bs::message::ProcessingResult processWalletBalances(const BlockSettle::Common::WalletsMessage_WalletBalances &);
   bs::message::ProcessingResult processTXDetails(uint64_t msgId, const BlockSettle::Common::WalletsMessage_TXDetailsResponse &);
   bs::message::ProcessingResult processLedgerEntries(const BlockSettle::Common::LedgerEntries &);
   bs::message::ProcessingResult processAddressHist(const BlockSettle::Common::ArmoryMessage_AddressHistory&);
   bs::message::ProcessingResult processFeeLevels(const BlockSettle::Common::ArmoryMessage_FeeLevelsResponse&);
   bs::message::ProcessingResult processWalletsList(const BlockSettle::Common::WalletsMessage_WalletsListResponse&);
   bs::message::ProcessingResult processUTXOs(const BlockSettle::Common::WalletsMessage_UtxoListResponse&);
   bs::message::ProcessingResult processSignTX(const BlockSettle::Common::SignerMessage_SignTxResponse&);
   bs::message::ProcessingResult processZC(const BlockSettle::Common::ArmoryMessage_ZCReceived&);
   bs::message::ProcessingResult processZCInvalidated(const BlockSettle::Common::ArmoryMessage_ZCInvalidated&);
   bs::message::ProcessingResult processReservedUTXOs(const BlockSettle::Common::WalletsMessage_ReservedUTXOs&);
   void processWalletAddresses(const std::vector<bs::sync::Address>&);

private:
   std::shared_ptr<spdlog::logger>        logger_;
   BSTerminalSplashScreen* splashScreen_{ nullptr };
   QObject* rootObj_{ nullptr };
   QQmlContext* rootCtxt_{nullptr};
   std::shared_ptr<bs::message::UserTerminal>   userSettings_, userWallets_;
   std::shared_ptr<bs::message::UserTerminal>   userBlockchain_, userSigner_;
   bool loadingDone_{ false };

   std::recursive_mutex mutex_;
   std::set<int>  createdComponents_;
   std::set<int>  loadingComponents_;
   int         armoryState_{ -1 };
   uint32_t    blockNum_{ 0 };
   int         signerState_{ -1 };
   std::string signerDetails_;
   bool  walletsReady_{ false };

   std::unordered_map<std::string, bs::sync::WalletInfo> hdWallets_;
   std::unordered_map<std::string, std::string> walletNames_;
   std::map<bs::message::SeqId, std::string> walletInfoReq_;
   std::map<bs::Address, std::string>  addrComments_;
   std::set<uint64_t>   newZCs_;

   QStringList walletsList_;
   const QStringList txTypes_;
   unsigned nbUsedWalletAddresses_{ 0 };
   double confWalletBalance_{ 0 }, unconfWalletBalance_{ 0 }, totalWalletBalance_{ 0 };
   unsigned nbTransactions_{ 0 };
   QmlAddressListModel* addrModel_{ nullptr };
   TxListModel* pendingTxModel_{ nullptr };
   TxListModel* txModel_{ nullptr };
   bs::Address generatedAddress_;
};

#endif	// QT_QUICK_ADAPTER_H