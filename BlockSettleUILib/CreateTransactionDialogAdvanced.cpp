#include "CreateTransactionDialogAdvanced.h"
#include "ui_CreateTransactionDialogAdvanced.h"

#include "Address.h"
#include "ArmoryConnection.h"
#include "CoinControlDialog.h"
#include "OfflineSigner.h"
#include "SelectAddressDialog.h"
#include "SelectedTransactionInputs.h"
#include "SignContainer.h"
#include "TransactionData.h"
#include "TransactionOutputsModel.h"
#include "UiUtils.h"
#include "UsedInputsModel.h"
#include "WalletsManager.h"
#include "XbtAmountValidator.h"

#include <QEvent>
#include <QKeyEvent>
#include <QFile>
#include <QFileDialog>
#include <QPushButton>

#include <stdexcept>


CreateTransactionDialogAdvanced::CreateTransactionDialogAdvanced(const std::shared_ptr<ArmoryConnection> &armory
   , const std::shared_ptr<WalletsManager>& walletManager
   , const std::shared_ptr<SignContainer> &container, bool loadFeeSuggestions
   , const std::shared_ptr<spdlog::logger>& logger, QWidget* parent)
 : CreateTransactionDialog(armory, walletManager, container, loadFeeSuggestions
    , logger, parent)
 , ui_(new Ui::CreateTransactionDialogAdvanced)
 , logger_(logger_)
{
   ui_->setupUi(this);

   selectedChangeAddress_ = bs::Address{};

   initUI();

   connect(ui_->doubleSpinBoxFeesManualPerByte, QOverload<double>::of(&QDoubleSpinBox::valueChanged)
      , this, &CreateTransactionDialogAdvanced::setTxFees);
   connect(ui_->spinBoxFeesManualTotal, QOverload<int>::of(&QSpinBox::valueChanged)
      , this, &CreateTransactionDialogAdvanced::setTxFees);

   updateManualFeeControls();
}

CreateTransactionDialogAdvanced::~CreateTransactionDialogAdvanced() = default;

std::shared_ptr<CreateTransactionDialogAdvanced> CreateTransactionDialogAdvanced::CreateForRBF(
        const std::shared_ptr<ArmoryConnection> &armory
      , const std::shared_ptr<WalletsManager>& walletManager
      , const std::shared_ptr<SignContainer>& container
      , const std::shared_ptr<spdlog::logger>& logger
      , const Tx &tx
      , const std::shared_ptr<bs::Wallet>& wallet
      , QWidget* parent)
{
   auto dlg = std::make_shared<CreateTransactionDialogAdvanced>(armory,
                                                                walletManager,
                                                                container,
                                                                true,
                                                                logger,
                                                                parent);

   dlg->setWindowTitle(tr("Replace-By-Fee"));

   dlg->ui_->checkBoxRBF->setChecked(true);
   dlg->ui_->checkBoxRBF->setEnabled(false);
   dlg->ui_->pushButtonImport->setEnabled(false);

   dlg->setRBFinputs(tx, wallet);
   return dlg;
}

std::shared_ptr<CreateTransactionDialogAdvanced> CreateTransactionDialogAdvanced::CreateForCPFP(
        const std::shared_ptr<ArmoryConnection> &armory
      , const std::shared_ptr<WalletsManager>& walletManager
      , const std::shared_ptr<SignContainer>& container
      , const std::shared_ptr<bs::Wallet>& wallet
      , const std::shared_ptr<spdlog::logger>& logger
      , const Tx &tx
      , QWidget* parent)
{
   auto dlg = std::make_shared<CreateTransactionDialogAdvanced>(armory,
                                                                walletManager,
                                                                container,
                                                                true,
                                                                logger,
                                                                parent);

   dlg->setWindowTitle(tr("Child-Pays-For-Parent"));
   dlg->ui_->pushButtonImport->setEnabled(false);

   dlg->setCPFPinputs(tx, wallet);
   return dlg;
}

void CreateTransactionDialogAdvanced::setCPFPinputs(const Tx &tx, const std::shared_ptr<bs::Wallet> &wallet)
{
   std::set<BinaryData> txHashSet;
   std::map<BinaryData, std::set<uint32_t>> txOutIndices;
   for (size_t i = 0; i < tx.getNumTxIn(); i++) {
      const auto txin = tx.getTxInCopy(i);
      const auto outpoint = txin.getOutPoint();
      txHashSet.insert(outpoint.getTxHash());
      txOutIndices[outpoint.getTxHash()].insert(outpoint.getTxOutIndex());
   }

   const auto &cbTXs = [this, tx, wallet, txOutIndices](std::vector<Tx> txs) {
      auto selInputs = transactionData_->GetSelectedInputs();
      selInputs->SetUseAutoSel(false);
      int64_t totalVal = 0;
      for (const auto &prevTx : txs) {
         const auto &txHash = prevTx.getThisHash();
         const auto &itTxOut = txOutIndices.find(txHash);
         if (itTxOut == txOutIndices.end()) {
            continue;
         }
         for (const auto &txOutIdx : itTxOut->second) {
            if (prevTx.isInitialized()) {
               TxOut prevOut = prevTx.getTxOutCopy(txOutIdx);
               totalVal += prevOut.getValue();
            }
         }

         unsigned int cntOutputs = 0;
         for (size_t i = 0; i < tx.getNumTxOut(); i++) {
            auto out = tx.getTxOutCopy(i);
            const auto addr = bs::Address::fromTxOut(out);
            if (wallet->containsAddress(addr)) {
               if (selInputs->SetUTXOSelection(tx.getThisHash(),
                                               out.getIndex())) {
                  cntOutputs++;
               }
            }
            totalVal -= out.getValue();
         }

         if (!cntOutputs) {
            //!throw std::runtime_error("No input[s] found");
            return;
         }
         if (totalVal < 0) {
            //!throw std::runtime_error("negative TX balance");
            return;
         }

         const auto &cbFee = [this, tx, totalVal](float fee) {
            const auto txSize = tx.serializeNoWitness().getSize();
            const float feePerByte = (float)totalVal / txSize;
            originalFee_ = totalVal;
            const size_t projectedTxSize = 85;  // 1 input and 1 output bech32
            const float totalFee = std::abs(txSize * (fee - feePerByte) + projectedTxSize * fee);
            const float newFPB = std::ceil(totalFee / (txSize + projectedTxSize));

            QMetaObject::invokeMethod(this, [this, totalFee, newFPB] {
               SetMinimumFee(totalFee, newFPB);
               onTransactionUpdated();
            });
         };
         walletsManager_->estimatedFeePerByte(2, cbFee, this);
      }
   };

   SetFixedWallet(wallet->GetWalletId(), [this, txHashSet, cbTXs] {
      armory_->getTXsByHash(txHashSet, cbTXs);
   });
}

void CreateTransactionDialogAdvanced::setRBFinputs(const Tx &tx, const std::shared_ptr<bs::Wallet> &wallet)
{
   std::set<BinaryData> txHashSet;
   std::map<BinaryData, std::set<uint32_t>> txOutIndices;
   for (size_t i = 0; i < tx.getNumTxIn(); i++) {
      const auto txin = tx.getTxInCopy(i);
      const auto outpoint = txin.getOutPoint();
      txHashSet.insert(outpoint.getTxHash());
      txOutIndices[outpoint.getTxHash()].insert(outpoint.getTxOutIndex());
   }

   const auto &cbTXs = [this, tx, wallet, txOutIndices](std::vector<Tx> txs) {
      int64_t totalVal = 0;
      for (const auto &prevTx : txs) {
         const auto &txHash = prevTx.getThisHash();
         const auto &itTxOut = txOutIndices.find(txHash);
         if (itTxOut == txOutIndices.end()) {
            continue;
         }
         for (const auto &txOutIdx : itTxOut->second) {
            if (prevTx.isInitialized()) {
               TxOut prevOut = prevTx.getTxOutCopy(txOutIdx);
               totalVal += prevOut.getValue();
            }
            if (!transactionData_->GetSelectedInputs()->SetUTXOSelection(txHash, txOutIdx)) {
               //!throw std::runtime_error("No input[s] found");
               continue;
            }
         }
      }

      QString  changeAddress;
      double   changeAmount = 0;

      // set outputs
      for (size_t i = 0; i < tx.getNumTxOut(); i++) {
         TxOut out = tx.getTxOutCopy(i);
         const auto addr = bs::Address::fromTxOut(out);

         const auto addressString = addr.display();
         const auto amount = UiUtils::amountToBtc(out.getValue());

         // use last output as change addres
         if (wallet->containsAddress(addr)) {
            if (!changeAddress.isEmpty()) {
               AddRecipient(changeAddress, changeAmount);
            }

            changeAddress = addressString;
            changeAmount = amount;
         }
         else {
            AddRecipient(addressString, amount);
         }

         totalVal -= out.getValue();
      }

      QMetaObject::invokeMethod(this, [this, changeAddress] { SetFixedChangeAddress(changeAddress); });

      // set fee
      if (totalVal < 0) {
         //!throw std::runtime_error("Negative amount");
         return;
      }

      originalFee_ = totalVal;
      const auto &txSize = tx.serializeNoWitness().getSize();
      const float feePerByte = std::ceil((float)totalVal / txSize);
      SetMinimumFee(totalVal, feePerByte + minRelayFeePerByte_);

      if (changeAddress.isNull()) {
         setUnchangeableTx();
      }

      onTransactionUpdated();
   };

   const auto &cbRBFInputs = [this, wallet, txHashSet, cbTXs](ReturnMessage<std::vector<UTXO>> utxos) {
      try {
         auto inUTXOs = utxos.get();
         QMetaObject::invokeMethod(this, [this, wallet, inUTXOs, txHashSet, cbTXs] {
            SetFixedWalletAndInputs(wallet, inUTXOs);

            armory_->getTXsByHash(txHashSet, cbTXs);
         });
      }
      catch(std::exception& e) {
         logger_->error("[CreateTransactionDialogAdvanced::setRBFinputs] " \
            "Return data error - {}", e.what());
      }
   };
   wallet->getRBFTxOutList(cbRBFInputs);
}

void CreateTransactionDialogAdvanced::initUI()
{
   usedInputsModel_ = new UsedInputsModel(this);
   outputsModel_ = new TransactionOutputsModel(this);

   CreateTransactionDialog::init();

   ui_->treeViewInputs->setModel(usedInputsModel_);
   ui_->treeViewInputs->header()->setSectionResizeMode(QHeaderView::ResizeToContents);

   ui_->treeViewOutputs->setModel(outputsModel_);
   ui_->treeViewOutputs->setColumnWidth(2, 30);
   ui_->treeViewOutputs->header()->setSectionResizeMode(2, QHeaderView::Fixed);
   ui_->treeViewOutputs->header()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
   ui_->treeViewOutputs->header()->setSectionResizeMode(0, QHeaderView::Stretch);

   // QModelIndex isn't used. We should use it or lose it.
   connect(outputsModel_, &TransactionOutputsModel::rowsInserted, [this](const QModelIndex &parent, int first, int last)
   {
      for (int i = first; i <= last; i++) {
         auto index = outputsModel_->index(i, 2);
         auto outputId = outputsModel_->GetOutputId(i);

         auto button = new QPushButton();
         button->setFixedSize(30, 16);
         button->setContentsMargins(0, 0, 0, 0);

         button->setIcon(UiUtils::icon(0xeaf1, QVariantMap{
            { QLatin1String{ "color" }, QColor{ Qt::white } }
         }));

         ui_->treeViewOutputs->setIndexWidget(index, button);

         connect(button, &QPushButton::clicked, [this, outputId]()
            {
               RemoveOutputByRow(outputsModel_->GetRowById(outputId));
            });
      }
   });

   currentAddressValid_ = false;
   currentValue_ = 0;

   ui_->pushButtonAddOutput->setEnabled(false);
   ui_->line->hide();

   connect(ui_->comboBoxWallets, SIGNAL(currentIndexChanged(int)), this, SLOT(selectedWalletChanged(int)));

   connect(ui_->lineEditAddress, &QLineEdit::textChanged, this, &CreateTransactionDialogAdvanced::onAddressTextChanged);
   connect(ui_->lineEditAmount, &QLineEdit::textChanged, this, &CreateTransactionDialogAdvanced::onXBTAmountChanged);
   ui_->lineEditAddress->installEventFilter(this);
   ui_->lineEditAmount->installEventFilter(this);

   connect(ui_->pushButtonSelectInputs, &QPushButton::clicked, this, &CreateTransactionDialogAdvanced::onSelectInputs);
   connect(ui_->pushButtonAddOutput, &QPushButton::clicked, this, &CreateTransactionDialogAdvanced::onAddOutput);
   connect(ui_->pushButtonCreate, &QPushButton::clicked, this, &CreateTransactionDialogAdvanced::onCreatePressed);
   connect(ui_->pushButtonImport, &QPushButton::clicked, this, &CreateTransactionDialogAdvanced::onImportPressed);
   connect(ui_->pushButtonCancel, &QPushButton::clicked, this, &CreateTransactionDialogAdvanced::reject);

   ui_->radioButtonNewAddrNative->setChecked(true);

   connect(ui_->radioButtonNewAddrNative, &QRadioButton::clicked, this, &CreateTransactionDialogAdvanced::onNewAddressSelectedForChange);
   connect(ui_->radioButtonNewAddrNested, &QRadioButton::clicked, this, &CreateTransactionDialogAdvanced::onNewAddressSelectedForChange);
   connect(ui_->radioButtonExistingAddress, &QRadioButton::clicked, this, &CreateTransactionDialogAdvanced::onExistingAddressSelectedForChange);

   ui_->treeViewOutputs->setContextMenuPolicy(Qt::CustomContextMenu);

   removeOutputAction_ = new QAction(tr("Remove Output"), this);
   connect(removeOutputAction_, &QAction::triggered, this, &CreateTransactionDialogAdvanced::onRemoveOutput);
   contextMenu_.addAction(removeOutputAction_);

   connect(ui_->treeViewOutputs, &QTreeView::customContextMenuRequested, this, &CreateTransactionDialogAdvanced::showContextMenu);
}

void CreateTransactionDialogAdvanced::clear()
{
   CreateTransactionDialog::clear();

   outputsModel_->clear();
   usedInputsModel_->clear();
}

bool CreateTransactionDialogAdvanced::eventFilter(QObject *watched, QEvent *evt)
{
   if (evt->type() == QEvent::KeyPress) {
      auto keyID = static_cast<QKeyEvent *>(evt)->key();
      if ((keyID == Qt::Key_Return) || (keyID == Qt::Key_Enter)) {
         if ((watched == ui_->lineEditAddress) && currentAddressValid_ && qFuzzyIsNull(currentValue_)) {
            ui_->lineEditAmount->setFocus();
         }
         if ((watched == ui_->lineEditAmount) && !qFuzzyIsNull(currentValue_) && !currentAddressValid_) {
            ui_->lineEditAddress->setFocus();
         }
         else if (ui_->pushButtonAddOutput->isEnabled()) {
            ui_->pushButtonAddOutput->animateClick();
            ui_->lineEditAddress->setFocus();
         }
         else if (ui_->lineEditAddress->text().isEmpty() && ui_->lineEditAmount->text().isEmpty()) {
            return QDialog::eventFilter(watched, evt);
         }
         return true;
      }
   }
   return QDialog::eventFilter(watched, evt);
}

QComboBox *CreateTransactionDialogAdvanced::comboBoxWallets() const
{
   return ui_->comboBoxWallets;
}

QComboBox *CreateTransactionDialogAdvanced::comboBoxFeeSuggestions() const
{
   return ui_->comboBoxFeeSuggestions;
}

QLineEdit *CreateTransactionDialogAdvanced::lineEditAddress() const
{
   return ui_->lineEditAddress;
}

QLineEdit *CreateTransactionDialogAdvanced::lineEditAmount() const
{
   return ui_->lineEditAmount;
}

QPushButton *CreateTransactionDialogAdvanced::pushButtonMax() const
{
   return ui_->pushButtonMax;
}

QTextEdit *CreateTransactionDialogAdvanced::textEditComment() const
{
   return ui_->textEditComment;
}

QCheckBox *CreateTransactionDialogAdvanced::checkBoxRBF() const
{
   return ui_->checkBoxRBF;
}

QLabel *CreateTransactionDialogAdvanced::labelBalance() const
{
   return ui_->labelBalance;
}

QLabel *CreateTransactionDialogAdvanced::labelAmount() const
{
   return ui_->labelInputAmount;
}

QLabel *CreateTransactionDialogAdvanced::labelTxInputs() const
{
   return ui_->labelTXInputs;
}

QLabel *CreateTransactionDialogAdvanced::labelEstimatedFee() const
{
   return ui_->labelFee;
}

QLabel *CreateTransactionDialogAdvanced::labelTotalAmount() const
{
   return ui_->labelTransationAmount;
}

QLabel *CreateTransactionDialogAdvanced::labelTxSize() const
{
   return ui_->labelTxSize;
}

QPushButton *CreateTransactionDialogAdvanced::pushButtonCreate() const
{
   return ui_->pushButtonCreate;
}

QPushButton *CreateTransactionDialogAdvanced::pushButtonCancel() const
{
   return ui_->pushButtonCancel;
}

QLabel* CreateTransactionDialogAdvanced::feePerByteLabel() const
{
   return ui_->labelFeePerByte;
}

QLabel* CreateTransactionDialogAdvanced::changeLabel() const
{
   return ui_->labelReturnAmount;
}

void CreateTransactionDialogAdvanced::showContextMenu(const QPoint &point)
{
   if (!removeOutputEnabled_) {
      return;
   }

   const auto index = ui_->treeViewOutputs->indexAt(point);
   if (index.row() != -1) {
      removeOutputAction_->setEnabled(true);
      removeOutputAction_->setData(index.row());

      contextMenu_.exec(ui_->treeViewOutputs->mapToGlobal(point));
   }
}

void CreateTransactionDialogAdvanced::onRemoveOutput()
{
   int row = removeOutputAction_->data().toInt();
   RemoveOutputByRow(row);
}

void CreateTransactionDialogAdvanced::RemoveOutputByRow(int row)
{
   auto outputId = outputsModel_->GetOutputId(row);

   transactionData_->RemoveRecipient(outputId);
   outputsModel_->RemoveRecipient(row);

   ui_->comboBoxFeeSuggestions->setEnabled(true);
}

void CreateTransactionDialogAdvanced::selectedWalletChanged(int index, bool resetInputs, const std::function<void()> &cbInputsReset)
{
   CreateTransactionDialog::selectedWalletChanged(index, resetInputs, cbInputsReset);

   ui_->radioButtonNewAddrNative->setChecked(true);
}

void CreateTransactionDialogAdvanced::onTransactionUpdated()
{
   CreateTransactionDialog::onTransactionUpdated();

   usedInputsModel_->updateInputs(transactionData_->inputs());

   const auto &summary = transactionData_->GetTransactionSummary();

   if (!changeAddressFixed_) {
      bool changeSelectionEnabled = summary.hasChange || (summary.transactionSize == 0);
      ui_->changeAddrGroupBox->setEnabled(changeSelectionEnabled);
      showExistingChangeAddress(changeSelectionEnabled);
   }

   if (originalFee_) {
      SetMinimumFee(originalFee_ + minRelayFeePerByte_ * summary.transactionSize, minFeePerByte_);
   }
   QMetaObject::invokeMethod(this, &CreateTransactionDialogAdvanced::validateCreateButton
      , Qt::QueuedConnection);
}

void CreateTransactionDialogAdvanced::preSetAddress(const QString& address)
{
   ui_->lineEditAddress->setText(address);
   onAddressTextChanged(address);
}

void CreateTransactionDialogAdvanced::preSetValue(const double value)
{
   ui_->lineEditAmount->setText(UiUtils::displayAmount(value));
}

void CreateTransactionDialogAdvanced::onAddressTextChanged(const QString& addressString)
{
   try {
      bs::Address address{addressString.trimmed()};
      currentAddressValid_ = address.isValid();
   } catch (...) {
      currentAddressValid_ = false;
   }

   if (currentAddressValid_)
      UiUtils::setWrongState(ui_->lineEditAddress, false);
   else
      UiUtils::setWrongState(ui_->lineEditAddress, true);

   validateAddOutputButton();
}

void CreateTransactionDialogAdvanced::onXBTAmountChanged(const QString &text)
{
   currentValue_ = UiUtils::parseAmountBtc(text);
   validateAddOutputButton();
}

void CreateTransactionDialogAdvanced::onSelectInputs()
{
   CoinControlDialog dlg(transactionData_->GetSelectedInputs(), this);
   dlg.exec();
}

void CreateTransactionDialogAdvanced::onAddOutput()
{
   const bs::Address address(ui_->lineEditAddress->text().trimmed());

   auto maxValue = transactionData_->CalculateMaxAmount(address);
   const bool maxAmount = qFuzzyCompare(maxValue, currentValue_);

   AddRecipient(address, currentValue_, maxAmount);

   // clear edits
   ui_->lineEditAddress->clear();
   ui_->lineEditAmount->clear();
   if (maxAmount) {
      ui_->comboBoxFeeSuggestions->setEnabled(false);
   }

   ui_->pushButtonAddOutput->setEnabled(false);
}

// Nothing is being done with isMax right now. We should use it or lose it.
void CreateTransactionDialogAdvanced::AddRecipient(const bs::Address &address, double amount, bool isMax)
{
   auto recipientId = transactionData_->RegisterNewRecipient();

   transactionData_->UpdateRecipientAddress(recipientId, address);
   transactionData_->UpdateRecipientAmount(recipientId, amount);

   // add to the model
   outputsModel_->AddRecipient(recipientId, address.display(), amount);
}

void CreateTransactionDialogAdvanced::validateAddOutputButton()
{
   ui_->pushButtonMax->setEnabled(currentAddressValid_);
   ui_->pushButtonAddOutput->setEnabled(currentAddressValid_
                                        && !qFuzzyIsNull(currentValue_));
}

void CreateTransactionDialogAdvanced::validateCreateButton()
{
   const bool isSignerReady = signingContainer_ && ((signingContainer_->opMode() == SignContainer::OpMode::Offline)
      || !signingContainer_->isOffline());
   const bool isTxValid = transactionData_->IsTransactionValid() && transactionData_->GetTransactionSummary().transactionSize;

   ui_->pushButtonCreate->setEnabled(isTxValid
      && isSignerReady
      && !broadcasting_
      && (ui_->radioButtonNewAddrNative->isChecked() || ui_->radioButtonNewAddrNested->isChecked()
         || (selectedChangeAddress_.isValid())));
}

void CreateTransactionDialogAdvanced::AddManualFeeEntries(float feePerByte, float totalFee)
{
   ui_->doubleSpinBoxFeesManualPerByte->setValue(feePerByte);
   ui_->spinBoxFeesManualTotal->setValue(qRound(totalFee));
   ui_->comboBoxFeeSuggestions->addItem(tr("Manual Fee Selection"));
   ui_->comboBoxFeeSuggestions->addItem(tr("Total Network Fee"));
}

void CreateTransactionDialogAdvanced::onFeeSuggestionsLoaded(const std::map<unsigned int, float> &feeValues)
{
   if (feeChangeDisabled_) {
      return;
   }

   CreateTransactionDialog::onFeeSuggestionsLoaded(feeValues);

   AddManualFeeEntries((minFeePerByte_ > 0) ? minFeePerByte_ : feeValues.begin()->second
      , (minTotalFee_ > 0) ? minTotalFee_ : 0);

   if (minFeePerByte_ > 0) {
      const auto index = ui_->comboBoxFeeSuggestions->count() - 2;
      ui_->comboBoxFeeSuggestions->setCurrentIndex(index);
      feeSelectionChanged(index);
   }
}

void CreateTransactionDialogAdvanced::SetMinimumFee(float totalFee, float feePerByte)
{
   minTotalFee_ = totalFee;
   minFeePerByte_ = feePerByte;

   ui_->doubleSpinBoxFeesManualPerByte->setMinimum(feePerByte);
   ui_->spinBoxFeesManualTotal->setMinimum(qRound(totalFee));
}

// currentIndex isn't being used. We should use it or lose it.
void CreateTransactionDialogAdvanced::feeSelectionChanged(int currentIndex)
{
   setTxFees();
   updateManualFeeControls();
}

bs::Address CreateTransactionDialogAdvanced::getChangeAddress() const
{
   bs::Address result;
   if (transactionData_->GetTransactionSummary().hasChange) {
      if (ui_->radioButtonNewAddrNative->isChecked() || ui_->radioButtonNewAddrNested->isChecked()) {
         result = transactionData_->GetWallet()->GetNewChangeAddress(
            ui_->radioButtonNewAddrNative->isChecked() ? AddressEntryType_P2WPKH : AddressEntryType_P2SH);
         transactionData_->createAddress(result);
         transactionData_->GetWallet()->SetAddressComment(result, bs::wallet::Comment::toString(bs::wallet::Comment::ChangeAddress));
      } else {
         result = selectedChangeAddress_;
      }
   }
   return result;
}

void CreateTransactionDialogAdvanced::onCreatePressed()
{
   if (!importedSignedTX_.isNull()) {
      if (BroadcastImportedTx()) {
         accept();
      }
      else {
         initUI();
         validateCreateButton();
      }
      return;
   }

   if (!CreateTransaction()) {
      reject();
   }
}

bool CreateTransactionDialogAdvanced::HaveSignedImportedTransaction() const
{
   return !importedSignedTX_.isNull();
}

void CreateTransactionDialogAdvanced::SetImportedTransactions(const std::vector<bs::wallet::TXSignRequest>& transactions)
{
   ui_->pushButtonCreate->setEnabled(false);
   ui_->pushButtonCreate->setText(tr("Broadcast"));

   const auto &tx = transactions[0];
   if (!tx.prevStates.empty()) {    // signed TX
      ui_->textEditComment->insertPlainText(QString::fromStdString(tx.comment));

      if (tx.prevStates.size() == 1) {
         importedSignedTX_ = tx.prevStates[0];

         Tx tx(importedSignedTX_);
         if (tx.isInitialized()) {
            ui_->pushButtonCreate->setEnabled(true);

            std::set<BinaryData> txHashSet;
            std::map<BinaryData, std::set<uint32_t>> txOutIndices;
            std::vector<std::pair<BinaryData, uint32_t>> utxoHashes;

            for (size_t i = 0; i < tx.getNumTxIn(); i++) {
               auto in = tx.getTxInCopy((int)i);
               OutPoint op = in.getOutPoint();
               utxoHashes.push_back({ op.getTxHash(), op.getTxOutIndex() });
               txHashSet.insert(op.getTxHash());
               txOutIndices[op.getTxHash()].insert(op.getTxOutIndex());
            }

            const auto &cbTXs = [this, tx, utxoHashes, txOutIndices](std::vector<Tx> txs) {
               std::shared_ptr<bs::Wallet> wallet;
               int64_t totalVal = 0;

               for (const auto &prevTx : txs) {
                  const auto &itTxOut = txOutIndices.find(prevTx.getThisHash());
                  if (itTxOut == txOutIndices.end()) {
                     continue;
                  }
                  for (const auto &txOutIdx : itTxOut->second) {
                     const auto prevOut = prevTx.getTxOutCopy(txOutIdx);
                     totalVal += prevOut.getValue();
                     if (!wallet) {
                        const auto addr = bs::Address::fromTxOut(prevOut);
                        const auto &addrWallet = walletsManager_->GetWalletByAddress(addr);
                        if (addrWallet) {
                           wallet = addrWallet;
                        }
                     }
                  }
               }

               if (wallet) {
                  SetFixedWallet(wallet->GetWalletId());
                  auto selInputs = transactionData_->GetSelectedInputs();
                  for (const auto &txHash : utxoHashes) {
                     selInputs->SetUTXOSelection(txHash.first, txHash.second);
                  }
               }
               for (size_t i = 0; i < tx.getNumTxOut(); ++i) {
                  TxOut out = tx.getTxOutCopy((int)i);
                  const auto addr = bs::Address::fromTxOut(out);
                  if (wallet && (i == (tx.getNumTxOut() - 1)) && (wallet->containsAddress(addr))) {
                     SetFixedChangeAddress(addr.display());
                  }
                  else {
                     AddRecipient(addr.display(), out.getValue() / BTCNumericTypes::BalanceDivider);
                  }
                  totalVal -= out.getValue();
               }
               SetPredefinedFee(totalVal);
            };
            armory_->getTXsByHash(txHashSet, cbTXs);
         }
      }
   } else {
      SetFixedWallet(tx.walletId);
      if (tx.change.value) {
         SetFixedChangeAddress(tx.change.address.display());
      }
      SetPredefinedFee(tx.fee);
      labelEstimatedFee()->setText(UiUtils::displayAmount(tx.fee));
      ui_->textEditComment->insertPlainText(QString::fromStdString(tx.comment));
      auto selInputs = transactionData_->GetSelectedInputs();
      for (const auto &utxo : tx.inputs) {
         selInputs->SetUTXOSelection(utxo.getTxHash(), utxo.getTxOutIndex());
      }

      for (const auto &recip : tx.recipients) {
         const auto addr = bs::Address::fromRecipient(recip);
         AddRecipient(addr.display(), recip->getValue() / BTCNumericTypes::BalanceDivider);
      }

      if (!signingContainer_->isOffline() && tx.isValid()) {
         ui_->pushButtonCreate->setEnabled(true);
      }
   }

   ui_->checkBoxRBF->setChecked(tx.RBF);
   ui_->checkBoxRBF->setEnabled(false);

   disableOutputsEditing();
   disableInputSelection();
   disableFeeChanging();
   updateCreateButtonText();
   disableChangeAddressSelecting();
}

void CreateTransactionDialogAdvanced::onImportPressed()
{
   const auto transactions = ImportTransactions();
   if (transactions.empty()) {
      return;
   }

   SetImportedTransactions(transactions);
}

void CreateTransactionDialogAdvanced::onNewAddressSelectedForChange()
{
   selectedChangeAddress_ = bs::Address{};
   showExistingChangeAddress(false);
}

void CreateTransactionDialogAdvanced::onExistingAddressSelectedForChange()
{
   SelectAddressDialog selectAddressDialog(walletsManager_, transactionData_->GetWallet(), this
      , AddressListModel::AddressType::Internal);

   if (selectAddressDialog.exec() == QDialog::Accepted) {
      selectedChangeAddress_ = selectAddressDialog.getSelectedAddress();
      showExistingChangeAddress(true);
   } else {
      if (!selectedChangeAddress_.isValid()) {
         ui_->radioButtonNewAddrNative->setChecked(true);
      }
   }
}

void CreateTransactionDialogAdvanced::SetFixedWallet(const std::string& walletId, const std::function<void()> &cbInputsReset)
{
   const int idx = SelectWallet(walletId);
   selectedWalletChanged(idx, true, cbInputsReset);
   ui_->comboBoxWallets->setEnabled(false);
}

void CreateTransactionDialogAdvanced::SetFixedWalletAndInputs(const std::shared_ptr<bs::Wallet> &wallet, const std::vector<UTXO> &inputs)
{
   SelectWallet(wallet->GetWalletId());
   ui_->comboBoxWallets->setEnabled(false);
   disableInputSelection();
   transactionData_->SetWalletAndInputs(wallet, inputs, armory_->topBlock());
}

void CreateTransactionDialogAdvanced::disableOutputsEditing()
{
   ui_->lineEditAddress->setEnabled(false);
   ui_->lineEditAmount->setEnabled(false);
   ui_->pushButtonMax->setEnabled(false);
   ui_->pushButtonAddOutput->setEnabled(false);
   ui_->treeViewOutputs->setEnabled(false);

   removeOutputEnabled_ = false;
}

void CreateTransactionDialogAdvanced::disableInputSelection()
{
   ui_->pushButtonSelectInputs->setEnabled(false);
}

void CreateTransactionDialogAdvanced::disableFeeChanging()
{
   feeChangeDisabled_ = true;
   ui_->comboBoxFeeSuggestions->setEnabled(false);
   ui_->doubleSpinBoxFeesManualPerByte->setEnabled(false);
   ui_->spinBoxFeesManualTotal->setEnabled(false);
}

void CreateTransactionDialogAdvanced::SetFixedChangeAddress(const QString& changeAddress)
{
   ui_->radioButtonExistingAddress->setChecked(true);

   ui_->radioButtonNewAddrNative->setEnabled(false);
   ui_->radioButtonNewAddrNested->setEnabled(false);

   selectedChangeAddress_ = bs::Address{changeAddress};
   showExistingChangeAddress(true);

   changeAddressFixed_ = true;
}

void CreateTransactionDialogAdvanced::SetPredefinedFee(const int64_t& manualFee)
{
   ui_->comboBoxFeeSuggestions->clear();
   ui_->comboBoxFeeSuggestions->addItem(tr("%1 satoshi").arg(manualFee), (qlonglong)manualFee);
   transactionData_->SetTotalFee(manualFee);
}

void CreateTransactionDialogAdvanced::setUnchangeableTx()
{
   ui_->comboBoxFeeSuggestions->setEnabled(false);
   ui_->treeViewOutputs->setEnabled(false);
   ui_->lineEditAddress->setEnabled(false);
   ui_->lineEditAmount->setEnabled(false);
   ui_->textEditComment->setEnabled(false);
   ui_->pushButtonCreate->setEnabled(false);
}

void CreateTransactionDialogAdvanced::showExistingChangeAddress(bool show)
{
   if (show && selectedChangeAddress_.isValid()) {
      ui_->labelChangeAddress->setText(selectedChangeAddress_.display());
   } else {
      ui_->labelChangeAddress->clear();
   }
}

void CreateTransactionDialogAdvanced::disableChangeAddressSelecting()
{
   ui_->widgetChangeAddress->setEnabled(false);
}

void CreateTransactionDialogAdvanced::updateManualFeeControls()
{
   int itemIndex = ui_->comboBoxFeeSuggestions->currentIndex();
   int itemCount = ui_->comboBoxFeeSuggestions->count();

   ui_->doubleSpinBoxFeesManualPerByte->setVisible(itemCount > 2 && itemIndex == itemCount - 2);
   ui_->spinBoxFeesManualTotal->setVisible(itemCount > 2 && itemIndex == itemCount - 1);
}

void CreateTransactionDialogAdvanced::setTxFees()
{
   int itemIndex = ui_->comboBoxFeeSuggestions->currentIndex();
   int itemCount = ui_->comboBoxFeeSuggestions->count();

   if (itemIndex < (ui_->comboBoxFeeSuggestions->count() - 2)) {
      CreateTransactionDialog::feeSelectionChanged(itemIndex);
   } else if (itemIndex == itemCount - 2) {
      transactionData_->SetFeePerByte(float(ui_->doubleSpinBoxFeesManualPerByte->value()));
   } else if (itemIndex == itemCount - 1) {
      transactionData_->SetTotalFee(ui_->spinBoxFeesManualTotal->value());
   }
}
