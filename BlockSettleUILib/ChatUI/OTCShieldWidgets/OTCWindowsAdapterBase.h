/*

***********************************************************************************
* Copyright (C) 2019 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef __OTCWINDOWSMANAGER_H__
#define __OTCWINDOWSMANAGER_H__

#include <memory>
#include "CommonTypes.h"
#include "OtcTypes.h"
#include "ValidityFlag.h"
#include "UtxoReservationToken.h"

#include <QWidget>
#include <QTimer>
#include <QPointer>

class QComboBox;
class OTCWindowsManager;
class AuthAddressManager;
class AssetManager;
class QLabel;
class QProgressBar;

namespace bs {
   namespace sync {
      class WalletsManager;

      namespace hd {
         class Wallet;
      }
   }

   namespace network {
      namespace otc {
         struct Peer;
      }
   }
   class UTXOReservationManager;
}

struct TimeoutData
{
   std::chrono::steady_clock::time_point offerTimestamp_{};
   QPointer<QProgressBar> progressBarTimeLeft_{};
   QPointer<QLabel> labelTimeLeft_{};
};

using CbSuccess = std::function<void(void)>;
class OTCWindowsAdapterBase : public QWidget {
   Q_OBJECT
public:
   OTCWindowsAdapterBase(QWidget* parent = nullptr);
   ~OTCWindowsAdapterBase() override = default;

   void setChatOTCManager(const std::shared_ptr<OTCWindowsManager>& otcManager);
   std::shared_ptr<bs::sync::WalletsManager> getWalletManager() const;
   std::shared_ptr<AuthAddressManager> getAuthManager() const;
   std::shared_ptr<AssetManager> getAssetManager() const;
   std::shared_ptr<bs::UTXOReservationManager> getUtxoManager() const;

   virtual void setPeer(const bs::network::otc::Peer &);
   
   bs::UtxoReservationToken releaseReservation();
   void setReservation(bs::UtxoReservationToken&& reservation);

signals:
   
   void xbtInputsProcessed();

public slots:
   virtual void onAboutToApply() {}
   virtual void onChatRoomChanged() {}
   virtual void onParentAboutToHide() {}

protected slots:
   virtual void onSyncInterface();
   void onUpdateMD(bs::network::Asset::Type, const QString&, const bs::network::MDFields&);
   virtual void onMDUpdated();
   virtual void onUpdateBalances();
   void onUpdateTimerData();

protected:
   // Shared function between children
   void showXBTInputsClicked(QComboBox *walletsCombobox);
   
   void updateIndicativePrices(
      bs::network::Asset::Type type
      , const QString& security
      , const bs::network::MDFields& fields);
   
   BTCNumericTypes::balance_type getXBTSpendableBalanceFromCombobox(QComboBox *walletsCombobox) const;
   std::shared_ptr<bs::sync::hd::Wallet> getCurrentHDWalletFromCombobox(QComboBox *walletsCombobox) const;

   void submitProposal(QComboBox *walletsCombobox, bs::XBTAmount amount, CbSuccess onSuccess);

   QString getXBTRange(bs::network::otc::Range xbtRange);
   QString getCCRange(bs::network::otc::Range ccRange);

   QString getSide(bs::network::otc::Side requestSide, bool isOwnRequest);

   const std::vector<UTXO> &selectedUTXOs() const { return selectedUTXO_; }
   void clearSelectedInputs();
   void setSelectedInputs(const std::vector<UTXO>& selectedUTXO);

   void setupTimer(TimeoutData&& timeoutData);
   std::chrono::seconds getSeconds(std::chrono::milliseconds durationInMillisecs);

protected:
   std::shared_ptr<OTCWindowsManager> otcManager_{};

   bs::network::Asset::Type productGroup_ = bs::network::Asset::SpotXBT;
   // #new_logic : fix security & product checking
   QString security_{ QLatin1String("XBT/EUR") };
   QString sellProduct_{ QLatin1String("XBT") };
   QString buyProduct_{ QLatin1String("EUR") };
   double sellIndicativePrice_{};
   double buyIndicativePrice_{};

   ValidityFlag validityFlag_;
   std::chrono::seconds timeoutSec_{};

   std::vector<UTXO> selectedUTXO_;
   bs::UtxoReservationToken reservation_;

private:
   void showXBTInputs(QComboBox *walletsCombobox);

   QTimer timeoutTimer_;
   TimeoutData currentTimeoutData_{};
};

#endif // __OTCWINDOWSMANAGER_H__
