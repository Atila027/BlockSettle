#ifndef QML_WALLETS_VIEW_MODEL_H
#define QML_WALLETS_VIEW_MODEL_H

#include <QAbstractItemModel>
#include <memory>
#include <atomic>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include "CoreWallet.h"
#include "QWalletInfo.h"


namespace bs {
   namespace core {
      namespace hd {
         class Wallet;
      }
      class WalletsManager;
   }
}
class SignContainer;
class QmlWalletsViewModel;


class QmlWalletNode
{
public:
   enum class Type {
      Unknown,
      Root,
      WalletDummy,
      WalletPrimary,
      WalletRegular,
      GroupBitcoin,
      GroupAuth,
      GroupCC,
      Leaf
   };

   QmlWalletNode(QmlWalletsViewModel *vm, Type type, int row = 0, QmlWalletNode *parent = nullptr)
      : viewModel_(vm), parent_(parent), row_(row), type_(type) {}
   virtual ~QmlWalletNode() { clear(); }

   virtual std::vector<std::shared_ptr<bs::core::Wallet>> wallets() const { return {}; }
   virtual std::shared_ptr<bs::core::hd::Wallet> hdWallet() const { return nullptr; }
   virtual QVariant data(int, int) const { return QVariant(); }
   virtual std::string id() const { return {}; }

   void add(QmlWalletNode *child) { children_.append(child); }
   void clear();
   int nbChildren() const { return children_.count(); }
   bool hasChildren() const { return !children_.empty(); }
   QmlWalletNode *parent() const { return parent_; }
   QmlWalletNode *child(int index) const;
   int row() const { return row_; }
   const std::string &name() const { return name_; }
   Type type() const { return type_; }

   QmlWalletNode *findByWalletId(const std::string &walletId);

protected:
   std::string          name_;
   QmlWalletsViewModel *viewModel_;
   QmlWalletNode  *     parent_ = nullptr;
   int                  row_;
   Type                 type_ = Type::Unknown;
   QList<QmlWalletNode *> children_;
};


class QmlWalletsViewModel : public QAbstractItemModel
{
   Q_OBJECT
public:
   QmlWalletsViewModel(const std::shared_ptr<bs::core::WalletsManager> &
                       , QObject *parent = nullptr);
   ~QmlWalletsViewModel() override = default;

   QmlWalletsViewModel(const QmlWalletsViewModel&) = delete;
   QmlWalletsViewModel& operator = (const QmlWalletsViewModel&) = delete;
   QmlWalletsViewModel(QmlWalletsViewModel&&) = delete;
   QmlWalletsViewModel& operator = (QmlWalletsViewModel&&) = delete;

   std::shared_ptr<bs::core::Wallet> getWallet(const QModelIndex &index) const;
   QmlWalletNode *getNode(const QModelIndex &) const;

/*   void setSelectedWallet(const std::shared_ptr<bs::core::Wallet> &selWallet) { selectedWallet_ = selWallet; }
   std::shared_ptr<bs::core::Wallet> selectedWallet() const { return selectedWallet_; }*/

   void loadWallets();

   int columnCount(const QModelIndex &parent = QModelIndex()) const override;
   int rowCount(const QModelIndex &parent = QModelIndex()) const override;

   QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const Q_DECL_OVERRIDE;
   QHash<int, QByteArray> roleNames() const Q_DECL_OVERRIDE;
   QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
   QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
   QModelIndex parent(const QModelIndex &child) const override;
   bool hasChildren(const QModelIndex& parent = QModelIndex()) const override;

public:
   enum class WalletColumns : int
   {
      ColumnName,
      ColumnID,
      ColumnDescription,
      ColumnEmpty,
      ColumnCount
   };

   enum Roles {
      firstRole = Qt::UserRole + 1,
      NameRole = firstRole,
      DescRole,
      StateRole,
      WalletIdRole,
      IsHDRootRole,
      RootWalletIdRole,
      IsEncryptedRole,
      EncKeyRole
   };
   Q_ENUM(Roles)

private:
   QVariant getData(const QModelIndex &index, int role) const;

private:
   std::shared_ptr<bs::core::WalletsManager> walletsManager_;
//   std::shared_ptr<bs::core::Wallet>   selectedWallet_;
   std::shared_ptr<QmlWalletNode>      rootNode_;
};

#endif // QML_WALLETS_VIEW_MODEL_H
