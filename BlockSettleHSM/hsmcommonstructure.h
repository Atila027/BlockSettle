/*

***********************************************************************************
* Copyright (C) 2016 - , BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef HSMCOMMONSTRUCTURE_H
#define HSMCOMMONSTRUCTURE_H

#include <QMetaType>
#include <functional>
#include <QString>
#include <string>
#include <QObject>
#include "CoreWallet.h"

using AsyncCallBack = std::function<void()>;
using AsyncCallBackCall = std::function<void(QVariant&&)>;

struct DeviceData
{
   QByteArray path_ = {};
   QByteArray vendor_ = {};
   QByteArray product_ = {};
   QByteArray sessionId_ = {};
   QByteArray debug_ = {};
   QByteArray debugSession_ = {};
};

enum class DeviceType {
   None = 0,
   HWLedger,
   HWTrezor
};

struct DeviceKey
{
   QString deviceLabel_;
   QString deviceId_;
   QString vendor_;
   QString walletId_;
   QString status_;

   DeviceType type_ = DeviceType::None;
};

class HSMWalletWrapper {
   Q_GADGET
public:
   bs::core::wallet::HSMWalletInfo info_;
   Q_INVOKABLE QString walletName() {
      return QString::fromStdString(info_.label_);
   }
   Q_INVOKABLE QString walletDesc() {
      return QString::fromStdString(info_.vendor_);
   }
};
Q_DECLARE_METATYPE(HSMWalletWrapper)

struct HSMSignedTx {
   std::string signedTx;
};
Q_DECLARE_METATYPE(HSMSignedTx)

std::vector<uint32_t> getDerivationPath(bool testNet, bool isNestedSegwit);
#endif // HSMCOMMONSTRUCTURE_H
