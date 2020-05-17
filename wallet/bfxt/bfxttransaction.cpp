#include "bfxttransaction.h"

#include "base58.h"
#include "main.h"
#include "bfxt/bfxtscript.h"
#include "bfxt/bfxtscript_burn.h"
#include "bfxt/bfxtscript_issuance.h"
#include "bfxt/bfxtscript_transfer.h"
#include "bfxt/bfxtv1_issuance_static_data.h"
#include "bfxttools.h"
#include "bfxttxin.h"
#include "bfxttxout.h"
#include "txdb.h"
#include "util.h"

#include <boost/algorithm/hex.hpp>

const std::string  BFXTOpReturnRegexStr = R"(^OP_RETURN\s+(4e54(?:01|03)[a-fA-F0-9]*)$)";
const boost::regex BFXTOpReturnRegex(BFXTOpReturnRegexStr);
const std::string  OpReturnRegexStr = R"(^OP_RETURN\s+(.*)$)";
const boost::regex OpReturnRegex(OpReturnRegexStr);

// token id vs max block to take transactions from these tokens
const ThreadSafeHashMap<std::string, int>
    bfxt_blacklisted_token_ids(std::unordered_map<std::string, int>{
        {"La77KcJTUj991FnvxNKhrCD1ER8S81T3LgECS6", 300000}, // old QRT mainnet
        {"La4kVcoUAddLWkmQU9tBxrNdFjmSaHQruNJW2K", 300000}, // old TNIBB testnet
        {"La36YNY2G6qgBPj7VSiQDjGCy8aC2GUUsGqtbQ", 300000}, // old TNIBB testnet
        {"La9wLfpkfZTQvRqyiWjaEpgQStUbCSVMWZW2by", 300000}, // TEST3 on testnet
        {"La347xkKhi5VUCNDCqxXU4F1RUu8wPvC3pnQk6", 300000}, // BOT on testnet
        {"La531vUwiu9NnvtJcwPEjV84HrdKCupFCCb6D7", 300000}, // BAUTO on testnet
        {"La5JGnJcSsLCvYWxqqVSyj3VUqsrAcLBjZjbw5", 300000}, // XYZ from Sam on testnet
        {"La86PtvXGftbwdoZ9rVMKsLQU5nPHganJDsCRq", 300000}  // ON
        //    ,{"LaA5grPQMDhwvciWFqxwG1ySDqNHAgms1yLrPp", 300000}  // NIBBL
    });

// list of transactions to be excluded because they're invalid
// this should be a thread-safe hashset, but we don't have one. So we're using the map.
const ThreadSafeHashMap<uint256, int> excluded_txs_testnet(std::unordered_map<uint256, int>{
    {uint256("826e7b74b24e458e39d779b1033567d325b8d93b507282f983e3c4b3f950fca1"), 0},
    {uint256("c378447562be04c6803fdb9f829c9ba0dda462b269e15bcfc7fac3b3561d2eef"), 0},
    {uint256("a57a3e4746a79dd0d0e32e6a831d4207648ff000c82a4c5e8d9f3b6b0959f8b8"), 0},
    {uint256("7e71508abef696d6c0427cc85073e0d56da9380f3d333354c7dd9370acd422bc"), 0},
    {uint256("adb421a497e25375a88848b17b5c632a8d60db3d02dcc61dbecd397e6c1fb1ca"), 0},
    {uint256("adedc16e0318668e55f08f2a1ea57be8c5a86cfce3c1900346b0337a8f75a390"), 0},
    {uint256("bb8f1a29237e64285b9bd1f2bf1500c0de6205e8eb5e004c3b1ab6671e9c4cb2"), 0},
    {uint256("cc8f8a763677b8015bf79a19c9bcf87837b734d1cb203b30726af27b75f41a48"), 0},
    {uint256("666d81ad74e470ef1c9e74022a8be886e4951a0bec0d27f9b078519a30af71b2"), 0},
    {uint256("27bea35b4e2ac8987441aa7c5ff3d305047664ef7244b822cad54e549b84f50b"), 0},
    {uint256("59cb6e2cc9649d9a9b806f820a91927dcb0e43d1e1e92b0b9d976e921bba1334"), 0},
    {uint256("054cead1a3b498ec845462a1920508698e4f0ab2a71e1f4f8d827d007a43a2f4"), 0},
    {uint256("7d211b98e4796e9375233d935eb8d1262d6fb9d79645b576f15ad1b85427facf"), 0},
    {uint256("ab336eecf51cdaecd3f7444d5da7eca2286462d44e7f3439458ecbe3d7514971"), 0},
    {uint256("95c6f2b978160ab0d51545a13a7ee7b931713a52bd1c9f12807f4cd77ff7536b"), 0}});

const ThreadSafeHashMap<uint256, int> excluded_txs_mainnet = {};

bool IsBFXTTokenBlacklisted(const std::string& tokenId, int& maxHeight)
{
    return bfxt_blacklisted_token_ids.get(tokenId, maxHeight);
}

bool IsBFXTTokenBlacklisted(const std::string& tokenId)
{
    return bfxt_blacklisted_token_ids.exists(tokenId);
}

bool IsBFXTTxExcluded(const uint256& txHash)
{
    return excluded_txs_testnet.exists(txHash) || excluded_txs_mainnet.exists(txHash);
}

BFXTTransaction::BFXTTransaction() { setNull(); }

void BFXTTransaction::setNull()
{
    nVersion = BFXTTransaction::CURRENT_VERSION;
    nTime    = GetAdjustedTime() * 1000;
    vin.clear();
    vout.clear();
    nLockTime = 0;
}

bool BFXTTransaction::isNull() const noexcept { return (vin.empty() && vout.empty()); }

void BFXTTransaction::importJsonData(const std::string& data)
{
    try {
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(data, parsedData);

        setHex(BFXTTools::GetStrField(parsedData.get_obj(), "hex"));
        std::string hash = BFXTTools::GetStrField(parsedData.get_obj(), "txid");
#ifdef DEBUG__INCLUDE_STR_HASH
        strHash = hash;
#endif
        txHash.SetHex(hash);
        nLockTime                   = BFXTTools::GetUint64Field(parsedData.get_obj(), "locktime");
        nTime                       = BFXTTools::GetUint64Field(parsedData.get_obj(), "time");
        nVersion                    = BFXTTools::GetUint64Field(parsedData.get_obj(), "version");
        json_spirit::Array vin_list = BFXTTools::GetArrayField(parsedData.get_obj(), "vin");
        vin.clear();
        vin.resize(vin_list.size());
        for (unsigned long i = 0; i < vin_list.size(); i++) {
            vin[i].importJsonData(vin_list[i]);
        }
        json_spirit::Array vout_list = BFXTTools::GetArrayField(parsedData.get_obj(), "vout");
        vout.clear();
        vout.resize(vout_list.size());
        for (unsigned long i = 0; i < vout_list.size(); i++) {
            vout[i].importJsonData(vout_list[i]);
        }
    } catch (std::exception& ex) {
        printf("%s", ex.what());
        throw;
    }
}

json_spirit::Value BFXTTransaction::exportDatabaseJsonData() const
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("version", nVersion));
    root.push_back(json_spirit::Pair("txid", txHash.GetHex()));
    root.push_back(json_spirit::Pair("locktime", nLockTime));
    root.push_back(json_spirit::Pair("time", nTime));
    root.push_back(json_spirit::Pair("hex", getHex()));

    json_spirit::Array vinArray;
    for (long i = 0; i < static_cast<long>(vin.size()); i++) {
        vinArray.push_back(vin[i].exportDatabaseJsonData());
    }
    root.push_back(json_spirit::Pair("vin", json_spirit::Value(vinArray)));

    json_spirit::Array voutArray;
    for (long i = 0; i < static_cast<long>(vout.size()); i++) {
        voutArray.push_back(vout[i].exportDatabaseJsonData());
    }
    root.push_back(json_spirit::Pair("vout", json_spirit::Value(voutArray)));

    return json_spirit::Value(root);
}

void BFXTTransaction::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    nVersion = BFXTTools::GetUint64Field(data.get_obj(), "version");
    txHash.SetHex(BFXTTools::GetStrField(data.get_obj(), "txid"));
#ifdef DEBUG__INCLUDE_STR_HASH
    strHash = BFXTTools::GetStrField(data.get_obj(), "txid");
#endif
    nLockTime = BFXTTools::GetUint64Field(data.get_obj(), "locktime");
    nTime     = BFXTTools::GetUint64Field(data.get_obj(), "time");
    setHex(BFXTTools::GetStrField(data.get_obj(), "hex"));

    json_spirit::Array vin_list = BFXTTools::GetArrayField(data.get_obj(), "vin");
    vin.clear();
    vin.resize(vin_list.size());
    for (unsigned long i = 0; i < vin_list.size(); i++) {
        vin[i].importDatabaseJsonData(vin_list[i]);
    }

    json_spirit::Array vout_list = BFXTTools::GetArrayField(data.get_obj(), "vout");
    vout.clear();
    vout.resize(vout_list.size());
    for (unsigned long i = 0; i < vout_list.size(); i++) {
        vout[i].importDatabaseJsonData(vout_list[i]);
    }
}

std::string BFXTTransaction::getHex() const
{
    std::string out;
    boost::algorithm::hex(txSerialized.begin(), txSerialized.end(), std::back_inserter(out));
    return out;
}

void BFXTTransaction::setHex(const std::string& Hex)
{
    txSerialized.clear();
    boost::algorithm::unhex(Hex.begin(), Hex.end(), std::back_inserter(txSerialized));
}

uint256 BFXTTransaction::getTxHash() const { return txHash; }

uint64_t BFXTTransaction::getLockTime() const { return nLockTime; }

uint64_t BFXTTransaction::getTime() const { return nTime; }

unsigned long BFXTTransaction::getTxInCount() const { return vin.size(); }

const BFXTTxIn& BFXTTransaction::getTxIn(unsigned long index) const { return vin[index]; }

unsigned long BFXTTransaction::getTxOutCount() const { return vout.size(); }

const BFXTTxOut& BFXTTransaction::getTxOut(unsigned long index) const { return vout[index]; }

BFXTTransactionType BFXTTransaction::getTxType() const { return bfxtTransactionType; }

std::string BFXTTransaction::getTokenSymbolIfIssuance() const
{
    std::string                 script    = getBFXTOpReturnScriptHex();
    std::shared_ptr<BFXTScript> scriptPtr = BFXTScript::ParseScript(script);
    if (scriptPtr->getTxType() != BFXTScript::TxType_Issuance) {
        throw std::runtime_error(
            "Attempted to get the token symbol of a non-issuance transaction. Txid: " +
            this->getTxHash().ToString() + "; and current tx type: " + ToString(scriptPtr->getTxType()));
    }
    std::shared_ptr<BFXTScript_Issuance> scriptPtrD =
        std::dynamic_pointer_cast<BFXTScript_Issuance>(scriptPtr);

    if (!scriptPtrD) {
        throw std::runtime_error("While getting token symbol for issuance tx, casting script pointer to "
                                 "transfer type failed: " +
                                 script);
    }
    return scriptPtrD->getTokenSymbol();
}

std::string BFXTTransaction::getTokenIdIfIssuance(std::string input0txid, unsigned int input0index) const
{
    std::string                 script    = getBFXTOpReturnScriptHex();
    std::shared_ptr<BFXTScript> scriptPtr = BFXTScript::ParseScript(script);
    if (scriptPtr->getTxType() != BFXTScript::TxType_Issuance) {
        throw std::runtime_error("Attempted to get the token id of a non-issuance transaction. Txid: " +
                                 this->getTxHash().ToString() +
                                 "; and current tx type: " + ToString(scriptPtr->getTxType()));
    }
    std::shared_ptr<BFXTScript_Issuance> scriptPtrD =
        std::dynamic_pointer_cast<BFXTScript_Issuance>(scriptPtr);

    if (!scriptPtrD) {
        throw std::runtime_error("While getting token id for issuance tx, casting script pointer to "
                                 "transfer type failed: " +
                                 script);
    }
    return scriptPtrD->getTokenID(input0txid, input0index);
}

void BFXTTransaction::updateDebugStrHash()
{
#ifdef DEBUG__INCLUDE_STR_HASH
    strHash = txHash.ToString();
#endif
}

std::unordered_map<std::string, TokenMinimalData>
BFXTTransaction::CalculateTotalInputTokens(const BFXTTransaction& bfxttx)
{
    std::unordered_map<std::string, TokenMinimalData> result;
    for (const BFXTTxIn& in : bfxttx.vin) {
        for (const BFXTTokenTxData& token : in.tokens) {
            const std::string& tokenId = token.getTokenId();
            if (result.find(tokenId) == result.end()) {
                TokenMinimalData tokenData;
                tokenData.amount    = token.getAmount();
                tokenData.tokenId   = token.getTokenId();
                tokenData.tokenName = token.getTokenSymbol();
                result[tokenId]     = tokenData;
            } else {
                result[tokenId].amount += token.getAmount();
            }
        }
    }
    return result;
}

std::unordered_map<std::string, TokenMinimalData>
BFXTTransaction::CalculateTotalOutputTokens(const BFXTTransaction& bfxttx)
{
    std::unordered_map<std::string, TokenMinimalData> result;
    for (const BFXTTxOut& in : bfxttx.vout) {
        for (const BFXTTokenTxData& token : in.tokens) {
            const std::string& tokenId = token.getTokenId();
            if (result.find(tokenId) == result.end()) {
                TokenMinimalData tokenData;
                tokenData.amount    = token.getAmount();
                tokenData.tokenId   = token.getTokenId();
                tokenData.tokenName = token.getTokenSymbol();
                result[tokenId]     = tokenData;
            } else {
                result[tokenId].amount += token.getAmount();
            }
        }
    }
    return result;
}

void BFXTTransaction::ReorderTokenInputsToGoFirst(
    CTransaction& tx, const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{

    EnsureInputTokensRelateToTx(tx, inputsTxs);
    EnsureInputsHashesMatch(inputsTxs);

    if (CountTokenKindsInInputs(tx, inputsTxs) == 0) {
        return;
    }

    // loop over vin's with no tokens, and swap them with ones that do to make them first
    for (int i = 0; i < (int)tx.vin.size(); i++) {
        auto it1 = GetPrevInputIt(tx, tx.vin[i].prevout.hash, inputsTxs);

        const BFXTTransaction& bfxtInTx1 = it1->second;

        // if there are no tokens in this instance, find next ones that do, and move tokens here
        if (bfxtInTx1.getTxOut(tx.vin[i].prevout.n).tokenCount() == 0) {
            for (int j = i + 1; j < (int)tx.vin.size(); j++) {
                auto it2 = GetPrevInputIt(tx, tx.vin[j].prevout.hash, inputsTxs);

                const BFXTTransaction& bfxtInTx2 = it2->second;

                if (bfxtInTx2.getTxOut(tx.vin[j].prevout.n).tokenCount() != 0) {
                    std::swap(tx.vin[i], tx.vin[j]);
                    break;
                }
            }
        }
    }
}

unsigned int BFXTTransaction::CountTokenKindsInInputs(
    const CTransaction& tx, const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{
    unsigned result = 0;

    for (const auto& in : tx.vin) {
        auto it = GetPrevInputIt(tx, in.prevout.hash, inputsTxs);

        const CTransaction&    neblInTx = it->first;
        const BFXTTransaction& bfxtInTx = it->second;

        if (in.prevout.n + 1 > bfxtInTx.getTxOutCount()) {
            throw std::runtime_error("Failed at retrieving the number of tokens from transaction " +
                                     tx.GetHash().ToString() + " at input " +
                                     neblInTx.GetHash().ToString() +
                                     "; input: " + ToString(in.prevout.n) + " is out of range.");
        }

        result += bfxtInTx.getTxOut(in.prevout.n).tokenCount();
    }

    return result;
}

void BFXTTransaction::EnsureInputsHashesMatch(
    const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{
    // ensure that input pairs match
    for (const auto& in : inputsTxs) {
        if (in.first.GetHash() != in.second.getTxHash()) {
            throw std::runtime_error(
                "Input transactions in the BFXT parser do not have matching hashes.");
        }
    }
}

void BFXTTransaction::EnsureInputTokensRelateToTx(
    const CTransaction& tx, const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{
    // ensure that all inputs are relevant to this transaction (to protect from double-spending
    // tokens)
    for (unsigned i = 0; i < inputsTxs.size(); i++) {
        uint256 currentHash = inputsTxs[i].first.GetHash(); // the tx-hash of the input
        auto    it = std::find_if(tx.vin.begin(), tx.vin.end(), [&currentHash](const CTxIn& in) {
            return in.prevout.hash == currentHash;
        });
        if (it == tx.vin.end()) {
            throw std::runtime_error("An input was included in BFXT transaction parser while it was not "
                                     "being spent by the spending transaction. This is not allowed.");
        }
    }
}

std::vector<std::pair<CTransaction, BFXTTransaction>>::const_iterator
BFXTTransaction::GetPrevInputIt(const CTransaction& tx, const uint256& inputTxHash,
                                const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{
    auto it = std::find_if(inputsTxs.cbegin(), inputsTxs.cend(),
                           [&inputTxHash](const std::pair<CTransaction, BFXTTransaction>& inPair) {
                               return inPair.first.GetHash() == inputTxHash;
                           });
    if (it == inputsTxs.end()) {
        throw std::runtime_error(
            "While reading/parsing BFXT transction: Could not find input related to transaction: " +
            tx.GetHash().ToString() + " with a prevout hash: " + inputTxHash.ToString());
    }

    return it;
}

void BFXTTransaction::AmendStdTxWithBFXT(CTransaction& tx, int changeIndex)
{
    CTxDB                                                 txdb;
    std::vector<std::pair<CTransaction, BFXTTransaction>> inputs = GetAllBFXTInputsOfTx(tx, txdb, false);

    AmendStdTxWithBFXT(tx, inputs, changeIndex);
}

void BFXTTransaction::AmendStdTxWithBFXT(
    CTransaction& tx, const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputs,
    int changeIndex)
{
    // temp copy to avoid changing the original if the operation fails
    CTransaction tx_ = tx;

    EnsureInputsHashesMatch(inputs);

    EnsureInputTokensRelateToTx(tx_, inputs);

    unsigned inputTokenKinds = CountTokenKindsInInputs(tx_, inputs);

    bool txContainsOpReturn = TxContainsOpReturn(&tx_);

    // if no inputs contain BFXT AND no OP_RETURN argument exists, then this is a pure BFX transaction
    // with no BFXT
    if (inputTokenKinds == 0) {
        return;
    }

    // there are tokens, but there is already OP_RETURN
    if (txContainsOpReturn) {
        throw std::runtime_error("Cannot BFXT-amend transaction " + tx_.GetHash().ToString() +
                                 " because it already has an OP_RETURN output");
    }

    ReorderTokenInputsToGoFirst(tx_, inputs);

    if (!txContainsOpReturn && inputTokenKinds > 0) {
        // no OP_RETURN output, but there are input tokens to be diverted to output
        tx_.vout.push_back(CTxOut(0, CScript())); // pushed now, but will be filled later
        unsigned opRetIdx = tx_.vout.size() - 1;

        std::vector<BFXTScript::TransferInstruction> TIs;

        for (int i = 0; i < (int)tx_.vin.size(); i++) {
            const auto& inHash  = tx_.vin[i].prevout.hash;
            const auto& inIndex = tx_.vin[i].prevout.n;
            auto        it      = GetPrevInputIt(tx_, inHash, inputs);

            const CTransaction&    inputTxNebl = it->first;
            const BFXTTransaction& inputTxBFXT = it->second;

            for (int j = 0; j < (int)inputTxBFXT.vout.at(inIndex).tokenCount(); j++) {
                if (inputTxBFXT.vout.at(inIndex).getToken(j).getAmount() == 0) {
                    if (i == 0) {
                        throw std::runtime_error("While amending a native bfx transactions, the "
                                                 "first input is empty. This basically cannot be "
                                                 "amended. Inputs must be reordered to have tokens in "
                                                 "first inputs, and it seems that reordering failed");
                    }
                    continue;
                }

                // prepare native BFX output
                CTxDestination currentTokenAddress;
                // get the current address where the token is
                if (!ExtractDestination(inputTxNebl.vout.at(inIndex).scriptPubKey,
                                        currentTokenAddress)) {
                    throw std::runtime_error("Unable to extract address from previous output; tx: " +
                                             tx_.GetHash().ToString() + " and prevout: " +
                                             inHash.ToString() + ":" + ToString(inIndex));
                }

                if (changeIndex + 1 > (int)tx_.vout.size()) {
                    throw std::runtime_error("Invalid change index provided to fund moving BFXT tokens");
                }

                if (changeIndex < 0) {
                    throw std::runtime_error(
                        "Could not find a change output from which BFXT token moving can be funded");
                }

                if (tx_.vout.at(changeIndex).nValue < MIN_TX_FEE) {
                    throw std::runtime_error(
                        "Insufficient balance in change to create fund moving an BFXT token");
                }

                CScript outputScript;
                outputScript.SetDestination(currentTokenAddress);
                // create a new output
                tx_.vout[changeIndex].nValue -= MIN_TX_FEE; // subtract
                tx_.vout.push_back(CTxOut(MIN_TX_FEE, outputScript));

                // create the transfer instruction
                BFXTScript::TransferInstruction ti;
                ti.amount    = inputTxBFXT.vout.at(inIndex).getToken(j).getAmount();
                ti.skipInput = false;
                // set the output index based on the number of outputs (since we added the last output)
                ti.outputIndex = tx_.vout.size() - 1;

                // push the transfer instruction
                TIs.push_back(ti);
            }
        }

        if (changeIndex + 1 > (int)tx_.vout.size()) {
            throw std::runtime_error("Invalid change index provided to fund moving BFXT tokens");
        }

        if (changeIndex < 0) {
            throw std::runtime_error(
                "Could not find a change output from which BFXT token moving can be funded");
        }

        if (tx_.vout.at(changeIndex).nValue < MIN_TX_FEE) {
            throw std::runtime_error(
                "Insufficient balance in change to create fund moving an BFXT token");
        }

        // if the change left equals exactly the amount required to create OP_RETURN, make it there
        bool setOpRetAtChangeOutput = false;
        if (tx_.vout[changeIndex].nValue == MIN_TX_FEE) {
            setOpRetAtChangeOutput = true;
        }

        std::shared_ptr<BFXTScript_Transfer> scriptPtrT = BFXTScript_Transfer::CreateScript(TIs, "");

        std::string script    = scriptPtrT->calculateScriptBin();
        std::string scriptHex = boost::algorithm::hex(script);

        CScript outputScript = CScript() << OP_RETURN << ParseHex(scriptHex);
        if (setOpRetAtChangeOutput) {
            // use the change output to move the tokens there
            tx_.vout[changeIndex] = CTxOut(MIN_TX_FEE, outputScript);
            // delete the unused index since change index was replaced
            tx_.vout.erase(tx_.vout.begin() + opRetIdx);
        } else {
            tx_.vout[opRetIdx] = CTxOut(MIN_TX_FEE, outputScript);
            tx_.vout[changeIndex].nValue -= MIN_TX_FEE; // subtract
        }
    }

    // copy the result to the input
    tx = tx_;
}

void BFXTTransaction::__manualSet(int NVersion, uint256 TxHash, std::vector<unsigned char> TxSerialized,
                                  std::vector<BFXTTxIn> Vin, std::vector<BFXTTxOut> Vout,
                                  uint64_t NLockTime, uint64_t NTime,
                                  BFXTTransactionType Ntp1TransactionType)
{
    nVersion = NVersion;
    txHash   = TxHash;
#ifdef DEBUG__INCLUDE_STR_HASH
    strHash = TxHash.ToString();
#endif
    txSerialized        = TxSerialized;
    vin                 = Vin;
    vout                = Vout;
    nLockTime           = NLockTime;
    nTime               = NTime;
    bfxtTransactionType = Ntp1TransactionType;
}

std::string BFXTTransaction::getBFXTOpReturnScriptHex() const
{
    // TODO: Sam: This has to be taken from a common source with the one from main
    const static std::string  BFXTOpReturnRegexStr = R"(^OP_RETURN\s+(4e54(?:01|03)[a-fA-F0-9]*)$)";
    const static boost::regex BFXTOpReturnRegex(BFXTOpReturnRegexStr);

    boost::smatch opReturnArgMatch;
    std::string   opReturnArg;

    for (unsigned long j = 0; j < vout.size(); j++) {
        std::string scriptPubKeyStr = vout[j].scriptPubKeyAsm;
        if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, BFXTOpReturnRegex)) {
            if (opReturnArgMatch[1].matched) {
                opReturnArg = std::string(opReturnArgMatch[1]);
                return opReturnArg;
            }
        }
    }
    throw std::runtime_error("Could not extract BFXT script from OP_RETURN for transaction " +
                             txHash.ToString());
}

void BFXTTransaction::readBFXTDataFromTx_minimal(const CTransaction& tx)
{
    txHash = tx.GetHash();
#ifdef DEBUG__INCLUDE_STR_HASH
    strHash = tx.GetHash().ToString();
#endif
    vin.clear();
    vin.resize(tx.vin.size());
    for (int i = 0; i < (int)tx.vin.size(); i++) {
        vin[i].setNull();
        vin[i].setPrevout(BFXTOutPoint(tx.vin[i].prevout.hash, tx.vin[i].prevout.n));
        std::string scriptSig;
        boost::algorithm::hex(tx.vin[i].scriptSig.begin(), tx.vin[i].scriptSig.end(),
                              std::back_inserter(scriptSig));
        vin[i].setScriptSigHex(scriptSig);
    }
    vout.clear();
    vout.resize(tx.vout.size());
    for (int i = 0; i < (int)tx.vout.size(); i++) {
        vout[i].nValue = tx.vout[i].nValue;
        vout[i].scriptPubKeyHex.clear();
        boost::algorithm::hex(tx.vout[i].scriptPubKey.begin(), tx.vout[i].scriptPubKey.end(),
                              std::back_inserter(vout[i].scriptPubKeyHex));
        vout[i].scriptPubKeyAsm = tx.vout[i].scriptPubKey.ToString();
        CTxDestination dest;
        if (ExtractDestination(tx.vout[i].scriptPubKey, dest)) {
            vout[i].setAddress(CBitcoinAddress(dest).ToString());
        } else {
            vout[i].setAddress("");
        }
    }
    bfxtTransactionType = BFXTTxType_NOT_BFXT;
}

void BFXTTransaction::readBFXTDataFromTx(
    const CTransaction& tx, const std::vector<std::pair<CTransaction, BFXTTransaction>>& inputsTxs)
{
    if (IsBFXTTxExcluded(tx.GetHash())) {
        return;
    }

    readBFXTDataFromTx_minimal(tx);

    std::string opReturnArg;
    if (!IsTxBFXT(&tx, &opReturnArg)) {
        bfxtTransactionType = BFXTTxType_NOT_BFXT;
        return;
    }

    if (tx.vin.size() != inputsTxs.size()) {
        throw std::runtime_error("The number of input transactions must match the number of inputs in "
                                 "the provided transaction. Error in tx: " +
                                 tx.GetHash().ToString());
    }

    uint64_t totalOutput = tx.GetValueOut();
    uint64_t totalInput  = 0;

    for (unsigned i = 0; i < tx.vin.size(); i++) {
        const auto& currInputHash  = tx.vin[i].prevout.hash;
        const auto& currInputIndex = tx.vin[i].prevout.n;

        vin[i].setPrevout(BFXTOutPoint(currInputHash, currInputIndex));

        // find inputs in the list of inputs and parse their OP_RETURN
        auto it = GetPrevInputIt(tx, vin[i].getPrevout().getHash(), inputsTxs);

        std::string opReturnArgInput;

        // The transaction that has an input that matches currInputHash
        const CTransaction&    currStdInput  = it->first;
        const BFXTTransaction& currBFXTInput = it->second;

        totalInput += currStdInput.vout.at(currInputIndex).nValue;

        // if the transaction is not BFXT, continue
        if (!IsTxBFXT(&currStdInput, &opReturnArgInput)) {
            continue;
        }

        // if the input is not an BFXT transaction, then currBFXTInput.vout.size() is zero
        if (currBFXTInput.vout.size() > 0) {
            vin[i].tokens = currBFXTInput.vout.at(currInputIndex).tokens;
        }
    }

    EnsureInputsHashesMatch(inputsTxs);

    EnsureInputTokensRelateToTx(tx, inputsTxs);

    if (totalInput == 0) {
        throw std::runtime_error("Total input is zero; that's invalid; in transaction: " +
                                 tx.GetHash().ToString());
    }

    this->nTime     = tx.nTime;
    this->nLockTime = tx.nLockTime;

    std::shared_ptr<BFXTScript> scriptPtr = BFXTScript::ParseScript(opReturnArg);
    if (scriptPtr->getTxType() == BFXTScript::TxType::TxType_Issuance) {
        bfxtTransactionType = BFXTTxType_ISSUANCE;

        std::shared_ptr<BFXTScript_Issuance> scriptPtrD =
            std::dynamic_pointer_cast<BFXTScript_Issuance>(scriptPtr);

        if (!scriptPtrD) {
            throw std::runtime_error(
                "While parsing BFXTTransaction, casting script pointer to transfer type failed: " +
                opReturnArg);
        }

        int64_t feeProvided = static_cast<int64_t>(totalInput) - static_cast<int64_t>(totalOutput);
        if (feeProvided < static_cast<int64_t>(IssuanceFee)) {
            throw std::runtime_error("Issuance fee is less than 10 nebls. You provided only: " +
                                     FormatMoney(feeProvided));
        }

        BFXTInt totalAmountLeft = scriptPtrD->getAmount();
        if (tx.vin.size() < 1) {
            throw std::runtime_error("Number of inputs is zero for transaction: " +
                                     tx.GetHash().ToString());
        }
        for (unsigned i = 0; i < scriptPtrD->getTransferInstructionsCount(); i++) {
            BFXTTokenTxData bfxttokenTxData;
            const auto&     instruction = scriptPtrD->getTransferInstruction(i);
            if (instruction.outputIndex >= tx.vout.size()) {
                throw std::runtime_error("An output of issuance is outside the available range of "
                                         "outputs in BFXT OP_RETURN argument: " +
                                         opReturnArg + ", where the number of available outputs is " +
                                         ::ToString(tx.vout.size()) + " in transaction " +
                                         tx.GetHash().ToString());
            }
            BFXTInt currentAmount = instruction.amount;

            // ensure the output is larger than input
            if (totalAmountLeft < currentAmount) {
                throw std::runtime_error("The amount targeted to outputs in bigger than the amount "
                                         "issued in BFXT OP_RETURN argument: " +
                                         opReturnArg);
            }

            totalAmountLeft -= currentAmount;
            bfxttokenTxData.setAmount(currentAmount);
            bfxttokenTxData.setAggregationPolicy(scriptPtrD->getAggregationPolicyStr());
            bfxttokenTxData.setDivisibility(scriptPtrD->getDivisibility());
            bfxttokenTxData.setTokenSymbol(scriptPtrD->getTokenSymbol());
            bfxttokenTxData.setLockStatus(scriptPtrD->isLocked());
            bfxttokenTxData.setIssueTxIdHex(tx.GetHash().ToString());
            bfxttokenTxData.setTokenId(
                scriptPtrD->getTokenID(tx.vin[0].prevout.hash.ToString(), tx.vin[0].prevout.n));
            vout[instruction.outputIndex].tokens.push_back(bfxttokenTxData);
        }

        // distribute the remainder of the issued tokens
        if (totalAmountLeft > 0) {
            if (vout.size() > 0) {
                BFXTTokenTxData bfxttokenTxData;

                bfxttokenTxData.setAmount(totalAmountLeft);
                totalAmountLeft = 0;
                bfxttokenTxData.setAggregationPolicy(scriptPtrD->getAggregationPolicyStr());
                bfxttokenTxData.setDivisibility(scriptPtrD->getDivisibility());
                bfxttokenTxData.setTokenSymbol(scriptPtrD->getTokenSymbol());
                bfxttokenTxData.setLockStatus(scriptPtrD->isLocked());
                bfxttokenTxData.setIssueTxIdHex(tx.GetHash().ToString());
                bfxttokenTxData.setTokenId(
                    scriptPtrD->getTokenID(tx.vin[0].prevout.hash.ToString(), tx.vin[0].prevout.n));
                vout.back().tokens.push_back(bfxttokenTxData);
            } else {
                throw std::runtime_error(
                    "Unable to send token change to the last output; the number of outputs is zero.");
            }
        }

        // loop over all inputs and add their tokens to the last output
        for (const auto& in : tx.vin) {
            const uint256&      currHash  = in.prevout.hash;
            const unsigned int& currIndex = in.prevout.n;
            // find the input tx from the list of inputs that matches the input hash from the tx in
            // question
            auto it = GetPrevInputIt(tx, currHash, inputsTxs);

            const std::pair<CTransaction, BFXTTransaction>& input = *it;

            for (int i = 0; i < (int)input.second.vout[currIndex].tokenCount(); i++) {
                if (vout.size() > 0) {
                    vout.back().tokens.push_back(input.second.vout[currIndex].getToken(i));
                } else {
                    throw std::runtime_error("Unable to send token change to the last output; the "
                                             "number of outputs is zero.");
                }
            }
        }
    } else if (scriptPtr->getTxType() == BFXTScript::TxType::TxType_Transfer) {
        bfxtTransactionType = BFXTTxType_TRANSFER;
        std::shared_ptr<BFXTScript_Transfer> scriptPtrD =
            std::dynamic_pointer_cast<BFXTScript_Transfer>(scriptPtr);

        if (!scriptPtrD) {
            throw std::runtime_error(
                "While parsing BFXTTransaction, casting script pointer to transfer type failed: " +
                opReturnArg);
        }

        __TransferTokens<BFXTScript_Transfer>(scriptPtrD, tx, inputsTxs, false);

    } else if (scriptPtr->getTxType() == BFXTScript::TxType::TxType_Burn) {
        bfxtTransactionType = BFXTTxType_BURN;
        std::shared_ptr<BFXTScript_Burn> scriptPtrD =
            std::dynamic_pointer_cast<BFXTScript_Burn>(scriptPtr);

        if (!scriptPtrD) {
            throw std::runtime_error(
                "While parsing BFXTTransaction, casting script pointer to burn type failed: " +
                opReturnArg);
        }

        __TransferTokens<BFXTScript_Burn>(scriptPtrD, tx, inputsTxs, true);

    } else {
        bfxtTransactionType = BFXTTxType_UNKNOWN;
        throw std::runtime_error("Unknown BFXT transaction type");
    }
}

json_spirit::Value BFXTTransaction::GetBFXTIssuanceMetadata(const uint256& issuanceTxid)
{
    CTransaction    tx = CTransaction::FetchTxFromDisk(issuanceTxid);
    BFXTTransaction bfxttx;
    bfxttx.readBFXTDataFromTx_minimal(tx);
    std::string opRet;
    bool        isBFXT = IsTxBFXT(&tx, &opRet);
    if (!isBFXT) {
        return json_spirit::Value();
    }

    std::shared_ptr<BFXTScript>          s  = BFXTScript::ParseScript(opRet);
    std::shared_ptr<BFXTScript_Issuance> sd = std::dynamic_pointer_cast<BFXTScript_Issuance>(s);
    if (!sd || s->getTxType() != BFXTScript::TxType_Issuance) {
        return json_spirit::Value();
    }
    if (s->getProtocolVersion() == 1) {
        if (tx.vin.empty()) {
            return json_spirit::Value();
        }
        const auto& prevout0 = tx.vin[0].prevout;
        try {
            std::string tokenId = bfxttx.getTokenIdIfIssuance(prevout0.hash.ToString(), prevout0.n);
            return GetBFXTv1IssuanceMetadataNode(tokenId);
        } catch (std::exception& ex) {
            return json_spirit::Value();
        }
    } else if (s->getProtocolVersion() == 3) {
        return BFXTScript::GetMetadataAsJson(sd.get(), tx);
    } else {
        return json_spirit::Value();
    }
}

BFXTTokenMetaData BFXTTransaction::GetFullBFXTIssuanceMetadata(const CTransaction&    issuanceTx,
                                                               const BFXTTransaction& bfxtIssuanceTx)
{
    if (issuanceTx.GetHash() != bfxtIssuanceTx.getTxHash()) {
        throw std::runtime_error("For GetFullBFXTIssuanceMetadata(), the BFXT transaction doesn't match "
                                 "the standard transaction");
    }
    uint256     issuanceTxid = issuanceTx.GetHash();
    std::string opRet;
    bool        isBFXT = IsTxBFXT(&issuanceTx, &opRet);
    if (!isBFXT) {
        throw std::runtime_error("A non-BFXT transaction was proided (txid: " + issuanceTxid.ToString() +
                                 ") to get BFXT issuance metadata");
    }

    std::shared_ptr<BFXTScript>          s  = BFXTScript::ParseScript(opRet);
    std::shared_ptr<BFXTScript_Issuance> sd = std::dynamic_pointer_cast<BFXTScript_Issuance>(s);
    if (!sd || s->getTxType() != BFXTScript::TxType_Issuance) {
        throw std::runtime_error("A non-issuance BFXT transaction was provided (txid: " +
                                 issuanceTxid.ToString() + ") to retrieve issuance metadata");
    }
    const auto& prevout0 = issuanceTx.vin[0].prevout;
    std::string tokenId  = bfxtIssuanceTx.getTokenIdIfIssuance(prevout0.hash.ToString(), prevout0.n);
    if (s->getProtocolVersion() == 1) {
        if (issuanceTx.vin.empty()) {
            throw std::runtime_error(
                "An invalid BFXT transaction was provided (txid: " + issuanceTxid.ToString() +
                ") to retrieve issuance metadata. The transaction has zero inputs.");
        }
        try {
            BFXTTokenMetaData result;
            result.readSomeDataFromStandardJsonFormat(GetBFXTv1IssuanceMetadataNode(tokenId));
            result.readSomeDataFromBFXTIssuanceScript(sd.get());
            result.setTokenId(tokenId);
            result.setIssuanceTxId(issuanceTxid);
            return result;
        } catch (std::exception& ex) {
            throw std::runtime_error("Failed to get BFXT transaction metadata for txid: " +
                                     issuanceTxid.ToString() + " . Error: " + std::string(ex.what()));
        }
    } else if (s->getProtocolVersion() == 3) {
        BFXTTokenMetaData result;
        result.readSomeDataFromStandardJsonFormat(BFXTScript::GetMetadataAsJson(sd.get(), issuanceTx));
        result.readSomeDataFromBFXTIssuanceScript(sd.get());
        result.setTokenId(tokenId);
        result.setIssuanceTxId(issuanceTxid);
        return result;
    } else {
        throw std::runtime_error("Failed to get BFXT transaction metadata for txid: " +
                                 issuanceTxid.ToString() + " . Unknown BFXT protocol version");
    }
}

BFXTTokenMetaData BFXTTransaction::GetFullBFXTIssuanceMetadata(const uint256& issuanceTxid)
{
    CTransaction    tx = CTransaction::FetchTxFromDisk(issuanceTxid);
    BFXTTransaction bfxttx;
    bfxttx.readBFXTDataFromTx_minimal(tx);
    CDataStream ds1(SER_NETWORK, PROTOCOL_VERSION);
    CDataStream ds2(SER_NETWORK, PROTOCOL_VERSION);
    return GetFullBFXTIssuanceMetadata(tx, bfxttx);
}

bool BFXTTransaction::TxContainsOpReturn(const CTransaction* tx, std::string* opReturnArg)
{
    if (!tx) {
        return false;
    }

    boost::smatch opReturnArgMatch;

    for (unsigned long j = 0; j < tx->vout.size(); j++) {
        // if the string OP_RET_STR is found in scriptPubKey
        std::string scriptPubKeyStr = tx->vout[j].scriptPubKey.ToString();
        if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, OpReturnRegex)) {
            if (opReturnArg != nullptr && opReturnArgMatch[1].matched) {
                *opReturnArg = std::string(opReturnArgMatch[1]);
                return true;
            }
            return true; // could not retrieve OP_RETURN argument
        }
    }
    return false;
}

std::vector<std::pair<CTransaction, BFXTTransaction>>
BFXTTransaction::GetAllBFXTInputsOfTx(CTransaction tx, bool recoverProtection, int recursionCount)
{
    CTxDB txdb;
    return GetAllBFXTInputsOfTx(
        tx, txdb, recoverProtection,
        std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>(),
        std::map<uint256, CTxIndex>(), recursionCount);
}

std::vector<std::pair<CTransaction, BFXTTransaction>>
BFXTTransaction::GetAllBFXTInputsOfTx(CTransaction tx, CTxDB& txdb, bool recoverProtection,
                                      int recursionCount)
{
    return GetAllBFXTInputsOfTx(
        tx, txdb, recoverProtection,
        std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>(),
        std::map<uint256, CTxIndex>(), recursionCount);
}

std::vector<std::pair<CTransaction, BFXTTransaction>> BFXTTransaction::GetAllBFXTInputsOfTx(
    CTransaction tx, CTxDB& txdb, bool recoverProtection,
    const std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>& mapQueuedBFXTInputs,
    const std::map<uint256, CTxIndex>& queuedAcceptedTxs, int recursionCount)
{
    // rertrieve standard transaction inputs (NOT BFXT)
    MapPrevTx mapInputs;
    bool      fInvalid = false;
    if (!tx.FetchInputs(txdb, queuedAcceptedTxs, true, false, mapInputs, fInvalid)) {
        if (fInvalid) {
            printf("Error: For GetAllBFXTInputsOfTx, FetchInputs found invalid tx %s\n",
                   tx.GetHash().ToString().c_str());
            throw std::runtime_error("Error: For BFXT, FetchInputs found invalid tx " +
                                     tx.GetHash().ToString());
        }
    }

    std::vector<std::pair<CTransaction, BFXTTransaction>> result = StdFetchedInputTxsToBFXT(
        tx, mapInputs, txdb, recoverProtection, mapQueuedBFXTInputs, queuedAcceptedTxs, recursionCount);
    return result;
}

std::vector<std::pair<CTransaction, BFXTTransaction>> BFXTTransaction::StdFetchedInputTxsToBFXT(
    const CTransaction& tx, const MapPrevTx& mapInputs, CTxDB& txdb, bool recoverProtection,
    const std::map<uint256, CTxIndex>& queuedAcceptedTxs, int recursionCount)
{
    // It's not possible to use default parameter here because BFXTTransaction is an incomplete type in
    // main.h, and including BFXTTransaction header file is not possible due to circular dependency
    return StdFetchedInputTxsToBFXT(
        tx, mapInputs, txdb, recoverProtection,
        std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>(), queuedAcceptedTxs,
        recursionCount);
}

std::vector<std::pair<CTransaction, BFXTTransaction>> BFXTTransaction::StdFetchedInputTxsToBFXT(
    const CTransaction& tx, const MapPrevTx& mapInputs, CTxDB& txdb, bool recoverProtection,
    const std::map<uint256, std::vector<std::pair<CTransaction, BFXTTransaction>>>& mapQueuedBFXTInputs,
    const std::map<uint256, CTxIndex>& queuedAcceptedTxs, int recursionCount)
{
    if (recursionCount >= 32) {
        throw std::runtime_error("Reached maximum recursion in StdFetchedInputTxsToBFXT");
    }

    std::vector<std::pair<CTransaction, BFXTTransaction>> inputsWithBFXT;
    // put the input transactions in a vector with their corresponding BFXT transactions
    {
        inputsWithBFXT.clear();
        std::transform(tx.vin.begin(), tx.vin.end(), std::back_inserter(inputsWithBFXT),
                       [&mapInputs, &tx](const CTxIn& in) {
                           if (mapInputs.count(in.prevout.hash) == 0) {
                               throw std::runtime_error("Could not find input after having fetched it "
                                                        "(for BFXT database storage); for tx: " +
                                                        tx.GetHash().ToString());
                           }
                           auto result = std::make_pair(mapInputs.find(in.prevout.hash)->second.second,
                                                        BFXTTransaction());
                           result.second.readBFXTDataFromTx_minimal(result.first);
                           return result;
                       });
    }

    // BFXT transaction data is either in the test pool OR in the database; no third option here
    auto it = mapQueuedBFXTInputs.find(tx.GetHash());
    if (it == mapQueuedBFXTInputs.end()) {
        for (auto&& inTx : inputsWithBFXT) {
            if (IsTxBFXT(&inTx.first)) {
                if (txdb.ContainsBFXTTx(inTx.first.GetHash())) {
                    // if the transaction is in the database, get it
                    FetchBFXTTxFromDisk(inTx, txdb, recoverProtection);
                } else if (queuedAcceptedTxs.find(inTx.first.GetHash()) != queuedAcceptedTxs.end()) {
                    // otherwise, if the transaction is already in the test pool, use it to read it

                    std::vector<std::pair<CTransaction, BFXTTransaction>> inputsOfInput =
                        GetAllBFXTInputsOfTx(inTx.first, txdb, recoverProtection, mapQueuedBFXTInputs,
                                             queuedAcceptedTxs, recursionCount + 1);

                    BFXTTransaction bfxttx;
                    inTx.second.readBFXTDataFromTx(inTx.first, inputsOfInput);
                } else {
                    // read BFXT transaction inputs. If they fail, that's OK, because they will
                    // fail later if they're necessary
                    FetchBFXTTxFromDisk(inTx, txdb, recoverProtection);
                }
            }
        }
    } else {
        inputsWithBFXT = it->second;
    }
    return inputsWithBFXT;
}

bool BFXTTransaction::IsTxBFXT(const CTransaction* tx, std::string* opReturnArg)
{
    if (!tx) {
        return false;
    }

    if (IsBFXTTxExcluded(tx->GetHash())) {
        return false;
    }

    boost::smatch opReturnArgMatch;

    for (unsigned long j = 0; j < tx->vout.size(); j++) {
        std::string scriptPubKeyStr = tx->vout[j].scriptPubKey.ToString();
        if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, BFXTOpReturnRegex)) {
            if (opReturnArg != nullptr && opReturnArgMatch[1].matched) {
                *opReturnArg = std::string(opReturnArgMatch[1]);
                return true;
            }
            return true; // could not retrieve OP_RETURN argument
        }
    }
    return false;
}

bool BFXTTransaction::IsTxOutputBFXTOpRet(const CTransaction* tx, unsigned int index,
                                          std::string* opReturnArg)
{
    if (!tx) {
        return false;
    }

    if (IsBFXTTxExcluded(tx->GetHash())) {
        return false;
    }

    boost::smatch opReturnArgMatch;

    // out of range index
    if (index + 1 >= tx->vout.size()) {
        return false;
    }

    std::string scriptPubKeyStr = tx->vout[index].scriptPubKey.ToString();
    if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, BFXTOpReturnRegex)) {
        if (opReturnArg != nullptr && opReturnArgMatch[1].matched) {
            *opReturnArg = std::string(opReturnArgMatch[1]);
            return true;
        }
        return true; // could not retrieve OP_RETURN argument
    }
    return false;
}

bool BFXTTransaction::IsTxOutputOpRet(const CTransaction* tx, unsigned int index,
                                      std::string* opReturnArg)
{
    if (!tx) {
        return false;
    }

    boost::smatch opReturnArgMatch;

    // out of range index
    if (index + 1 >= tx->vout.size()) {
        return false;
    }

    std::string scriptPubKeyStr = tx->vout[index].scriptPubKey.ToString();
    if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, OpReturnRegex)) {
        if (opReturnArg != nullptr && opReturnArgMatch[1].matched) {
            *opReturnArg = std::string(opReturnArgMatch[1]);
            return true;
        }
        return true; // could not retrieve OP_RETURN argument
    }
    return false;
}

bool BFXTTransaction::IsTxOutputOpRet(const CTxOut* output, std::string* opReturnArg)
{
    if (!output) {
        return false;
    }

    boost::smatch opReturnArgMatch;

    std::string scriptPubKeyStr = output->scriptPubKey.ToString();
    if (boost::regex_match(scriptPubKeyStr, opReturnArgMatch, OpReturnRegex)) {
        if (opReturnArg != nullptr && opReturnArgMatch[1].matched) {
            *opReturnArg = std::string(opReturnArgMatch[1]);
            return true;
        }
        return true; // could not retrieve OP_RETURN argument
    }
    return false;
}
