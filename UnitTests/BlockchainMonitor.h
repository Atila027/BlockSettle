#ifndef __BLOCKCHAIN_MONITOR_H__
#define __BLOCKCHAIN_MONITOR_H__

#include <atomic>
#include <memory>
#include <vector>
#include <QObject>
#include "ArmoryObject.h"
#include "BinaryData.h"
#include "ClientClasses.h"


namespace bs {
   namespace sync {
      class Wallet;
   }
}
class ArmoryConnection;

class BlockchainMonitor : public QObject
{
   Q_OBJECT

public:
   BlockchainMonitor(const std::shared_ptr<ArmoryObject> &);

   uint32_t waitForNewBlocks(uint32_t targetHeight = 0);
   bool waitForZC(double timeoutInSec = 30) { return waitForFlag(receivedZC_, timeoutInSec); }
   std::vector<bs::TXEntry> getZCentries() const { return zcEntries_; }

   static bool waitForFlag(std::atomic_bool &, double timeoutInSec = 30);
   static bool waitForWalletReady(const std::shared_ptr<bs::sync::Wallet> &, double timeoutInSec = 30);

private:
   std::shared_ptr<ArmoryObject>   armory_;
   std::atomic_bool  receivedNewBlock_;
   std::atomic_bool  receivedZC_;
   std::vector<bs::TXEntry>   zcEntries_;
};

#endif // __BLOCKCHAIN_MONITOR_H__
