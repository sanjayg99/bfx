#ifndef BFXTTOKENMINIMALMETADATA_H
#define BFXTTOKENMINIMALMETADATA_H

#include "uint256.h"
#include <string>
#include <vector>

class BFXTTokenMinimalMetaData
{
protected:
    std::string tokenId;
    uint256     issuanceTxId;
    uint64_t    divisibility;
    bool        lockStatus;
    std::string aggregationPolicy;

public:
    BFXTTokenMinimalMetaData();
    void               setNull();
    void               setTokenId(const std::string& Str);
    void               setIssuanceTxIdHex(const std::string& hex);
    const std::string& getTokenId() const;
    std::string        getIssuanceTxIdHex() const;
    uint64_t           getDivisibility() const;
    bool               getLockStatus() const;
    const std::string& getAggregationPolicy() const;
    uint256            getIssuanceTxId() const;
    void               setIssuanceTxId(const uint256& value);

    friend inline bool operator==(const BFXTTokenMinimalMetaData& lhs,
                                  const BFXTTokenMinimalMetaData& rhs);
};

bool operator==(const BFXTTokenMinimalMetaData& lhs, const BFXTTokenMinimalMetaData& rhs)
{
    return (lhs.getTokenId() == rhs.getTokenId() && lhs.getIssuanceTxId() == rhs.getIssuanceTxId() &&
            lhs.getDivisibility() == rhs.getDivisibility() &&
            lhs.getLockStatus() == rhs.getLockStatus() &&
            lhs.getAggregationPolicy() == rhs.getAggregationPolicy());
}

#endif // BFXTTOKENMINIMALMETADATA_H
