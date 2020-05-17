#include "bfxttokentxdata.h"
#include "bfxttools.h"

#include "base58.h"

void BFXTTokenTxData::setDivisibility(const uint64_t& value) { divisibility = value; }

void BFXTTokenTxData::setLockStatus(bool value) { lockStatus = static_cast<int>(value); }

void BFXTTokenTxData::setAggregationPolicy(const std::string& value) { aggregationPolicy = value; }

std::string BFXTTokenTxData::getTokenSymbol() const { return tokenSymbol; }

void BFXTTokenTxData::setTokenSymbol(const std::string& value) { tokenSymbol = value; }

BFXTTokenTxData::BFXTTokenTxData() { setNull(); }

void BFXTTokenTxData::setNull()
{
    tokenId.clear();
    amount       = 0;
    issueTxId    = 0;
    divisibility = 0;
    lockStatus   = 0;
    aggregationPolicy.clear();
}

void BFXTTokenTxData::setTokenId(const std::string& Str) { tokenId = Str; }

void BFXTTokenTxData::setIssueTxIdHex(const std::string& hex) { issueTxId.SetHex(hex); }

void BFXTTokenTxData::importJsonData(const std::string& data)
{
    try {
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(data, parsedData);
        importJsonData(parsedData);
    } catch (std::exception& ex) {
        printf("%s", ex.what());
        throw;
    }
}

void BFXTTokenTxData::importJsonData(const json_spirit::Value& data)
{
    try {
        setTokenId(BFXTTools::GetStrField(data.get_obj(), "tokenId"));
        setIssueTxIdHex(BFXTTools::GetStrField(data.get_obj(), "issueTxid"));
        amount            = BFXTTools::GetUint64Field(data.get_obj(), "amount");
        divisibility      = BFXTTools::GetUint64Field(data.get_obj(), "divisibility");
        lockStatus        = (int)BFXTTools::GetBoolField(data.get_obj(), "lockStatus");
        aggregationPolicy = BFXTTools::GetStrField(data.get_obj(), "aggregationPolicy");
    } catch (std::exception& ex) {
        printf("%s", ex.what());
        throw;
    }
}

json_spirit::Value BFXTTokenTxData::exportDatabaseJsonData() const
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("tokenId", getTokenId()));
    root.push_back(json_spirit::Pair("issueTxid", getIssueTxId().ToString()));
    root.push_back(json_spirit::Pair("amount", ToString(amount)));
    root.push_back(json_spirit::Pair("divisibility", divisibility));
    root.push_back(json_spirit::Pair("lockStatus", static_cast<bool>(lockStatus)));
    root.push_back(json_spirit::Pair("aggregationPolicy", aggregationPolicy));

    return json_spirit::Value(root);
}

void BFXTTokenTxData::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    setTokenId(BFXTTools::GetStrField(data.get_obj(), "tokenId"));
    setIssueTxIdHex(BFXTTools::GetStrField(data.get_obj(), "issueTxid"));
    amount            = BFXTTools::GetBFXTIntField(data.get_obj(), "amount");
    divisibility      = BFXTTools::GetUint64Field(data.get_obj(), "divisibility");
    lockStatus        = (int)BFXTTools::GetBoolField(data.get_obj(), "lockStatus");
    aggregationPolicy = BFXTTools::GetStrField(data.get_obj(), "aggregationPolicy");
}

std::string BFXTTokenTxData::getTokenId() const { return tokenId; }

BFXTInt BFXTTokenTxData::getAmount() const { return FromString<BFXTInt>(amount); }

void BFXTTokenTxData::setAmount(const BFXTInt& value) { amount = value; }

uint64_t BFXTTokenTxData::getDivisibility() const { return divisibility; }

uint256 BFXTTokenTxData::getIssueTxId() const { return issueTxId; }

bool BFXTTokenTxData::getLockStatus() const { return static_cast<bool>(lockStatus); }

const std::string& BFXTTokenTxData::getAggregationPolicy() const { return aggregationPolicy; }
