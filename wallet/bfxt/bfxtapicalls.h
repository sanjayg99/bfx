#ifndef BFXTAPICALLS_H
#define BFXTAPICALLS_H

#include "bfxttokenmetadata.h"
#include "bfxttools.h"
#include "bfxttransaction.h"

class BFXTAPICalls
{
public:
    BFXTAPICalls();
    static bool     RetrieveData_AddressContainsBFXTTokens(const std::string& address, bool testnet);
    static uint64_t RetrieveData_TotalNeblsExcludingBFXT(const std::string& address, bool testnet);
    static BFXTTokenMetaData RetrieveData_BFXTTokensMetaData(const std::string& tokenId,
                                                             const std::string& tx, int outputIndex,
                                                             bool testnet);
    static const long        BFXT_CONNECTION_TIMEOUT = 10;
    static BFXTTransaction   RetrieveData_TransactionInfo(const std::string& txHash, bool testnet);
    static std::string       RetrieveData_TransactionInfo_Str(const std::string& txHash, bool testnet);
};

#endif // BFXTAPICALLS_H
