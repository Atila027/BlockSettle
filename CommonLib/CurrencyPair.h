#ifndef __CURRENCY_PAIR_H__
#define __CURRENCY_PAIR_H__

#include <string>
#include <CommonLib_export.h>

class CommonLib_EXPORT CurrencyPair
{
public:
   CurrencyPair(const std::string& pairString);

   const std::string &NumCurrency() const { return numCurrency_; }
   const std::string &DenomCurrency() const { return denomCurrency_; }
   const std::string &ContraCurrency(const std::string &cur);

private:
   std::string numCurrency_;
   std::string denomCurrency_;
   std::string invalidCurrency_;
};

#endif // __CURRENCY_PAIR_H__
