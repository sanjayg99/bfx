#ifndef BFXTSCRIPT_H
#define BFXTSCRIPT_H

#include "boost/algorithm/string.hpp"
#include "crypto_highlevel.h"
#include "json_spirit.h"
#include <bitset>
#include <boost/algorithm/hex.hpp>
#include <boost/dynamic_bitset.hpp>
#include <boost/multiprecision/cpp_int.hpp>
#include <boost/regex.hpp>
#include <cmath>
#include <limits>
#include <memory>
#include <set>
#include <string>
#include <vector>

using BFXTInt = boost::multiprecision::cpp_int;

// You should NEVER change these without changing the database version
// These go to the database for verifying issuance transactions duplication
typedef uint32_t          BFXTTransactionType;
const BFXTTransactionType BFXTTxType_UNKNOWN  = 0;
const BFXTTransactionType BFXTTxType_NOT_BFXT = 1;
const BFXTTransactionType BFXTTxType_ISSUANCE = 2;
const BFXTTransactionType BFXTTxType_TRANSFER = 3;
const BFXTTransactionType BFXTTxType_BURN     = 4;

constexpr const char* METADATA_SER_FIELD__VERSION               = "SerializationVersion";
constexpr const char* METADATA_SER_FIELD__TARGET_PUBLIC_KEY_HEX = "TargetPubKeyHex";
constexpr const char* METADATA_SER_FIELD__SOURCE_PUBLIC_KEY_HEX = "SourcePubKeyHex";
constexpr const char* METADATA_SER_FIELD__CIPHER_BASE64         = "Cipher64";

const std::string  HexBytesRegexStr("^([0-9a-fA-F][0-9a-fA-F])+$");
const boost::regex HexBytexRegex(HexBytesRegexStr);

const BFXTInt BFXTMaxAmount = std::numeric_limits<int64_t>::max();

class CKey;
class CTransaction;
class BFXTSendTokensOneRecipientData;

struct RawBFXTMetadataBeforeSend
{
    RawBFXTMetadataBeforeSend(std::string Metadata = "", bool DoEncrypt = false)
    {
        metadata = std::move(Metadata);
        encrypt  = DoEncrypt;
    }
    bool        encrypt = false;
    std::string metadata;

    /**
     * @param bfxtmetadata
     * @param wtxNew
     * @param bfxtTxData
     * @return If RawBFXTMetadataBeforeSend has encrypt set to true, the message will be encrypted and
     * returned. Otherwise, it'll be returned as is
     */
    std::string
    applyMetadataEncryption(const CTransaction&                                wtxNew,
                            const std::vector<BFXTSendTokensOneRecipientData>& recipients) const;
};

class BFXTScript
{
    std::string parsedScriptHex;

public:
    enum TxType
    {
        TxType_None = 0,
        TxType_Issuance,
        TxType_Transfer,
        TxType_Burn
    };

protected:
    std::string headerBin;
    int         protocolVersion;
    std::string opCodeBin;

    TxType txType                  = TxType_None;
    bool   enableOpReturnSizeCheck = true;

    void setCommonParams(std::string Header, int ProtocolVersion, std::string OpCodeBin,
                         std::string scriptHex);

public:
    void setEnableOpReturnSizeCheck(bool value = true);
    bool isOpReturnSizeCheckEnabled() const;

    struct TransferInstruction
    {
        TransferInstruction()
        {
            amount       = 0;
            skipInput    = false;
            outputIndex  = -1;
            firstRawByte = 1;
        }
        unsigned char firstRawByte;
        // transfer instructions act on inputs in order until they're empty, so instruction 0 will act on
        // input 0, and instruction 1 will act on input 0, etc... until input 0 is empty, or a skip
        // instruction is given to move to the next input
        bool         skipInput;
        unsigned int outputIndex;

        std::string rawAmount;
        BFXTInt     amount;
    };

    struct IssuanceFlags
    {
        unsigned int divisibility;
        bool         locked; // no more issuing allowed
        enum AggregationPolicy
        {
            AggregationPolicy_Aggregatable,
            AggregationPolicy_NonAggregatable,
            AggregationPolicy_Unknown
        };
        AggregationPolicy aggregationPolicy;

        static const std::string AggregationPolicy_Aggregatable_Str;
        static const std::string AggregationPolicy_NonAggregatable_Str;

        static IssuanceFlags ParseIssuanceFlag(uint8_t flags);
        uint8_t              convertToByte() const
        {
            if (this->divisibility > 7) {
                throw std::runtime_error("Divisibility cannot be larger than 7");
            }

            std::bitset<3> divisibility_bits(static_cast<uint8_t>(this->divisibility));
            std::string    lockStatusStrBits = (this->locked ? "1" : "0");
            std::string    aggrPolicyStrBits;
            if (this->aggregationPolicy ==
                IssuanceFlags::AggregationPolicy::AggregationPolicy_Aggregatable) {
                aggrPolicyStrBits = "00";
            } else if (this->aggregationPolicy ==
                       IssuanceFlags::AggregationPolicy::AggregationPolicy_NonAggregatable) {
                aggrPolicyStrBits = "10";
            } else {
                throw std::runtime_error("Unknown aggregation policy:" +
                                         std::to_string(static_cast<int>(this->aggregationPolicy)));
            }
            std::string issuanceFlagsBitsStr =
                divisibility_bits.to_string() + lockStatusStrBits + aggrPolicyStrBits + "00";
            if (issuanceFlagsBitsStr.size() != 8) {
                throw std::runtime_error("Error while constructing issuance flags");
            }
            std::bitset<8> issuanceFlagsBits(issuanceFlagsBitsStr);
            return static_cast<uint8_t>(issuanceFlagsBits.to_ulong());
        }
    };

    static std::string TransferInstructionToBinScript(const TransferInstruction& inst);

    virtual std::string calculateScriptBin() const = 0;
    /**
     * @brief getBFXTIndices
     * @return a list of all BFXT output indices in this script
     */
    virtual std::set<unsigned int> getBFXTOutputIndices() const = 0;

    virtual std::string getHexMetadata() const      = 0;
    virtual std::string getRawMetadata() const      = 0;
    virtual std::string getInflatedMetadata() const = 0;

    virtual ~BFXTScript() = default;
    static uint64_t    CalculateMetadataSize(const std::string& op_code_bin);
    static TxType      CalculateTxType(const std::string& op_code_bin);
    static TxType      CalculateTxTypeBFXTv3(const std::string& op_code_bin);
    static uint64_t    CalculateAmountSize(uint8_t firstChar);
    static BFXTInt     ParseAmountFromLongEnoughString(const std::string& BinAmountStartsAtByte0,
                                                       int&               rawSize);
    static std::string ParseOpCodeFromLongEnoughString(const std::string& BinOpCodeStartsAtByte0);
    static std::string ParseMetadataFromLongEnoughString(const std::string& BinMetadataStartsAtByte0,
                                                         const std::string& op_code_bin,
                                                         const std::string& wholeScriptHex = "");
    static std::string
    ParseBFXTv3MetadataFromLongEnoughString(const std::string& BinMetadataSizeStartsAtByte0,
                                            const std::string& wholeScriptHex = "");
    static std::string
    ParseTokenSymbolFromLongEnoughString(const std::string& BinTokenSymbolStartsAtByte0);
    static std::vector<TransferInstruction>
    ParseTransferInstructionsFromLongEnoughString(const std::string& BinInstructionsStartFromByte0,
                                                  int&               totalRawSize);
    static std::vector<TransferInstruction>
    ParseBFXTv3TransferInstructionsFromLongEnoughString(const std::string& BinInstructionsStartFromByte0,
                                                        int&               totalRawSize);

    std::string getHeader() const;
    std::string getOpCodeBin() const;
    TxType      getTxType() const;

    static std::shared_ptr<BFXTScript> ParseScript(const std::string& scriptHex);
    std::string                        getParsedScriptHex() const;
    int                                getProtocolVersion() const;

    static BFXTInt     BFXTAmountHexToNumber(std::string hexVal);
    static BFXTInt     GetTrailingZeros(const BFXTInt& num);
    static std::string NumberToHexBFXTAmount(const BFXTInt& num, bool caps = false);

    static std::string        GetMetadataAsString(const BFXTScript*   bfxtscript,
                                                  const CTransaction& tx) noexcept;
    static json_spirit::Value GetMetadataAsJson(const BFXTScript*   bfxtscript,
                                                const CTransaction& tx) noexcept;

    static bool IsBFXTTokenSymbolValid(const std::string& symbol);
    static bool IsTokenSymbolCharValid(const char c);

    [[nodiscard]] static std::string EncryptMetadataWithEphemeralKey(
        const StringViewT data, const CKey& publicKey, CHL::EncryptionAlgorithm encAlgo,
        CHL::AuthKeyRatchetAlgorithm ratchetAlgo, CHL::AuthenticationAlgorithm authAlgo);

    [[nodiscard]] static std::string EncryptMetadata(const StringViewT data, const CKey& privateKey,
                                                     const CKey&                  publicKey,
                                                     CHL::EncryptionAlgorithm     encAlgo,
                                                     CHL::AuthKeyRatchetAlgorithm ratchetAlgo,
                                                     CHL::AuthenticationAlgorithm authAlgo);
    [[nodiscard]] static std::string DecryptMetadata(const StringViewT data, const CKey& privateKey);

    [[nodiscard]] static std::string EncryptMetadataBeforeSend(const StringViewT bfxtmetadata,
                                                               const CKey&       inputPrivateKey,
                                                               const StringViewT recipientAddress);
};

template <typename Bitset>
void set_in_range(Bitset& b, uint8_t value, int from, int to)
{
    for (int i = from; i < to; ++i, value >>= 1) {
        b[i] = (value & 1);
    }
}

#endif // BFXTSCRIPT_H
