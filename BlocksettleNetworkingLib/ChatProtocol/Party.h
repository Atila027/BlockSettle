#ifndef Party_h__
#define Party_h__

#include <memory>
#include <string>

#include "chat.pb.h"

namespace Chat
{

   class Party
   {
   public:
      Party();
      Party(const std::string& id, const PartyType& partyType, const PartySubType& partySubType);

      virtual ~Party() {}

      virtual std::string id() const { return id_; }
      virtual void setId(std::string val) { id_ = val; }

      virtual Chat::PartyType partyType() const { return partyType_; }
      virtual void setPartyType(Chat::PartyType val) { partyType_ = val; }

      virtual Chat::PartySubType partySubType() const { return partySubType_; }
      virtual void setPartySubType(Chat::PartySubType val) { partySubType_ = val; }

   private:
      std::string id_;
      PartyType partyType_;
      PartySubType partySubType_;
   };

   using PartyPtr = std::shared_ptr<Party>;
}

#endif // Party_h__
