#ifndef BFXTWALLET_H
#define BFXTWALLET_H

#include "ThreadSafeHashMap.h"
#include "curltools.h"
#include "bfxt/bfxtoutpoint.h"
#include "bfxt/bfxttokenmetadata.h"
#include "bfxt/bfxttransaction.h"
#include "json/json_spirit.h"

#include <unordered_map>

class COutput;
class CWalletTx;

class BFXTWallet : public boost::enable_shared_from_this<BFXTWallet>
{
    bool retrieveFullMetadata = true;

    int minConfirmations = -1; // default value -1 will not make any constraints
    int maxConfirmations = -1; // default value -1 will not make any constraints

    // base58 token id vs BFXT token meta data object
    std::unordered_map<std::string, BFXTTokenMetaData> tokenInformation;
    // transaction with output index
    std::unordered_map<BFXTOutPoint, BFXTTransaction> walletOutputsWithTokens;
    // wallet balances
    std::map<std::string, BFXTInt> balances;
    // map from token id vs icon image data
    ThreadSafeHashMap<std::string, std::string> tokenIcons;

    bool updateBalance;

    // remains false until a successful attempt to retrieve tokens is over (for display purposes)
    bool everSucceededInLoadingTokens;

    void __getOutputs();
    void __RecalculateTokensBalances();

    // it's very important to use shared_from_this() here to guarantee thread-safety
    // if the shared_ptr's content gets deleted before the thread gets executed, it will lead to a
    // segfault passing a shared_ptr guarantees that the object will survive until the end
    static void __asyncDownloadAndSetIcon(std::string IconURL, std::string tokenId,
                                          boost::shared_ptr<BFXTWallet> wallet);

    static std::string __downloadIcon(const std::string& IconURL);
    static void        AddOutputToWalletBalance(const BFXTTransaction& tx, int outputIndex,
                                                std::map<std::string, BFXTInt>& balancesTable);
    // returns true if removed
    bool                removeOutputIfSpent(const BFXTOutPoint& output, const CWalletTx& neblTx);
    void                scanSpentTransactions();
    static BFXTOutPoint ConvertNeblOutputToBFXT(const COutput& output);

    // when scanning the bfx wallet, this is the number of relevant transactions found
    int64_t lastTxCount;
    int64_t lastOutputsCount;

    static const std::string ICON_ERROR_CONTENT;

    /// this function returns minimal information of the token from the tx (e.g., no icon)
    static BFXTTokenMetaData GetMinimalMetadataInfoFromTxData(const BFXTTokenTxData& tokenTx);

public:
    BFXTWallet();
    void                                  update();
    std::string                           getTokenName(const std::string& tokenID) const;
    BFXTInt                               getTokenBalance(const std::string& tokenID) const;
    std::string                           getTokenName(int index) const;
    std::string                           getTokenId(int index) const;
    std::string                           getTokenIssuanceTxid(int index) const;
    std::string                           getTokenDescription(int index) const;
    BFXTInt                               getTokenBalance(int index) const;
    std::string                           getTokenIcon(int index);
    int64_t                               getNumberOfTokens() const;
    const std::map<std::string, BFXTInt>& getBalancesMap() const;
    const std::unordered_map<BFXTOutPoint, BFXTTransaction>& getWalletOutputsWithTokens();
    bool                                                     hasEverSucceeded() const;
    friend inline bool operator==(const BFXTWallet& lhs, const BFXTWallet& rhs);
    static bool        IconHasErrorContent(const std::string& icon);
    void               clear();
    void               setMinMaxConfirmations(int minConfs, int maxConfs = -1);
    static std::string Serialize(const BFXTWallet& wallet);
    static BFXTWallet  Deserialize(const std::string& data);

    // Serialize and deserialize maps basically is a generic serializer for any map
    // Notice that the type considerations are taking in
    // __KeyToString/__KeyFromString/__ValToJson/__ValFromJson
    template <typename Container>
    static json_spirit::Value SerializeMap(const Container& TheMap, bool serializeKey,
                                           bool serializeValue);
    template <typename Container>
    static Container DeserializeMap(const json_spirit::Value& json_val, bool deserializeKey,
                                    bool deserializeValue);

    void exportToFile(const boost::filesystem::path& filePath) const;
    void importFromFile(const boost::filesystem::path& filePath);

    //    static void CreateBFXTSendTransaction(uint64_t fee);

    bool getRetrieveFullMetadata() const;
    void setRetrieveFullMetadata(bool value);

    std::map<std::string, BFXTInt> getBalances() const;

    const std::unordered_map<std::string, BFXTTokenMetaData>& getTokenMetadataMap() const;

private:
    static std::string __KeyToString(const std::string& str, bool serialize);
    static void        __KeyFromString(const std::string& str, bool deserialize, std::string& result);
    static std::string __KeyToString(const BFXTOutPoint& op, bool);
    static void __KeyFromString(const std::string& outputString, bool deserialize, BFXTOutPoint& result);
    static json_spirit::Value __ValToJson(const BFXTTokenMetaData& input, bool serialize);
    static void               __ValFromJson(const json_spirit::Value& input, bool deserialize,
                                            BFXTTokenMetaData& result);
    static json_spirit::Value __ValToJson(const BFXTTransaction& input, bool serialize);
    static void               __ValFromJson(const json_spirit::Value& input, bool deserialize,
                                            BFXTTransaction& result);
    static json_spirit::Value __ValToJson(const std::string& input, bool serialize);
    static void __ValFromJson(const json_spirit::Value& input, bool deserialize, std::string& result);
    static json_spirit::Value __ValToJson(const int64_t& input, bool serialize);
    static json_spirit::Value __ValToJson(const BFXTInt& input, bool serialize);
    static void __ValFromJson(const json_spirit::Value& input, bool deserialize, int64_t& result);
    static void __ValFromJson(const json_spirit::Value& input, bool deserialize, BFXTInt& result);
};

bool operator==(const BFXTWallet& lhs, const BFXTWallet& rhs)
{
    return (lhs.getNumberOfTokens() == rhs.getNumberOfTokens() &&
            lhs.tokenInformation == rhs.tokenInformation &&
            lhs.walletOutputsWithTokens == rhs.walletOutputsWithTokens &&
            lhs.tokenIcons == rhs.tokenIcons && lhs.balances == rhs.balances &&
            lhs.lastTxCount == rhs.lastTxCount && lhs.lastOutputsCount == rhs.lastOutputsCount);
}

#endif // BFXTWALLET_H
