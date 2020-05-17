#ifndef BFXTTRANSACTION_H
#define BFXTTRANSACTION_H

#include "ThreadSafeHashMap.h"
#include "bfxt/bfxtscript.h"
#include "bfxt/bfxtscript_burn.h"
#include "bfxt/bfxtscript_issuance.h"
#include "bfxt/bfxtscript_transfer.h"
#include "bfxttokenmetadata.h"
#include "bfxttxin.h"
#include "bfxttxout.h"
#include "transaction.h"
#include "uint256.h"

#include <boost/filesystem/path.hpp>
#include <numeric>
#include <string>
#include <unordered_map>
#include <vector>

#define DEBUG__INCLUDE_STR_HASH

extern const std::string  BFXTOpReturnRegexStr;
extern const boost::regex BFXTOpReturnRegex;

extern const ThreadSafeHashMap<std::string, int> bfxt_blacklisted_token_ids;
extern const ThreadSafeHashMap<uint256, int>     excluded_txs_testnet;
extern const ThreadSafeHashMap<uint256, int>     excluded_txs_mainnet;

bool IsBFXTTokenBlacklisted(const std::string& tokenId, int& maxHeight);
bool IsBFXTTokenBlacklisted(const std::string& tokenId);
bool IsBFXTTxExcluded(const uint256& txHash);

struct TokenMinimalData
{
    BFXTInt     amount;
    std::string tokenName;
    std::string tokenId;

    TokenMinimalData() : amount(0) {}
};

/**
 * @brief The BFXTTransaction class
 * A single BFXT transaction
 */
class BFXTTransaction
{
    static const int CURRENT_VERSION = 1;
    int              nVersion;
    uint256          txHash = 0;
#ifdef DEBUG__INCLUDE_STR_HASH
    std::string strHash;
#endif
    std::vector<unsigned char> txSerialized;
    std::vector<BFXTTxIn>      vin;
    std::vector<BFXTTxOut>     vout;
    uint64_t                   nLockTime;
    uint64_t                   nTime;
    BFXTTransactionType        bfxtTransactionType = BFXTTxType_NOT_BFXT;

    template <typename ScriptType>
    void __TransferTokens(const std::shared_ptr<ScriptType>& scriptPtrD, const CTransaction& tx,
                          const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs,
                          bool                                                         burnOutput31);

public:
    static const uint64_t IssuanceFee = 1000000000; // 10 nebls

    // clang-format off
    IMPLEMENT_SERIALIZE(
                        READWRITE(this->nVersion);
                        nVersion = this->nVersion;
                        READWRITE(nTime);
                        READWRITE(txHash);
                        READWRITE(vin);
                        READWRITE(vout);
                        READWRITE(nLockTime);
                        READWRITE(bfxtTransactionType);
                        )
    // clang-format on

    BFXTTransaction();
    void                setNull();
    bool                isNull() const noexcept;
    void                importJsonData(const std::string& data);
    json_spirit::Value  exportDatabaseJsonData() const;
    void                importDatabaseJsonData(const json_spirit::Value& data);
    void                setHex(const std::string& Hex);
    std::string         getHex() const;
    uint256             getTxHash() const;
    uint64_t            getLockTime() const;
    uint64_t            getTime() const;
    unsigned long       getTxInCount() const;
    const BFXTTxIn&     getTxIn(unsigned long index) const;
    unsigned long       getTxOutCount() const;
    const BFXTTxOut&    getTxOut(unsigned long index) const;
    BFXTTransactionType getTxType() const;
    std::string         getTokenSymbolIfIssuance() const;
    std::string         getTokenIdIfIssuance(std::string input0txid, unsigned int input0index) const;
    void                updateDebugStrHash();
    friend inline bool  operator==(const BFXTTransaction& lhs, const BFXTTransaction& rhs);

    static std::unordered_map<std::string, TokenMinimalData>
    CalculateTotalInputTokens(const BFXTTransaction& bfxttx);
    static std::unordered_map<std::string, TokenMinimalData>
    CalculateTotalOutputTokens(const BFXTTransaction& bfxttx);

    static json_spirit::Value GetBFXTIssuanceMetadata(const uint256& issuanceTxid);
    static BFXTTokenMetaData  GetFullBFXTIssuanceMetadata(const CTransaction&    issuanceTx,
                                                          const BFXTTransaction& bfxtIssuanceTx);
    static BFXTTokenMetaData  GetFullBFXTIssuanceMetadata(const uint256& issuanceTxid);

    static void
    ReorderTokenInputsToGoFirst(CTransaction&                                                tx,
                                const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    static unsigned int
    CountTokenKindsInInputs(const CTransaction&                                          tx,
                            const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    static void
    EnsureInputsHashesMatch(const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    static void
    EnsureInputTokensRelateToTx(const CTransaction&                                          tx,
                                const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    /**
     * from a list of previous transactions, get the pair of CTransaction/BFXTTransaction that matches
     * the given hash
     *
     * @brief GetPrevInputIt
     * @param tx is the current transaction; used only for hash information
     * @param hash
     * @param inputsTxs
     * @return
     */
    static std::vector<std::pair<CTransaction, BFXTTransaction>>::const_iterator
    GetPrevInputIt(const CTransaction& tx, const uint256& hash,
                   const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    /**
     * takes a standard transaction and deals and tries to find BFXT-related problems in this transaction
     * and solve them; for example, BFXT token change is located and is put in the transaction
     *
     * @brief ComplementStdTxWithBFXT
     * @param tx
     */
    static void AmendStdTxWithBFXT(CTransaction& tx, int changeIndex);
    static void AmendStdTxWithBFXT(CTransaction&                                                tx_,
                                   const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputs,
                                   int changeIndex);

    void __manualSet(int NVersion, uint256 TxHash, std::vector<unsigned char> TxSerialized,
                     std::vector<BFXTTxIn> Vin, std::vector<BFXTTxOut> Vout, uint64_t NLockTime,
                     uint64_t NTime, BFXTTransactionType Ntp1TransactionType);

    std::string getBFXTOpReturnScriptHex() const;

    /**
     * sets only shallow information from the source transaction (no token information)
     * it's not possible to set token information without prev inputs information; for that, use the
     * non-minimal version
     * @brief readBFXTDataFromTx_minimal
     * @param tx the source BFX transcation
     */
    void readBFXTDataFromTx_minimal(const CTransaction& tx);

    void readBFXTDataFromTx(const CTransaction&                                          tx,
                            const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs);

    static bool TxContainsOpReturn(const CTransaction* tx, std::string* opReturnArg = nullptr);
    static bool IsTxBFXT(const CTransaction* tx, std::string* opReturnArg = nullptr);
    static bool IsTxOutputBFXTOpRet(const CTransaction* tx, unsigned int index,
                                    std::string* opReturnArg = nullptr);
    static bool IsTxOutputOpRet(const CTransaction* tx, unsigned int index,
                                std::string* opReturnArg = nullptr);
    static bool IsTxOutputOpRet(const CTxOut* output, std::string* opReturnArg = nullptr);

    /** for a certain transaction, retrieve all BFXT data from the database */
    static std::vector<std::pair<CTransaction, BFXTTransaction>>
    GetAllBFXTInputsOfTx(CTransaction tx, bool recoverProtection, int recursionCount = 0);

    static std::vector<std::pair<CTransaction, BFXTTransaction>>
    GetAllBFXTInputsOfTx(CTransaction tx, CTxDB& txdb, bool recoverProtection, int recursionCount = 0);

    static std::vector<std::pair<CTransaction, BFXTTransaction>> GetAllBFXTInputsOfTx(
        CTransaction tx, CTxDB& txdb, bool recoverProtection,
        const std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>&
                                           mapQueuedBFXTInputs,
        const std::map<uint256, CTxIndex>& queuedAcceptedTxs = std::map<uint256, CTxIndex>(),
        int                                recursionCount    = 0);

    /** Take a list of standard bfx transactions and return pairs of bfx and BFXT transactions */
    static std::vector<std::pair<CTransaction, BFXTTransaction>> StdFetchedInputTxsToBFXT(
        const CTransaction& tx, const MapPrevTx& mapInputs, CTxDB& txdb, bool recoverProtection,
        const std::map<uint256, CTxIndex>& queuedAcceptedTxs = std::map<uint256, CTxIndex>(),
        int                                recursionCount    = 0);

    static std::vector<std::pair<CTransaction, BFXTTransaction>> StdFetchedInputTxsToBFXT(
        const CTransaction& tx, const MapPrevTx& mapInputs, CTxDB& txdb, bool recoverProtection,
        const std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>&
                                           mapQueuedBFXTInputs,
        const std::map<uint256, CTxIndex>& queuedAcceptedTxs = std::map<uint256, CTxIndex>(),
        int                                recursionCount    = 0);
};

bool operator==(const BFXTTransaction& lhs, const BFXTTransaction& rhs)
{
    return (lhs.nVersion == rhs.nVersion && lhs.txHash == rhs.txHash &&
            lhs.txSerialized == rhs.txSerialized && lhs.vin == rhs.vin && lhs.vout == rhs.vout &&
            lhs.nLockTime == rhs.nLockTime && lhs.nTime == rhs.nTime &&
            lhs.bfxtTransactionType == rhs.bfxtTransactionType);
}

template <typename ScriptType>
void BFXTTransaction::__TransferTokens(
    const std::shared_ptr<ScriptType>& scriptPtrD, const CTransaction& tx,
    const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs, bool burnOutput31)
{
    static_assert(std::is_same<ScriptType, BFXTScript_Transfer>::value ||
                      std::is_same<ScriptType, BFXTScript_Burn>::value,
                  "Script types can only be Transfer and Burn in this function");

    int currentInputIndex = 0;

    EnsureInputsHashesMatch(inputsTxs);
    EnsureInputTokensRelateToTx(tx, inputsTxs);

    // calculate total tokens in inputs
    std::vector<std::vector<BFXTInt>>         totalTokensLeftInInputs(tx.vin.size());
    std::vector<std::vector<BFXTTokenTxData>> tokensKindsInInputs(tx.vin.size());
    for (unsigned i = 0; i < tx.vin.size(); i++) {
        const auto& n    = tx.vin[i].prevout.n;
        const auto& hash = tx.vin[i].prevout.hash;

        auto it = GetPrevInputIt(tx, hash, inputsTxs);

        const BFXTTransaction& bfxttx = it->second;
        // this array keeps track of all tokens left
        totalTokensLeftInInputs[i].resize(bfxttx.getTxOut(n).tokenCount());
        // this object relates tokens in the last list to their corresponding information
        tokensKindsInInputs[i].resize(bfxttx.getTxOut(n).tokenCount());
        // loop over tokens of a single input (outputs from previous transactions)
        for (int j = 0; j < (int)bfxttx.getTxOut(n).tokenCount(); j++) {
            const auto& tokenObj          = bfxttx.getTxOut(n).getToken(j);
            totalTokensLeftInInputs[i][j] = tokenObj.getAmount();
            tokensKindsInInputs[i][j]     = tokenObj;
        }
    }

    std::vector<BFXTScript::TransferInstruction> TIs = scriptPtrD->getTransferInstructions();

    // the operation is invalid if input 0 has no tokens; this artificial failure is forced for this to
    // be compliant with the API
    bool invalid = false;
    if (totalTokensLeftInInputs.size() == 0) {
        invalid = true;
    } else {
        BFXTInt totalTokensInInput0 = std::accumulate(totalTokensLeftInInputs[0].begin(),
                                                      totalTokensLeftInInputs[0].end(), BFXTInt(0));

        invalid = !totalTokensInInput0;
    }

    for (int i = 0; i < (int)scriptPtrD->getTransferInstructionsCount(); i++) {
        // if the transaction is invalid, break here and go straight to change calculations
        if (invalid) {
            break;
        }

        if (currentInputIndex >= static_cast<int>(vin.size())) {
            throw std::runtime_error(
                "An input of transfer instruction is outside the available "
                "range of inputs in BFXT OP_RETURN argument: " +
                scriptPtrD->getParsedScriptHex() + ", where the number of available inputs is " +
                ::ToString(tx.vin.size()) + " in transaction " + tx.GetHash().ToString());
        }

        const int outputIndex    = TIs[i].outputIndex;
        bool      burnThisOutput = (burnOutput31 && outputIndex == 31);

        if (outputIndex >= static_cast<int>(vout.size()) && !burnThisOutput) {
            throw std::runtime_error(
                "An output of transfer instruction is outside the available "
                "range of outputs in BFXT OP_RETURN argument: " +
                scriptPtrD->getParsedScriptHex() + ", where the number of available outputs is " +
                ::ToString(tx.vout.size()) + " in transaction " + tx.GetHash().ToString());
        }

        // loop over the kinds of tokens in the input and distribute them over outputs
        // note: there's no way to switch from one token to the next unless its content depletes
        BFXTInt currentOutputAmount = TIs[i].amount;

        //  token index at which to start subtraction, helps in skipping empty tokens when
        // subtracting spent amount
        int startTokenIndex = 0;

        // if input is empty, just move to the next one since empty inputs don't break adjacency
        bool    stopInstructions = false;
        BFXTInt totalTokensInCurrentInput =
            std::accumulate(totalTokensLeftInInputs[currentInputIndex].begin(),
                            totalTokensLeftInInputs[currentInputIndex].end(), BFXTInt(0));
        while (totalTokensLeftInInputs[currentInputIndex].size() == 0 ||
               totalTokensInCurrentInput == 0) {
            currentInputIndex++;
            if (currentInputIndex >= static_cast<int>(vin.size())) {
                stopInstructions = true;
                break;
            }
            totalTokensInCurrentInput =
                std::accumulate(totalTokensLeftInInputs[currentInputIndex].begin(),
                                totalTokensLeftInInputs[currentInputIndex].end(), BFXTInt(0));
        }

        if (stopInstructions) {
            break;
        }

        for (int j = 0; j < (int)totalTokensLeftInInputs[currentInputIndex].size(); j++) {

            // calculate the total number of available tokens for spending
            BFXTInt     totalAdjacentTokensOfOneKind = 0;
            std::string currentTokenId;
            bool        inputDone = false;
            for (int k = currentInputIndex; k < (int)totalTokensLeftInInputs.size(); k++) {
                // an empty input in between doesn't break adjacency
                //                if (totalTokensLeftInInputs[k].size() == 0) {
                //                    break;
                //                }
                for (int l = (k == currentInputIndex ? j : 0);
                     l < (int)totalTokensLeftInInputs[k].size(); l++) {
                    // if the amount of tokens currently is zero, it's simply skipped and the token ID is
                    // changed to the next adjacent kind
                    if (tokensKindsInInputs[k][l].getTokenId() != currentTokenId &&
                        totalAdjacentTokensOfOneKind > 0) {
                        inputDone = true;
                        break;
                    } else {
                        if (totalAdjacentTokensOfOneKind == 0) {
                            startTokenIndex = l;
                            currentTokenId  = tokensKindsInInputs[currentInputIndex][l].getTokenId();
                        }
                        totalAdjacentTokensOfOneKind += totalTokensLeftInInputs[k][l];
                    }
                }
                if (inputDone) {
                    break;
                }
            }

            // check if the token is blacklisted
            int blacklistHeight = 0;
            if (IsBFXTTokenBlacklisted(currentTokenId, blacklistHeight)) {
                if (nBestHeight >= blacklistHeight) {
                    throw std::runtime_error("The BFXT token " + currentTokenId +
                                             " is blacklisted and cannot be transferred or burned.");
                }
            }

            // ensure that gaps won't create a problem; an empty input is automatically skipped here
            if (totalAdjacentTokensOfOneKind == 0) {
                if (j + 1 >= (int)totalTokensLeftInInputs[currentInputIndex].size()) {
                    currentInputIndex++;
                    j = -1; // resets j to 0 in the next iteration
                }
                continue;
            }

            if (currentOutputAmount > totalAdjacentTokensOfOneKind) {
                throw std::runtime_error(
                    "Insufficient tokens for transaction from inputs for transaction: " +
                    tx.GetHash().ToString() + " from input: " + ::ToString(currentInputIndex) +
                    ". Required output amount: " + ::ToString(currentOutputAmount) +
                    "; and the total available amount in all (possibly adjacent) inputs: " +
                    ::ToString(totalAdjacentTokensOfOneKind) +
                    "; in transfer instruction number: " + ::ToString(i) + ". Inputs in order are: " +
                    std::accumulate(tx.vin.begin(), tx.vin.end(), std::string(),
                                    [](const std::string& currRes, const CTxIn& inp) {
                                        return currRes + " - " + inp.prevout.hash.ToString() + ":" +
                                               ::ToString(inp.prevout.n);
                                    }) +
                    "; and OP_RETURN script: " + scriptPtrD->getParsedScriptHex());
            }

            const auto&   currentTokenObj = tokensKindsInInputs[currentInputIndex][startTokenIndex];
            const BFXTInt amountToCredit  = std::min(totalAdjacentTokensOfOneKind, currentOutputAmount);

            if (!burnThisOutput) {
                // create the token object that will be added to the output
                BFXTTokenTxData bfxttokenTxData;
                bfxttokenTxData.setAmount(amountToCredit);
                bfxttokenTxData.setTokenId(currentTokenObj.getTokenId());
                bfxttokenTxData.setAggregationPolicy(currentTokenObj.getAggregationPolicy());
                bfxttokenTxData.setDivisibility(currentTokenObj.getDivisibility());
                bfxttokenTxData.setTokenSymbol(currentTokenObj.getTokenSymbol());
                bfxttokenTxData.setLockStatus(currentTokenObj.getLockStatus());
                bfxttokenTxData.setIssueTxIdHex(currentTokenObj.getIssueTxId().ToString());

                // add the token to the output
                vout[outputIndex].tokens.push_back(bfxttokenTxData);
            }

            // reduce the available output amount
            if (amountToCredit > currentOutputAmount) {
                currentOutputAmount = 0;
            } else {
                currentOutputAmount -= amountToCredit;
            }

            // reduce the available balance from the array that tracks all available inputs
            BFXTInt amountLeftToSubtract = amountToCredit;
            for (int k = currentInputIndex; k < (int)totalTokensLeftInInputs.size(); k++) {
                // an empty input in between means inputs are not adjacent
                for (int l = (k == currentInputIndex ? startTokenIndex : 0);
                     l < (int)totalTokensLeftInInputs[k].size(); l++) {
                    if (tokensKindsInInputs[k][l].getTokenId() == currentTokenId) {
                        if (totalTokensLeftInInputs[k][l] >= amountLeftToSubtract) {
                            totalTokensLeftInInputs[k][l] -= amountLeftToSubtract;
                            amountLeftToSubtract = 0;
                        } else {
                            amountLeftToSubtract -= totalTokensLeftInInputs[k][l];
                            totalTokensLeftInInputs[k][l] = 0;
                        }
                    } else {
                        if (amountLeftToSubtract == 0) {
                            break;
                        } else {
                            throw std::runtime_error(
                                "Unable to decredit the available balances from inputs");
                        }
                    }
                    if (amountLeftToSubtract == 0) {
                        break;
                    }
                }
                if (amountLeftToSubtract == 0) {
                    break;
                }
            }
            if (amountLeftToSubtract > 0) {
                throw std::runtime_error("Unable to decredit the available balances from inputs");
            }

            // if skip, move on to the next input
            if (TIs[i].skipInput) {
                currentInputIndex++;
            }

            BFXTInt totalTokensLeftInCurrentInput =
                std::accumulate(totalTokensLeftInInputs[currentInputIndex].begin(),
                                totalTokensLeftInInputs[currentInputIndex].end(), BFXTInt(0));
            if (totalTokensLeftInCurrentInput == 0) {
                // avoid incrementing twice
                if (!TIs[i].skipInput) {
                    currentInputIndex++;
                }
            }

            // all required output amount is spent. The rest will be redirected as change
            if (currentOutputAmount == 0) {
                break;
            }
        }
    }

    for (int i = 0; i < (int)totalTokensLeftInInputs.size(); i++) {
        for (int j = 0; j < (int)totalTokensLeftInInputs[i].size(); j++) {
            if (totalTokensLeftInInputs[i][j] == 0) {
                continue;
            }

            const auto& currentTokenObj = tokensKindsInInputs[i][j];
            BFXTInt     amountToCredit  = totalTokensLeftInInputs[i][j];

            // create the token object that will be added to the output
            BFXTTokenTxData bfxttokenTxData;
            bfxttokenTxData.setAmount(amountToCredit);
            bfxttokenTxData.setTokenId(currentTokenObj.getTokenId());
            bfxttokenTxData.setAggregationPolicy(currentTokenObj.getAggregationPolicy());
            bfxttokenTxData.setDivisibility(currentTokenObj.getDivisibility());
            bfxttokenTxData.setTokenSymbol(currentTokenObj.getTokenSymbol());
            bfxttokenTxData.setLockStatus(currentTokenObj.getLockStatus());
            bfxttokenTxData.setIssueTxIdHex(currentTokenObj.getIssueTxId().ToString());

            if (vout.size() == 0) {
                throw std::runtime_error("No outputs in transaction: " + tx.GetHash().ToString());
            }

            // reduce the available balance
            totalTokensLeftInInputs[i][j] -= amountToCredit;
            amountToCredit = 0;

            if (bfxttokenTxData.getAggregationPolicy() ==
                BFXTScript::IssuanceFlags::AggregationPolicy_Aggregatable_Str) {
                // aggregate coins from next inputs
                bool stopLooping = false;
                for (int k = i; k < (int)totalTokensLeftInInputs.size(); k++) {
                    for (int l = (k == i ? j : 0); l < (int)totalTokensLeftInInputs[k].size(); l++) {
                        if (k == i && l == j) {
                            continue; // ignore the balance that was already credited
                        }
                        if (tokensKindsInInputs[i][j].getTokenId() !=
                            tokensKindsInInputs[k][l].getTokenId()) {
                            stopLooping = true;
                            break;
                        }
                        amountToCredit                = totalTokensLeftInInputs[k][l];
                        totalTokensLeftInInputs[k][l] = 0;
                        bfxttokenTxData.setAmount(bfxttokenTxData.getAmount() + amountToCredit);
                    }

                    // stop the outer loop
                    if (stopLooping) {
                        break;
                    }
                }
            }

            // add the token to the last output
            vout.back().tokens.push_back(bfxttokenTxData);
        }
    }

    // delete empty token slots
    for (int i = 0; i < (int)vout.size(); i++) {
        for (int j = 0; j < (int)vout[i].tokenCount(); j++) {
            if (vout[i].getToken(j).getAmount() == 0) {
                vout[i].tokens.erase(vout[i].tokens.begin() + j);
                j--;
            }
        }
    }
}

#endif // BFXTTRANSACTION_H
