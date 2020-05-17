#include "bfxttools.h"

#include "util.h"

const std::string BFXTTools::NTPAPI_base_url_mainnet_local = "https://bfxtnode.nebl.io/bfxt/";
const std::string BFXTTools::NTPAPI_base_url_testnet_local = "https://bfxtnode.nebl.io/testnet/bfxt/";

const std::string BFXTTools::NTPAPI_base_url_mainnet_remote = "https://bfxtnode.nebl.io/bfxt/";
const std::string BFXTTools::NTPAPI_base_url_testnet_remote = "https://bfxtnode.nebl.io/testnet/bfxt/";

const std::string BFXTTools::NTPAPI_addressInfo     = "addressinfo/";
const std::string BFXTTools::NTPAPI_transactionInfo = "transactioninfo/";
const std::string BFXTTools::NTPAPI_tokenId         = "tokenid/";
const std::string BFXTTools::NTPAPI_tokenMetaData   = "tokenmetadata/";
const std::string BFXTTools::NTPAPI_stakeHolders    = "stakeholders/";
const std::string BFXTTools::NTPAPI_sendTokens      = "sendtoken/";

const std::string BFXTTools::EXPLORER_base_url_testnet = "https://testnet-explorer.nebl.io/";
const std::string BFXTTools::EXPLORER_base_url_mainnet = "https://explorer.nebl.io/";

const std::string BFXTTools::EXPLORER_tokenInfo       = "token/";
const std::string BFXTTools::EXPLORER_transactionInfo = "tx/";

BFXTTools::BFXTTools() {}

std::string BFXTTools::GetStrField(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_str();
}

bool BFXTTools::GetBoolField(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_bool();
}

uint64_t BFXTTools::GetUint64Field(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_uint64();
}

BFXTInt BFXTTools::GetBFXTIntField(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return FromString<BFXTInt>(val.get_str());
}

bool BFXTTools::GetFieldExists(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return !(val == json_spirit::Value::null);
}

int64_t BFXTTools::GetInt64Field(const json_spirit::Object& data, const std::string& fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_int64();
}

std::string BFXTTools::GetURL_APIBase(bool testnet)
{
#ifdef BFX_REST
    return (testnet ? NTPAPI_base_url_testnet_local : NTPAPI_base_url_mainnet_local);
#else
    return (testnet ? NTPAPI_base_url_testnet_remote : NTPAPI_base_url_mainnet_remote);
#endif
}

json_spirit::Array BFXTTools::GetArrayField(const json_spirit::Object& data,
                                            const std::string&         fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_array();
}

json_spirit::Object BFXTTools::GetObjectField(const json_spirit::Object& data,
                                              const std::string&         fieldName)
{
    json_spirit::Value val;
    val = json_spirit::find_value(data, fieldName);
    return val.get_obj();
}

std::string BFXTTools::GetURL_TransactionInfo(const std::string& txHash, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_transactionInfo + txHash;
}

std::string BFXTTools::GetURL_TokenID(const std::string& tokenSymbol, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_tokenId + tokenSymbol;
}

std::string BFXTTools::GetURL_TokenMetaData(const std::string& tokenID, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_tokenMetaData + tokenID;
}

std::string BFXTTools::GetURL_TokenUTXOMetaData(const std::string& tokenID, const std::string& txHash,
                                                unsigned long outputIndex, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_tokenMetaData + tokenID + "/" + txHash + ":" +
           ToString(outputIndex);
}

std::string BFXTTools::GetURL_StakeHolders(const std::string& tokenID, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_stakeHolders + tokenID;
}

std::string BFXTTools::GetURL_AddressInfo(const std::string& address, bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_addressInfo + address;
}

std::string BFXTTools::GetURL_SendTokens(bool testnet)
{
    return GetURL_APIBase(testnet) + NTPAPI_sendTokens;
}

std::string BFXTTools::GetURL_ExplorerBase(bool testnet)
{
    return (testnet ? EXPLORER_base_url_testnet : EXPLORER_base_url_mainnet);
}

std::string BFXTTools::GetURL_ExplorerTokenInfo(const std::string& tokenId, bool testnet)
{
    return GetURL_ExplorerBase(testnet) + EXPLORER_tokenInfo + tokenId;
}

std::string BFXTTools::GetURL_ExplorerTransactionInfo(const std::string& txId, bool testnet)
{
    return GetURL_ExplorerBase(testnet) + EXPLORER_transactionInfo + txId;
}
