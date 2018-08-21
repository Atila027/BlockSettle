#ifndef __ARMORY_CONNECTION_H__
#define __ARMORY_CONNECTION_H__

#include <atomic>
#include <chrono>
#include <functional>
#include <memory>
#include <thread>
#include <unordered_map>
#include <unordered_set>

#include <QProcess>
#include <QMutex>
#include <spdlog/logger.h>

#include "Address.h"
#include "ArmorySettings.h"
#include "AsyncClient.h"
#include "CacheFile.h"


class ArmoryConnection;

class ArmoryCallback : public RemoteCallback
{
public:
   ArmoryCallback(ArmoryConnection *conn, const std::shared_ptr<spdlog::logger> &logger)
      : RemoteCallback(), connection_(conn), logger_(logger) {}
   virtual ~ArmoryCallback(void) noexcept = default;

   void run(BDMAction action, void* ptr, int block = 0) override;
   void progress(BDMPhase phase,
      const vector<string> &walletIdVec,
      float progress, unsigned secondsRem,
      unsigned progressNumeric) override;

   void socketStatus(bool status) override;
   std::shared_future<bool> connFuture();

private:
   ArmoryConnection * connection_;
   std::shared_ptr<spdlog::logger>  logger_;
   std::promise<bool>   connected_;
};

class ArmoryConnection : public QObject
{
   friend class ArmoryCallback;
   Q_OBJECT
public:
   enum class State : uint8_t {
      Offline,
      Connected,
      Scanning,
      Error,
      Closing,
      Ready
   };

   using ReqIdType = unsigned int;

public:
   ArmoryConnection(const std::shared_ptr<spdlog::logger> &, const std::string &txCacheFN);
   ~ArmoryConnection() noexcept;

   State state() const { return state_; }
   std::vector<ClientClasses::LedgerEntry> getZCentries(ReqIdType) const;

   void setupConnection(const ArmorySettings &);
   bool goOnline();

   bool broadcastZC(const BinaryData& rawTx);

   unsigned int topBlock() const;

   std::string registerWallet(std::shared_ptr<AsyncClient::BtcWallet> &, const std::string &walletId
      , const std::vector<BinaryData> &addrVec, std::function<void()>, bool asNew = false);
   bool getWalletsHistory(const std::vector<std::string> &walletIDs
      , std::function<void (std::vector<ClientClasses::LedgerEntry>)>);

   bool getLedgerDelegateForAddress(const std::string &walletId, const bs::Address &
      , std::function<void(AsyncClient::LedgerDelegate)>);
   bool getLedgerDelegatesForAddresses(const std::string &walletId, const std::vector<bs::Address>
      , std::function<void(std::map<bs::Address, AsyncClient::LedgerDelegate>)>);
   bool getWalletsLedgerDelegate(std::function<void(AsyncClient::LedgerDelegate)>);

   bool getTxByHash(const BinaryData &hash, std::function<void(Tx)>);
   bool getTXsByHash(const std::set<BinaryData> &hashes, std::function<void(std::vector<Tx>)>);

   bool estimateFee(unsigned int nbBlocks, std::function<void(float)>);

   bool isTransactionVerified(const ClientClasses::LedgerEntry &) const;
   bool isTransactionVerified(uint32_t blockNum) const;
   bool isTransactionConfirmed(const ClientClasses::LedgerEntry &) const;
   unsigned int getConfirmationsNumber(const ClientClasses::LedgerEntry &item) const;
   unsigned int getConfirmationsNumber(uint32_t blockNum) const;

signals:
   void stateChanged(ArmoryConnection::State) const;
   void connectionError(QString) const;
   void prepareConnection(NetworkType, std::string host, std::string port) const;
   void progress(BDMPhase, float progress, unsigned int secondsRem, unsigned int numProgress) const;
   void newBlock(unsigned int height) const;
   void zeroConfReceived(unsigned int) const;
   void refresh(std::vector<BinaryData> ids) const;
   void nodeStatus(NodeStatus, bool segWitEnabled, RpcStatus) const;
   void txBroadcastError(QString txHash, QString error) const;
   void error(QString errorStr, QString extraMsg) const;

private:
   void registerBDV(NetworkType);
   void setState(State);
   ReqIdType setZC(const std::vector<ClientClasses::LedgerEntry> &);
   void onRefresh(std::vector<BinaryData>);

   void stopServiceThreads();

   bool addGetTxCallback(const BinaryData &hash, const std::function<void(Tx)> &);  // returns true if hash exists
   void callGetTxCallbacks(const BinaryData &hash, const Tx &);

private:
   std::shared_ptr<spdlog::logger>  logger_;
   std::shared_ptr<AsyncClient::BlockDataViewer>   bdv_;
   std::shared_ptr<ArmoryCallback>  cbRemote_;
   State          state_ = State::Offline;
   TxCacheFile    txCache_;

   std::atomic_bool  regThreadRunning_;
   std::atomic_bool  connThreadRunning_;
   std::atomic_bool  maintThreadRunning_;
   std::atomic<ReqIdType>  reqIdSeq_;

   const std::chrono::duration<double> zcPersistenceTimeout_;
   struct ZCData {
      std::chrono::system_clock::time_point     received;
      std::vector<ClientClasses::LedgerEntry>   entries;
   };
   std::unordered_map<ReqIdType, ZCData>  zcData_;
   mutable std::atomic_flag      zcLock_ = ATOMIC_FLAG_INIT;
   std::condition_variable       zcMaintCV_;
   mutable std::mutex            zcMaintMutex_;

   std::atomic_bool              isOnline_;
   std::unordered_map<std::string, std::function<void()>>   preOnlineRegIds_;

   mutable std::atomic_flag      txCbLock_ = ATOMIC_FLAG_INIT;
   std::map<BinaryData, std::vector<std::function<void(Tx)>>>   txCallbacks_;
};

#endif // __ARMORY_CONNECTION_H__
