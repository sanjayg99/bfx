#ifndef BFXTTXIN_H
#define BFXTTXIN_H

#include "bfxtoutpoint.h"
#include "bfxttokentxdata.h"
#include "bfxttools.h"

#include <string>

/**
 * @brief The BFXTTxIn class
 * A single vin entry in a transaction
 */
class BFXTTxIn
{
    BFXTOutPoint                 prevout;
    std::string                  scriptSigHex;
    uint64_t                     nSequence;
    std::vector<BFXTTokenTxData> tokens;

    friend class BFXTTransaction;

public:
    BFXTTxIn();
    void                   setNull();
    void                   importJsonData(const std::string& data);
    void                   importJsonData(const json_spirit::Value& parsedData);
    json_spirit::Value     exportDatabaseJsonData() const;
    void                   importDatabaseJsonData(const json_spirit::Value& data);
    BFXTOutPoint           getOutPoint() const;
    BFXTOutPoint           getPrevout() const;
    void                   setPrevout(const BFXTOutPoint& value);
    std::string            getScriptSigHex() const;
    void                   setScriptSigHex(const std::string& s);
    uint64_t               getSequence() const;
    const BFXTTokenTxData& getToken(unsigned long index) const;
    unsigned long          getNumOfTokens() const;
    void                   __addToken(const BFXTTokenTxData& token);
    void                   setSequence(const uint64_t& value);
    friend inline bool     operator==(const BFXTTxIn& lhs, const BFXTTxIn& rhs);

    // clang-format off
    IMPLEMENT_SERIALIZE(
                        READWRITE(prevout);
                        READWRITE(scriptSigHex);
                        READWRITE(nSequence);
                        READWRITE(tokens);
                       )
    // clang-format on
};

bool operator==(const BFXTTxIn& lhs, const BFXTTxIn& rhs)
{
    return (lhs.prevout == rhs.prevout && lhs.scriptSigHex == rhs.scriptSigHex &&
            lhs.nSequence == rhs.nSequence && lhs.tokens == rhs.tokens);
}

#endif // BFXTTXIN_H
