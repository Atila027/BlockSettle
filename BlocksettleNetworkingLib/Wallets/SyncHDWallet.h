#ifndef BS_SYNC_HD_WALLET_H
#define BS_SYNC_HD_WALLET_H

#include <functional>
#include "SyncHDGroup.h"
#include "SyncHDLeaf.h"

namespace spdlog {
   class logger;
}
class SignContainer;

namespace bs {
   namespace sync {
      class Wallet;

      namespace hd {

         class Wallet : public QObject
         {
            Q_OBJECT
         public:
            using cb_scan_notify = std::function<void(Group *, bs::hd::Path::Elem wallet, bool isValid)>;
            using cb_scan_read_last = std::function<unsigned int(const std::string &walletId)>;
            using cb_scan_write_last = std::function<void(const std::string &walletId, unsigned int idx)>;

            Wallet(NetworkType, const std::string &walletId, const std::string &name
               , const std::string &desc, const std::shared_ptr<spdlog::logger> &logger = nullptr);
            Wallet(NetworkType, const std::string &walletId, const std::string &name
               , const std::string &desc, SignContainer *
               , const std::shared_ptr<spdlog::logger> &logger = nullptr);
            ~Wallet() override;

            Wallet(const Wallet&) = delete;
            Wallet& operator = (const Wallet&) = delete;
            Wallet(Wallet&&) = delete;
            Wallet& operator = (Wallet&&) = delete;

            void synchronize(const std::function<void()> &cbDone);
            bool isReady() const;

            std::vector<bs::wallet::EncryptionType> encryptionTypes() const;
            std::vector<SecureBinaryData> encryptionKeys() const;
            bs::wallet::KeyRank encryptionRank() const;
            bool isPrimary() const;
            NetworkType networkType() const { return netType_; }

            std::shared_ptr<Group> getGroup(bs::hd::CoinType ct) const;
            std::shared_ptr<Group> createGroup(bs::hd::CoinType ct, bool isExtOnly);
            void addGroup(const std::shared_ptr<Group> &group);
            size_t getNumGroups() const { return groups_.size(); }
            std::vector<std::shared_ptr<Group>> getGroups() const;
            virtual size_t getNumLeaves() const;
            std::vector<std::shared_ptr<bs::sync::Wallet>> getLeaves() const;
            std::shared_ptr<bs::sync::Wallet> getLeaf(const std::string &id) const;

            virtual std::string walletId() const;
            std::string name() const { return name_; }
            std::string description() const { return desc_; }

            void setUserId(const BinaryData &usedId);
            bool deleteRemotely();

            std::vector<std::string> registerWallet(
               const std::shared_ptr<ArmoryConnection> &, bool asNew = false);
            std::vector<std::string> setUnconfirmedTargets(void);

            void setArmory(const std::shared_ptr<ArmoryConnection> &);
            void trackChainAddressUse(const std::function<void(bs::sync::SyncState)> &);
            void startRescan();
            bs::hd::CoinType getXBTGroupType() const { return ((netType_ == NetworkType::MainNet)
               ? bs::hd::CoinType::Bitcoin_main : bs::hd::CoinType::Bitcoin_test); }

            void merge(const Wallet&);

            template<class U> void setCustomACT(
               const std::shared_ptr<ArmoryConnection> &armory)
            {
               auto& leaves = getLeaves();
               for (auto& leaf : leaves)
                  leaf->setCustomACT<U>(armory);
            }

         signals:
            void synchronized() const;
            void leafAdded(QString id);
            void leafDeleted(QString id);
            void scanComplete(const std::string &walletId);
            void metaDataChanged();

         private slots:
            void onGroupChanged();
            void onLeafAdded(QString id);
            void onLeafDeleted(QString id);

         protected:
            const std::string walletId_;
            const std::string name_, desc_;
            NetworkType    netType_ = NetworkType::MainNet;
            std::map<bs::hd::Path::Elem, std::shared_ptr<Group>>        groups_;
            mutable std::map<std::string, std::shared_ptr<bs::sync::Wallet>>  leaves_;
            mutable std::mutex   mtxGroups_;
            BinaryData        userId_;
            SignContainer  *  signContainer_;
            std::shared_ptr<ArmoryConnection>   armory_;
            std::shared_ptr<spdlog::logger>     logger_;
            std::vector<bs::wallet::EncryptionType>   encryptionTypes_{bs::wallet::EncryptionType::Password};
            std::vector<SecureBinaryData>          encryptionKeys_;
            std::pair<unsigned int, unsigned int>  encryptionRank_{ 1, 1 };

         private:
            std::unordered_set<std::string>  scannedLeaves_;

         public:
            unsigned containerId_ = UINT32_MAX;
         };


         class DummyWallet : public Wallet    // Just a container for old-style wallets
         {
         public:
            DummyWallet(const std::shared_ptr<spdlog::logger> &logger)
               : Wallet(NetworkType::Invalid, "Dummy", tr("Armory Wallets").toStdString()
                  , "", logger) {}

            size_t getNumLeaves() const override { return leaves_.size(); }

            void add(const std::shared_ptr<bs::sync::Wallet> wallet) {
               leaves_[wallet->walletId()] = wallet;
            }
         };
      }  //namespace hd
   }  //namespace sync
}  //namespace bs

#endif //BS_SYNC_HD_WALLET_H
