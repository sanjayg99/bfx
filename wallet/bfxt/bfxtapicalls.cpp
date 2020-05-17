#include "bfxtapicalls.h"
#include "bfxttransaction.h"

BFXTAPICalls::BFXTAPICalls() {}

bool BFXTAPICalls::RetrieveData_AddressContainsBFXTTokens(const std::string& address, bool testnet)
{
    try {
        std::string addressNTPInfoURL = BFXTTools::GetURL_AddressInfo(address, testnet);
        std::string ntpData =
            cURLTools::GetFileFromHTTPS(addressNTPInfoURL, BFXT_CONNECTION_TIMEOUT, false);
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(ntpData, parsedData);
        json_spirit::Array utxosArray = BFXTTools::GetArrayField(parsedData.get_obj(), "utxos");
        for (const auto& ob : utxosArray) {
            json_spirit::Array tokensArray = BFXTTools::GetArrayField(ob.get_obj(), "tokens");
            if (tokensArray.size() > 0) {
                return true;
            }
        }
        return false;
    } catch (std::exception& ex) {
        printf("%s\n", ex.what());
        throw;
    }
}

uint64_t BFXTAPICalls::RetrieveData_TotalNeblsExcludingBFXT(const std::string& address, bool testnet)
{
    try {
        std::string addressNTPInfoURL = BFXTTools::GetURL_AddressInfo(address, testnet);
        std::string ntpData =
            cURLTools::GetFileFromHTTPS(addressNTPInfoURL, BFXT_CONNECTION_TIMEOUT, false);
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(ntpData, parsedData);
        json_spirit::Array utxosArray = BFXTTools::GetArrayField(parsedData.get_obj(), "utxos");
        uint64_t           totalSats  = 0;
        for (const auto& ob : utxosArray) {
            json_spirit::Array tokensArray = BFXTTools::GetArrayField(ob.get_obj(), "tokens");
            if (tokensArray.size() == 0) {
                totalSats += BFXTTools::GetUint64Field(ob.get_obj(), "value");
            }
        }
        return totalSats;
    } catch (std::exception& ex) {
        printf("%s\n", ex.what());
        throw;
    }
}

BFXTTokenMetaData BFXTAPICalls::RetrieveData_BFXTTokensMetaData(const std::string& tokenId,
                                                                const std::string& tx, int outputIndex,
                                                                bool testnet)
{
    try {
        std::string bfxtMetaDataURL =
            BFXTTools::GetURL_TokenUTXOMetaData(tokenId, tx, outputIndex, testnet);
        std::string ntpData =
            cURLTools::GetFileFromHTTPS(bfxtMetaDataURL, BFXT_CONNECTION_TIMEOUT, false);
        BFXTTokenMetaData metadata;
        metadata.importRestfulAPIJsonData(ntpData);
        return metadata;
    } catch (std::exception& ex) {
        printf("%s\n", ex.what());
        throw;
    }
}

BFXTTransaction BFXTAPICalls::RetrieveData_TransactionInfo(const std::string& txHash, bool testnet)
{
    std::string     url     = BFXTTools::GetURL_TransactionInfo(txHash, testnet);
    std::string     ntpData = cURLTools::GetFileFromHTTPS(url, BFXT_CONNECTION_TIMEOUT, false);
    BFXTTransaction tx;
    tx.importJsonData(ntpData);
    return tx;
}

std::string BFXTAPICalls::RetrieveData_TransactionInfo_Str(const std::string& txHash, bool testnet)
{
    std::string url     = BFXTTools::GetURL_TransactionInfo(txHash, testnet);
    std::string ntpData = cURLTools::GetFileFromHTTPS(url, BFXT_CONNECTION_TIMEOUT, false);
    return ntpData;
}
