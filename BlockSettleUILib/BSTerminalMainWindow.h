#ifndef __BS_TERMINAL_MAIN_WINDOW_H__
#define __BS_TERMINAL_MAIN_WINDOW_H__

#include <QMainWindow>
#include <QStandardItemModel>

#include <memory>
#include <vector>

#include "ApplicationSettings.h"
#include "ArmoryConnection.h"
#include "CelerClient.h"
#include "QWalletInfo.h"
#include "SignContainer.h"

namespace Ui {
    class BSTerminalMainWindow;
}
namespace bs {
   class LogManager;
   namespace sync {
      class Wallet;
      class WalletsManager;
   }
}

class AboutDialog;
class AssetManager;
class AuthAddressDialog;
class AuthAddressManager;
class AuthSignManager;
class AutheIDClient;
class BSTerminalSplashScreen;
class CCFileManager;
class CCPortfolioModel;
class CelerClient;
class ConnectionManager;
class CelerMarketDataProvider;
class OfflineSigner;
class QSystemTrayIcon;
class RequestReplyCommand;
class StatusBarView;
class StatusViewBlockListener;
class TransactionsViewModel;
class WalletManagementWizard;
class ArmoryServersProvider;

class BSTerminalMainWindow : public QMainWindow
{
Q_OBJECT

public:
   BSTerminalMainWindow(const std::shared_ptr<ApplicationSettings>& settings
      , BSTerminalSplashScreen& splashScreen, QWidget* parent = nullptr);
   ~BSTerminalMainWindow() override;

   void postSplashscreenActions();

private:
   void setupToolbar();
   void setupMenu();
   void setupIcon();

   void setupWalletsView();
   void setupTransactionsView();

   void InitConnections();
   void initArmory();
   void connectArmory();
   void connectSigner();
   std::shared_ptr<SignContainer> createSigner();

   void setTabStyle();

   void LoadWallets();
   void InitAuthManager();
   bool InitSigningContainer();
   void InitAssets();

   void InitPortfolioView();
   void InitWalletsView();
   void InitChatView();

   void InitOTP();

   void UpdateMainWindowAppearence();

   bool isMDLicenseAccepted() const;
   void saveUserAcceptedMDLicense();

   bool showStartupDialog();
   void LoadCCDefinitionsFromPuB();

signals:
   void readyToLogin();
   void armoryServerPromptResultReady();

private slots:
   // display login dialog once network settings loaded
   void onReadyToLogin();

   void InitTransactionsView();
   void ArmoryIsOffline();
   void SignerReady();
   void onPasswordRequested(const bs::hd::WalletInfo &walletInfo, std::string prompt);
   void showInfo(const QString &title, const QString &text);
   void showError(const QString &title, const QString &text);
   void onSignerConnError(const QString &);

   void CompleteUIOnlineView();
   void CompleteDBConnection();

   bool createWallet(bool primary, bool reportSuccess = true);

   void acceptMDAgreement();
   void updateControlEnabledState();
   void onButtonUserClicked();
   void showArmoryServerPrompt(const BinaryData& srvPubKey, const std::string& srvIPPort, std::shared_ptr<std::promise<bool> > promiseObj);

   void onArmoryNeedsReconnect();

private:
   std::unique_ptr<Ui::BSTerminalMainWindow> ui_;
   QAction *action_send_;
   QAction *action_receive_;
   QAction *action_login_;
   QAction *action_logout_;

   std::shared_ptr<bs::LogManager>        logMgr_;
   std::shared_ptr<ApplicationSettings>   applicationSettings_;
   std::shared_ptr<bs::sync::WalletsManager> walletsMgr_;
   std::shared_ptr<ArmoryServersProvider> armoryServersProvider_;
   std::shared_ptr<AuthAddressManager>    authManager_;
   std::shared_ptr<AuthSignManager>       authSignManager_;
   std::shared_ptr<ArmoryConnection>      armory_;

   std::shared_ptr<RequestReplyCommand>   cmdPuBSettings_;

   std::shared_ptr<StatusBarView>            statusBarView_;
   std::shared_ptr<QSystemTrayIcon>          sysTrayIcon_;
   std::shared_ptr<TransactionsViewModel>    transactionsModel_;
   std::shared_ptr<CCPortfolioModel>         portfolioModel_;
   std::shared_ptr<ConnectionManager>        connectionManager_;
   std::shared_ptr<CelerClient>              celerConnection_;
   std::shared_ptr<CelerMarketDataProvider>  mdProvider_;
   std::shared_ptr<AssetManager>             assetManager_;
   std::shared_ptr<CCFileManager>            ccFileManager_;
   std::shared_ptr<AuthAddressDialog>        authAddrDlg_;
   std::shared_ptr<AboutDialog>              aboutDlg_;
   std::shared_ptr<SignContainer>            signContainer_;

   std::shared_ptr<WalletManagementWizard> walletsWizard_;

   QString currentUserLogin_;
   bool  widgetsInited_ = false;

   struct NetworkSettings {
      struct Connection {
         std::string host;
         uint32_t    port;
      };
      Connection  celer;
      Connection  marketData;
      Connection  mdhs;
      Connection  chat;
      bool        isSet = false;
   };
   void GetNetworkSettingsFromPuB(const std::function<void()> &);
   void OnNetworkSettingsLoaded();

public slots:
   void onReactivate();

private:
   struct TxInfo;

private slots:
   void onSend();
   void onReceive();

   void openAuthManagerDialog();
   void openAuthDlgVerify(const QString &addrToVerify);
   void openConfigDialog();
   void openAccountInfoDialog();
   void openCCTokenDialog();
   void onZCreceived(const std::vector<bs::TXEntry>);
   void onArmoryStateChanged(ArmoryConnection::State);

   void showZcNotification(const TxInfo *);

   void onLogin();
   void onLogout();

   void onCelerConnected();
   void onCelerDisconnected();
   void onCelerConnectionError(int errorCode);
   void showRunInBackgroundMessage();
   void onAuthMgrConnComplete();
   void onCCInfoMissing();

   void onMDConnectionDetailsRequired();

protected:
   void closeEvent(QCloseEvent* event) override;
   void changeEvent(QEvent* e) override;

private:
   void onUserLoggedIn();
   void onUserLoggedOut();

   void setLoginButtonText(const QString& text);

   void setupShortcuts();

   void createAdvancedTxDialog(const std::string &selectedWalletId);
   void createAuthWallet();

   bool isUserLoggedIn() const;
   bool isArmoryConnected() const;

   void loginWithCeler(const std::string& username, const std::string& password);
   void loginToCeler(const std::string& username, const std::string& password);

private:
   QString           loginButtonText_;
   NetworkSettings   networkSettings_;
   bool  readyToRegisterWallets_ = false;
};

#endif // __BS_TERMINAL_MAIN_WINDOW_H__
