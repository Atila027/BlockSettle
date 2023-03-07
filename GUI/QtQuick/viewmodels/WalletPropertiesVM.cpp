/*

***********************************************************************************
* Copyright (C) 2023, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#include "WalletPropertiesVM.h"

using namespace qtquick_gui;

WalletPropertiesVM::WalletPropertiesVM(QObject* parent)
   : QObject(parent)
{
}

void WalletPropertiesVM::setWalletInfo(const WalletInfo& info)
{
   info_ = info;
   emit changed();
}

void qtquick_gui::WalletPropertiesVM::setWalletSeed(const std::string& walletId, const std::string& seed)
{
   if (walletId != info_.walletId.toStdString()) {
      return;
   }
   seed_ = QString::fromStdString(seed).split(QLatin1Char(' '));
   emit seedChanged();
}

void qtquick_gui::WalletPropertiesVM::setNbActiveAddrs(const std::string& walletId, uint32_t nb)
{
   if (info_.walletId.isEmpty() || (info_.walletId.toStdString() != walletId)) {
      return;
   }
   nbActiveAddrs_ = nb;
   emit changed();
}

void qtquick_gui::WalletPropertiesVM::setNbUTXOs(const std::string& walletId, uint32_t nb)
{
   if (info_.walletId.isEmpty() || (info_.walletId.toStdString() != walletId)) {
      return;
   }
   nbUTXOs_ = nb;
   emit changed();
}

const QString& WalletPropertiesVM::walletName() const
{
   return info_.name;
}

const QString& WalletPropertiesVM::walletId() const
{
   return info_.walletId;
}

const QString& WalletPropertiesVM::walletGroups() const
{
   return info_.groups;
}

const QString& WalletPropertiesVM::walletEncryption() const
{
   return info_.ecryption;
}

quint32 WalletPropertiesVM::walletGeneratedAddresses() const
{
   return info_.generatedAddresses;
}

quint32 WalletPropertiesVM::walletActiveAddresses() const
{
   return nbActiveAddrs_;
}

quint32 WalletPropertiesVM::walletAvailableUtxo() const
{
   return nbUTXOs_;
}

bool WalletPropertiesVM::isHardware() const
{
   return info_.isHardware;
}

bool WalletPropertiesVM::isWatchingOnly() const
{
   return info_.isWatchingOnly;
}

const QStringList& WalletPropertiesVM::seed() const
{
   return seed_;
}

const QString& WalletPropertiesVM::exportPath() const
{
   return exportPath_;
}

void WalletPropertiesVM::setExportPath(const QString& path)
{
   exportPath_ = path;
   emit pathChanged();
}
