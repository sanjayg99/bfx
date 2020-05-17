#include "bfxttokenminimalmetadata.h"

void BFXTTokenMinimalMetaData::setIssuanceTxId(const uint256& value) { issuanceTxId = value; }

BFXTTokenMinimalMetaData::BFXTTokenMinimalMetaData() { setNull(); }

void BFXTTokenMinimalMetaData::setNull()
{
    tokenId.clear();
    issuanceTxId = 0;
    divisibility = -1;
    lockStatus   = false;
    aggregationPolicy.clear();
}

void BFXTTokenMinimalMetaData::setTokenId(const std::string& Str) { tokenId = Str; }

const std::string& BFXTTokenMinimalMetaData::getTokenId() const { return tokenId; }

std::string BFXTTokenMinimalMetaData::getIssuanceTxIdHex() const { return issuanceTxId.ToString(); }

uint64_t BFXTTokenMinimalMetaData::getDivisibility() const { return divisibility; }

bool BFXTTokenMinimalMetaData::getLockStatus() const { return lockStatus; }

const std::string& BFXTTokenMinimalMetaData::getAggregationPolicy() const { return aggregationPolicy; }

void BFXTTokenMinimalMetaData::setIssuanceTxIdHex(const std::string& hex) { issuanceTxId.SetHex(hex); }

uint256 BFXTTokenMinimalMetaData::getIssuanceTxId() const { return issuanceTxId; }
