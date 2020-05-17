#ifndef BFXTTXOUT_H
#define BFXTTXOUT_H

#include <inttypes.h>
#include <string>

#include "bfxttokentxdata.h"

/**
 * @brief The BFXTTxOut class
 * A single vout entry in a transaction
 */
class BFXTTxOut
{
public:
    enum OutputType
    {
        NormalOutput = 0,
        OPReturn,
        NonStandard
    };

private:
    int64_t                      nValue;
    std::string                  scriptPubKeyHex;
    std::string                  scriptPubKeyAsm;
    std::vector<BFXTTokenTxData> tokens;
    std::string                  address;

    friend class BFXTTransaction;

public:
    BFXTTxOut();
    BFXTTxOut(int64_t nValueIn, const std::string& scriptPubKeyIn);
    void                   setNull();
    bool                   isNull() const;
    void                   importJsonData(const std::string& data);
    void                   importJsonData(const json_spirit::Value& parsedData);
    json_spirit::Value     exportDatabaseJsonData() const;
    void                   importDatabaseJsonData(const json_spirit::Value& data);
    int64_t                getValue() const;
    const std::string&     getScriptPubKeyHex() const;
    const BFXTTokenTxData& getToken(unsigned long index) const;
    BFXTTokenTxData&       getToken(unsigned long index);
    unsigned long          tokenCount() const;
    friend inline bool     operator==(const BFXTTxOut& lhs, const BFXTTxOut& rhs);
    std::string            getAddress() const;
    void                   setAddress(const std::string& Address);
    OutputType             getType() const;
    std::string            getScriptPubKeyAsm() const;
    void                   setNValue(const int64_t& value);
    void                   setScriptPubKeyHex(const std::string& value);
    void                   setScriptPubKeyAsm(const std::string& value);
    void                   __addToken(const BFXTTokenTxData& token);

    void __manualSet(int64_t NValue, std::string ScriptPubKeyHex, std::string ScriptPubKeyAsm,
                     std::vector<BFXTTokenTxData> Tokens, std::string Address);

    // clang-format off
    IMPLEMENT_SERIALIZE(
                        READWRITE(nValue);
                        READWRITE(scriptPubKeyHex);
                        READWRITE(scriptPubKeyAsm);
                        READWRITE(tokens);
                        READWRITE(address);
                       )
    // clang-format on
};

bool operator==(const BFXTTxOut& lhs, const BFXTTxOut& rhs)
{
    return (lhs.nValue == rhs.nValue && lhs.scriptPubKeyHex == rhs.scriptPubKeyHex &&
            lhs.scriptPubKeyAsm == rhs.scriptPubKeyAsm && lhs.tokens == rhs.tokens &&
            lhs.address == rhs.address);
}

#endif // BFXTTXOUT_H
