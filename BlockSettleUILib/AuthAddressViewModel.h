/*

***********************************************************************************
* Copyright (C) 2018 - 2020, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef __AUTH_ADDRESS_MODEL_H__
#define __AUTH_ADDRESS_MODEL_H__

#include <QAbstractItemModel>
#include <QSortFilterProxyModel>
#include <QPointer>

#include <memory>

#include "AuthAddress.h"
#include "AuthAddressManager.h"
#include "BinaryData.h"

class AuthAddressViewModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   AuthAddressViewModel(const std::shared_ptr<AuthAddressManager>& authManager, QObject *parent = nullptr);
   ~AuthAddressViewModel() noexcept override;

   AuthAddressViewModel(const AuthAddressViewModel&) = delete;
   AuthAddressViewModel& operator = (const AuthAddressViewModel&) = delete;

   AuthAddressViewModel(AuthAddressViewModel&&) = delete;
   AuthAddressViewModel& operator = (AuthAddressViewModel&&) = delete;

   bs::Address getAddress(const QModelIndex& index) const;
   bool isAddressNotSubmitted(int row) const;
   void setDefaultAddr(const bs::Address &addr);

public:
   int columnCount(const QModelIndex &parent = QModelIndex()) const override;
   int rowCount(const QModelIndex &parent = QModelIndex()) const override;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
   QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
   QModelIndex parent(const QModelIndex &child) const override;

private slots :
   void onAddressListUpdated();

signals:
   void updateSelectionAfterReset(int row);

private:
   std::shared_ptr<AuthAddressManager> authManager_;

private:
   enum AuthAddressViewColumns : int
   {
      ColumnName,
      ColumnState,
      ColumnsCount
   };
   bs::Address  defaultAddr_;
   std::vector<bs::Address> addresses_;
};

class AuthAdressControlProxyModel : public QSortFilterProxyModel {
public:
   explicit AuthAdressControlProxyModel(AuthAddressViewModel *sourceModel, QWidget *parent);
   ~AuthAdressControlProxyModel() override;

   void setVisibleRowsCount(int rows);
   void increaseVisibleRowsCountByOne();
   int getVisibleRowsCount() const;

   void setDefaultAddr(const bs::Address &addr);
   bs::Address getAddress(const QModelIndex& index) const;
   bool isEmpty() const;

   QModelIndex getFirstUnsubmitted() const;
   bool isUnsubmittedAddressVisible() const;

protected:
   bool filterAcceptsRow(int row, const QModelIndex& parent) const override;

private:
   int visibleRowsCount_{};
   QPointer<AuthAddressViewModel> sourceModel_ = nullptr;
};

#endif // __AUTH_ADDRESS_MODEL_H__
