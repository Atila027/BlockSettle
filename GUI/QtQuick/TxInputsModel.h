/*

***********************************************************************************
* Copyright (C) 2020 - 2022, BlockSettle AB
* Distributed under the GNU Affero General Public License (AGPL v3)
* See LICENSE or http://www.gnu.org/licenses/agpl.html
*
**********************************************************************************

*/
#ifndef TX_INPUTS_MODEL_H
#define TX_INPUTS_MODEL_H

#include <QAbstractTableModel>
#include <QColor>
#include <QList>
#include <QObject>
#include <QVariant>
#include "Address.h"
#include "BinaryData.h"
#include "TxClasses.h"

namespace spdlog {
   class logger;
}
class TxOutputsModel;


class QUTXO : public QObject
{
   Q_OBJECT
public:
   QUTXO(const UTXO& utxo, QObject* parent = nullptr)
      : QObject(parent), utxo_(utxo) {}
   UTXO utxo() const { return utxo_; }

private:
   const UTXO utxo_;
};

class QUTXOList : public QObject
{
   Q_OBJECT
public:
   QUTXOList(const QList<QUTXO*>& data, QObject* parent = nullptr)
      : QObject(parent), data_(data)
   {}
   QList<QUTXO*> data() const { return data_; }

private:
   QList<QUTXO*> data_;
};

class TxInputsModel : public QAbstractTableModel
{
   Q_OBJECT
public:
   enum TableRoles { TableDataRole = Qt::UserRole + 1, HeadingRole, WidthRole
      , ColorRole, BgColorRole };
   TxInputsModel(const std::shared_ptr<spdlog::logger>&, TxOutputsModel*, QObject* parent = nullptr);

   int rowCount(const QModelIndex & = QModelIndex()) const override;
   int columnCount(const QModelIndex & = QModelIndex()) const override;
   QVariant data(const QModelIndex& index, int role) const override;
   QHash<int, QByteArray> roleNames() const override;

   void clear();
   void addUTXOs(const std::vector<UTXO>&);
   void setTopBlock(uint32_t topBlock) { topBlock_ = topBlock; }

   Q_PROPERTY(int nbTx READ nbTx NOTIFY selectionChanged)
   int nbTx() const { return nbTx_; }
   Q_PROPERTY(QString balance READ balance NOTIFY selectionChanged)
   QString balance() const { return QString::number(selectedBalance_ / BTCNumericTypes::BalanceDivider, 'f', 8); }

   Q_PROPERTY(QString fee READ fee WRITE setFee NOTIFY feeChanged)
   QString fee() const { return fee_; }
   void setFee(const QString& fee) { fee_ = fee; emit feeChanged(); }

   Q_INVOKABLE void toggle(int row);
   Q_INVOKABLE void toggleSelection(int row);
   Q_INVOKABLE QUTXOList* getSelection();

signals:
   void selectionChanged() const;
   void feeChanged() const;

private:
   QVariant getData(int row, int col) const;
   QColor dataColor(int row, int col) const;
   QColor bgColor(int row) const;
   float colWidth(int col) const;
   QList<QUTXO*> collectUTXOsFor(double amount);

private:
   std::shared_ptr<spdlog::logger>  logger_;
   TxOutputsModel* outsModel_{ nullptr };
   const QStringList header_;
   std::map<bs::Address, std::vector<UTXO>>  utxos_;

   struct Entry {
      bs::Address address;
      BinaryData  txId{};
      uint32_t    txOutIndex{ UINT32_MAX };
      bool  expanded{ false };
   };
   std::vector<Entry>   data_;
   std::set<int>  selection_;
   std::map<int, QList<QUTXO*>>   preSelected_;
   int nbTx_{ 0 };
   uint64_t  selectedBalance_{ 0 };
   QString fee_;
   uint32_t topBlock_{ 0 };
   double collectUTXOsForAmount_{ 0 };
};

#endif	// TX_INPUTS_MODEL_H