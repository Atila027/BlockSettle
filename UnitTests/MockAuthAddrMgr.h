#ifndef __MOCK_AUTH_ADDR_MGR_H__
#define __MOCK_AUTH_ADDR_MGR_H__

#include "Address.h"
#include "AuthAddressManager.h"
#include "BinaryData.h"


namespace spdlog {
   class logger;
}
class ArmoryConnection;

class MockAuthAddrMgr : public AuthAddressManager
{
   Q_OBJECT

public:
   MockAuthAddrMgr(const std::shared_ptr<spdlog::logger> &, const std::shared_ptr<ArmoryConnection> &);

   size_t getDefaultIndex() const override { return 0; }

   bool HaveAuthWallet() const override { return false; }
   bool HasAuthAddr() const override { return true; }
   ReadyError readyError() const override { return ReadyError::NoError; }

   bool CreateNewAuthAddress() override { return false; }
   void SubmitForVerification(BsClient *client, const bs::Address &) override {}
   bool RevokeAddress(const bs::Address &address) override { return true; }

   std::vector<bs::Address> GetVerifiedAddressList() const override { return addresses_; }

   void OnDisconnectedFromCeler() override {}
};

#endif // __MOCK_AUTH_ADDR_MGR_H__
