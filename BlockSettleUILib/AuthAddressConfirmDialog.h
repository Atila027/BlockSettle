/*

***********************************************************************************
* Copyright (C) 2016 - , BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef AUTH_ADDRESS_CONFIRMATION_DIALOG_H__
#define AUTH_ADDRESS_CONFIRMATION_DIALOG_H__

#include "AuthAddressManager.h"

#include <QDialog>
#include <QTimer>
#include <QString>
#include <QPointer>

#include <chrono>

class BsClient;
class ApplicationSettings;
namespace Ui {
    class AuthAddressConfirmDialog;
};

class AuthAddressConfirmDialog : public QDialog
{
Q_OBJECT

public:
   AuthAddressConfirmDialog(BsClient *bsClient,
      const bs::Address& address,
      const std::shared_ptr<AuthAddressManager>& authManager,
      const std::shared_ptr<ApplicationSettings> &settings,
      QWidget* parent = nullptr);
   ~AuthAddressConfirmDialog() override;

private slots:
   void onUiTimerTick();
   void onCancelPressed();

   void onError(const QString &errorText);
   void onAuthAddrSubmitError(const QString &address, const QString &error);
   void onAuthConfirmSubmitError(const QString &address, const QString &error);
   void onAuthAddrSubmitSuccess(const QString &address);
   void onAuthAddressSubmitCancelled(const QString &address);

private:
   void reject() override;

private:
   std::unique_ptr<Ui::AuthAddressConfirmDialog> ui_;

   bs::Address                         address_;
   std::shared_ptr<AuthAddressManager> authManager_;
   std::shared_ptr<ApplicationSettings>   settings_;

   QTimer                                 progressTimer_;
   std::chrono::steady_clock::time_point  startTime_;

   QPointer<BsClient> bsClient_;
};

#endif // AUTH_ADDRESS_CONFIRMATION_DIALOG_H__
