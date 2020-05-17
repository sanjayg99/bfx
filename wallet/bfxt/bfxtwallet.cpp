#include "bfxtwallet.h"

// the following is a necessary include for pwalletMain and CWalletTx objects
#include "init.h"
#include "main.h"

#include <boost/algorithm/hex.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/filesystem/fstream.hpp>
#include <boost/thread.hpp>

const std::string BFXTWallet::ICON_ERROR_CONTENT = "<DownloadError>";

boost::atomic<bool> appInitiated(false);

BFXTWallet::BFXTWallet()
{
    lastTxCount                  = 0;
    lastOutputsCount             = 0;
    updateBalance                = false;
    everSucceededInLoadingTokens = false;
}

void BFXTWallet::update()
{
    __getOutputs();
    if (updateBalance) {
        __RecalculateTokensBalances();
        everSucceededInLoadingTokens = true;
    }
}

bool BFXTWallet::getRetrieveFullMetadata() const { return retrieveFullMetadata; }

void BFXTWallet::setRetrieveFullMetadata(bool value) { retrieveFullMetadata = value; }

std::map<std::string, BFXTInt> BFXTWallet::getBalances() const { return balances; }

const std::unordered_map<std::string, BFXTTokenMetaData>& BFXTWallet::getTokenMetadataMap() const
{
    return tokenInformation;
}

void BFXTWallet::__getOutputs()
{
    // this helps in persisting to get the wallet data when the application is launched for the first
    // time and nebl wallet is null still the 100 number is just a protection against infinite waiting

    std::shared_ptr<CWallet> localWallet = std::atomic_load(&pwalletMain);
    for (int i = 0;
         i < 100 &&
         ((!everSucceededInLoadingTokens && std::atomic_load(&localWallet) == nullptr) || !appInitiated);
         i++) {
        boost::this_thread::sleep_for(boost::chrono::milliseconds(100));
    }

    if (std::atomic_load(&localWallet) == nullptr) {
        return;
    }

    std::vector<COutput> vecOutputs;
    std::atomic_load(&localWallet)->AvailableCoins(vecOutputs);

    // remove outputs that are outside confirmation bounds
    auto outputToRemoveIt =
        std::remove_if(vecOutputs.begin(), vecOutputs.end(), [this](const COutput& output) {
            if (maxConfirmations >= 0 && output.tx->GetDepthInMainChain() > maxConfirmations) {
                return true;
            }
            if (minConfirmations >= 0 && output.tx->GetDepthInMainChain() < minConfirmations) {
                return true;
            }
            return false;
        });
    vecOutputs.erase(outputToRemoveIt, vecOutputs.end());

    int64_t currTxCount      = 0;
    int64_t currOutputsCount = 0;
    {
        LOCK2(cs_main, std::atomic_load(&localWallet)->cs_wallet);

        currTxCount      = static_cast<int64_t>(std::atomic_load(&localWallet)->mapWallet.size());
        currOutputsCount = static_cast<int64_t>(vecOutputs.size());

        // if no new outputs are available
        if (lastTxCount == currTxCount && lastOutputsCount == currOutputsCount) {
            return;
        }
    }

    updateBalance = true;

    int failedRetrievals = 0;

    for (unsigned long i = 0; i < vecOutputs.size(); i++) {
        BFXTOutPoint output = ConvertNeblOutputToBFXT(vecOutputs[i]);
        uint256      txHash = output.getHash();

        // get the transaction from the wallet
        CWalletTx neblTx;
        if (!std::atomic_load(&localWallet)->GetTransaction(txHash, neblTx)) {
            printf("Error: Although the output number %i of transaction %s belongs to you, it couldn't "
                   "be found in your wallet.\n",
                   vecOutputs[i].i, txHash.ToString().c_str());
            continue;
        }

        // BFXT transactions strictly contain OP_RETURN in one of their vouts
        std::string opReturnArg;
        if (!BFXTTransaction::IsTxBFXT(&neblTx, &opReturnArg)) {
            continue;
        }

        // if output already exists, check if it's spent, if it's remove it
        if (removeOutputIfSpent(output, neblTx))
            continue;

        BFXTTransaction bfxttx;
        try {
            std::vector<std::pair<CTransaction, BFXTTransaction>> prevTxs =
                BFXTTransaction::GetAllBFXTInputsOfTx(neblTx, true);
            bfxttx.readBFXTDataFromTx(neblTx, prevTxs);
        } catch (std::exception& ex) {
            printf("Unable to download transaction information. Error says: %s\n", ex.what());
            failedRetrievals++;
            continue;
        }

        // include only BFXT transactions
        if (bfxttx.getTxOut(output.getIndex()).tokenCount() > 0) {
            try {
                // transaction with output index
                walletOutputsWithTokens[output] = bfxttx;
                for (long j = 0; j < static_cast<long>(bfxttx.getTxOut(output.getIndex()).tokenCount());
                     j++) {

                    BFXTTokenTxData tokenTx = bfxttx.getTxOut(output.getIndex()).getToken(j);

                    // find issue transaction to get meta data from
                    uint256      issueTxid = tokenTx.getIssueTxId();
                    CTransaction issueTx   = CTransaction::FetchTxFromDisk(issueTxid);
                    std::vector<std::pair<CTransaction, BFXTTransaction>> issueTxInputs =
                        BFXTTransaction::GetAllBFXTInputsOfTx(issueTx, true);
                    BFXTTransaction issueBFXTTx;
                    issueBFXTTx.readBFXTDataFromTx(issueTx, issueTxInputs);

                    // find the correct output in the issuance transaction that has the token in question
                    // issued
                    int  relevantIssueOutputIndex = -1;
                    bool stop                     = false;
                    for (int k = 0; k < (int)issueBFXTTx.getTxOutCount(); k++) {
                        for (int l = 0; l < (int)issueBFXTTx.getTxOut(k).tokenCount(); l++) {
                            if (issueBFXTTx.getTxOut(k).getToken(l).getTokenId() ==
                                tokenTx.getTokenId()) {
                                relevantIssueOutputIndex = k;
                                stop                     = true;
                                break;
                            }
                        }
                        if (stop) {
                            break;
                        }
                    }

                    if (relevantIssueOutputIndex < 0) {
                        throw std::runtime_error("Could not find the correct output index for token: " +
                                                 tokenTx.getTokenId());
                    }

                    if (retrieveFullMetadata) {
                        try {
                            tokenInformation[tokenTx.getTokenId()] =
                                BFXTTransaction::GetFullBFXTIssuanceMetadata(issueTxid);
                        } catch (std::exception& ex) {
                            printf("Failed to retrieve BFXT token metadata. Error: %s\n", ex.what());
                            tokenInformation[tokenTx.getTokenId()] =
                                GetMinimalMetadataInfoFromTxData(tokenTx);
                        } catch (...) {
                            printf("Failed to retrieve BFXT token metadata. Unknown exception.\n");
                            tokenInformation[tokenTx.getTokenId()] =
                                GetMinimalMetadataInfoFromTxData(tokenTx);
                        }
                    } else {
                        // no metadata available, set the name manually
                        tokenInformation[tokenTx.getTokenId()] =
                            GetMinimalMetadataInfoFromTxData(tokenTx);
                    }
                }
            } catch (std::exception& ex) {
                printf("Unable to download token metadata. Error says: %s\n", ex.what());
                continue;
            }
        }
    }

    scanSpentTransactions();

    lastTxCount      = currTxCount - failedRetrievals;
    lastOutputsCount = currOutputsCount - failedRetrievals;
}

void BFXTWallet::__RecalculateTokensBalances()
{
    balances.clear();
    for (std::unordered_map<BFXTOutPoint, BFXTTransaction>::iterator it =
             walletOutputsWithTokens.begin();
         it != walletOutputsWithTokens.end(); it++) {
        const BFXTTransaction& tx          = it->second;
        int                    outputIndex = it->first.getIndex();
        AddOutputToWalletBalance(tx, outputIndex, this->balances);
    }
    updateBalance = false;
}

void BFXTWallet::AddOutputToWalletBalance(const BFXTTransaction& tx, int outputIndex,
                                          std::map<std::string, BFXTInt>& balancesTable)
{
    for (long j = 0; j < static_cast<long>(tx.getTxOut(outputIndex).tokenCount()); j++) {
        BFXTTokenTxData    tokenTx = tx.getTxOut(outputIndex).getToken(j);
        const std::string& tokenID = tokenTx.getTokenId();
        if (balancesTable.find(tokenID) == balancesTable.end()) {
            balancesTable[tokenID] = tokenTx.getAmount();
        } else {
            balancesTable[tokenID] += tokenTx.getAmount();
        }
    }
}

bool BFXTWallet::removeOutputIfSpent(const BFXTOutPoint& output, const CWalletTx& neblTx)
{
    std::unordered_map<BFXTOutPoint, BFXTTransaction>::iterator outputIt =
        walletOutputsWithTokens.find(output);
    if (outputIt != walletOutputsWithTokens.end()) {
        if (neblTx.IsSpent(output.getIndex())) {
            walletOutputsWithTokens.erase(outputIt);
        }
        return true;
    }
    return false;
}

void BFXTWallet::scanSpentTransactions()
{
    std::shared_ptr<CWallet> localWallet = std::atomic_load(&pwalletMain);
    if (localWallet == nullptr)
        return;
    std::deque<BFXTOutPoint> toRemove;
    for (std::unordered_map<BFXTOutPoint, BFXTTransaction>::iterator it =
             walletOutputsWithTokens.begin();
         it != walletOutputsWithTokens.end(); it++) {
        CWalletTx      neblTx;
        int            outputIndex = it->first.getIndex();
        const uint256& txHash      = it->first.getHash();
        if (!localWallet->GetTransaction(txHash, neblTx))
            continue;
        if (neblTx.IsSpent(outputIndex)) {
            // this, although the right way to do things, causes a crash. A safer plan is chosen
            // it = walletOutputsWithTokens.erase(it);
            toRemove.push_back(it->first);
        }
    }
    for (unsigned long i = 0; i < toRemove.size(); i++) {
        if (walletOutputsWithTokens.find(toRemove[i]) != walletOutputsWithTokens.end()) {
            walletOutputsWithTokens.erase(toRemove[i]);
        } else {
            printf("Unable to find output %s:%s, although it was found before and marked for "
                   "removal.\n",
                   toRemove[i].getHash().ToString().c_str(), ToString(toRemove[i].getIndex()).c_str());
            //            std::cerr<<"Unable to find output " << toRemove[i].getHash().ToString() <<
            //            ":"
            //            << ToString(toRemove[i].getIndex()) << ", although it was found before and
            //            marked for removal." << std::endl;
        }
    }
}

BFXTOutPoint BFXTWallet::ConvertNeblOutputToBFXT(const COutput& output)
{
    return BFXTOutPoint(output.tx->GetHash(), output.i);
}

BFXTTokenMetaData BFXTWallet::GetMinimalMetadataInfoFromTxData(const BFXTTokenTxData& tokenTx)
{
    BFXTTokenMetaData res;
    res.setTokenName(tokenTx.getTokenSymbol());
    res.setTokenId(tokenTx.getTokenId());
    res.setIssuanceTxIdHex(tokenTx.getIssueTxId().ToString());
    return res;
}

std::string BFXTWallet::getTokenName(const std::string& tokenID) const
{
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator it =
        tokenInformation.find(tokenID);
    if (it == tokenInformation.end()) {
        return std::string("<NameError>");
    } else {
        return it->second.getTokenName();
    }
}

BFXTInt BFXTWallet::getTokenBalance(const std::string& tokenID) const
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.find(tokenID);
    if (it == balances.end()) {
        return 0;
    } else {
        return static_cast<int64_t>(it->second);
    }
}

std::string BFXTWallet::getTokenName(int index) const
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator itToken =
        tokenInformation.find(it->first);
    if (itToken == tokenInformation.end()) {
        return std::string("<NameError>");
    } else {
        return itToken->second.getTokenName();
    }
}

std::string BFXTWallet::getTokenId(int index) const
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator itToken =
        tokenInformation.find(it->first);
    if (itToken == tokenInformation.end()) {
        return std::string("<TokenIdError>");
    } else {
        return itToken->second.getTokenId();
    }
}

std::string BFXTWallet::getTokenIssuanceTxid(int index) const
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator itToken =
        tokenInformation.find(it->first);
    if (itToken == tokenInformation.end()) {
        return ""; // empty to detect error automatically (not viewable by user)
    } else {
        return itToken->second.getIssuanceTxId().ToString();
    }
}

std::string BFXTWallet::getTokenDescription(int index) const
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator itToken =
        tokenInformation.find(it->first);
    if (itToken == tokenInformation.end()) {
        return std::string("<DescError>");
    } else {
        return itToken->second.getTokenDescription();
    }
}

BFXTInt BFXTWallet::getTokenBalance(int index) const
{
    if (index > getNumberOfTokens())
        return 0;
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    if (it == balances.end()) {
        return 0;
    } else {
        return static_cast<int64_t>(it->second);
    }
}

std::string BFXTWallet::__downloadIcon(const std::string& IconURL)
{
    try {
        return cURLTools::GetFileFromHTTPS(IconURL, 30, false);
    } catch (std::exception& ex) {
        printf("Error: Failed at downloading icon from %s. Error says: %s\n", IconURL.c_str(),
               ex.what());
        return ICON_ERROR_CONTENT;
    }
}

void BFXTWallet::__asyncDownloadAndSetIcon(std::string IconURL, std::string tokenId,
                                           boost::shared_ptr<BFXTWallet> wallet)
{
    wallet->tokenIcons.set(tokenId, __downloadIcon(IconURL));
}

std::string BFXTWallet::getTokenIcon(int index)
{
    std::map<std::string, BFXTInt>::const_iterator it = balances.begin();
    std::advance(it, index);
    std::string                                                        tokenId = it->first;
    std::unordered_map<std::string, BFXTTokenMetaData>::const_iterator itToken =
        tokenInformation.find(tokenId);
    if (itToken == tokenInformation.end()) {
        return "";
    }
    if (!tokenIcons.exists(tokenId)) {
        const std::string& IconURL = itToken->second.getIconURL();
        if (IconURL.empty()) {
            // no icon URL provided, set empty icon and return
            tokenIcons.set(tokenId, "");
            return "";
        }
        boost::thread IconDownloadThread(
            boost::bind(__asyncDownloadAndSetIcon, IconURL, tokenId, shared_from_this()));
        IconDownloadThread.detach();
        return "";
    } else {
        std::string icon;
        tokenIcons.get(tokenId, icon);
        // if there was an error getting the icon OR the icon is empty, and a download URL now
        // exists, download again
        if (icon == ICON_ERROR_CONTENT || (icon == "" && !itToken->second.getIconURL().empty())) {
            const std::string& IconURL = itToken->second.getIconURL();
            boost::thread      IconDownloadThread(
                boost::bind(__asyncDownloadAndSetIcon, IconURL, tokenId, shared_from_this()));
            IconDownloadThread.detach();
        }

        tokenIcons.get(tokenId, icon);
        return icon;
    }
}

int64_t BFXTWallet::getNumberOfTokens() const { return balances.size(); }

const std::map<std::string, BFXTInt>& BFXTWallet::getBalancesMap() const { return balances; }

const std::unordered_map<BFXTOutPoint, BFXTTransaction>& BFXTWallet::getWalletOutputsWithTokens()
{
    return walletOutputsWithTokens;
}

bool BFXTWallet::hasEverSucceeded() const { return everSucceededInLoadingTokens; }

bool BFXTWallet::IconHasErrorContent(const std::string& icon) { return icon == ICON_ERROR_CONTENT; }

void BFXTWallet::clear()
{
    tokenInformation.clear();
    walletOutputsWithTokens.clear();
    tokenIcons.clear();
    balances.clear();
    lastTxCount      = 0;
    lastOutputsCount = 0;
}

void BFXTWallet::setMinMaxConfirmations(int minConfs, int maxConfs)
{
    minConfirmations = minConfs;
    maxConfirmations = maxConfs;
}

std::string BFXTWallet::Serialize(const BFXTWallet& wallet)
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("token_info", SerializeMap(wallet.tokenInformation, false, false)));
    root.push_back(
        json_spirit::Pair("outputs", SerializeMap(wallet.walletOutputsWithTokens, false, false)));
    root.push_back(
        json_spirit::Pair("icons", SerializeMap(wallet.tokenIcons.getInternalMap(), false, true)));
    root.push_back(json_spirit::Pair("balances", SerializeMap(wallet.balances, false, false)));

    return json_spirit::write_formatted(root);
}

BFXTWallet BFXTWallet::Deserialize(const std::string& data)
{
    BFXTWallet result;

    json_spirit::Value parsedData;
    json_spirit::read_or_throw(data, parsedData);

    json_spirit::Value tokenInfoData(BFXTTools::GetObjectField(parsedData.get_obj(), "token_info"));
    result.tokenInformation =
        DeserializeMap<std::unordered_map<std::string, BFXTTokenMetaData>>(tokenInfoData, false, false);
    json_spirit::Value outputsData(BFXTTools::GetObjectField(parsedData.get_obj(), "outputs"));
    result.walletOutputsWithTokens =
        DeserializeMap<std::unordered_map<BFXTOutPoint, BFXTTransaction>>(outputsData, false, false);
    json_spirit::Value iconsData(BFXTTools::GetObjectField(parsedData.get_obj(), "icons"));
    result.tokenIcons.setInternalMap(
        DeserializeMap<std::unordered_map<std::string, std::string>>(iconsData, false, true));
    json_spirit::Value balancesData(BFXTTools::GetObjectField(parsedData.get_obj(), "balances"));
    result.balances = DeserializeMap<std::map<std::string, BFXTInt>>(balancesData, false, false);

    return result;
}

void BFXTWallet::exportToFile(const boost::filesystem::path& filePath) const
{
    std::string                output = Serialize(*this);
    boost::filesystem::fstream fileObj(filePath, std::ios::out);
    fileObj.write(output.c_str(), output.size());
    fileObj.close();
}

void BFXTWallet::importFromFile(const boost::filesystem::path& filePath)
{
    boost::filesystem::fstream fileObj(filePath, std::ios::in);
    std::string data((std::istreambuf_iterator<char>(fileObj)), std::istreambuf_iterator<char>());
    fileObj.close();
    this->clear();
    *this = Deserialize(data);
}

std::string BFXTWallet::__KeyToString(const std::string& str, bool serialize)
{
    if (serialize) {
        std::string res;
        boost::algorithm::hex(str.begin(), str.end(), std::back_inserter(res));
        return res;
    } else {
        return str;
    }
}

void BFXTWallet::__KeyFromString(const std::string& str, bool deserialize, std::string& result)
{
    if (deserialize) {
        result.clear();
        boost::algorithm::unhex(str.begin(), str.end(), std::back_inserter(result));
    } else {
        result = str;
    }
}

std::string BFXTWallet::__KeyToString(const BFXTOutPoint& op, bool)
{
    return op.getHash().ToString() + ":" + ToString(op.getIndex());
}

void BFXTWallet::__KeyFromString(const std::string& outputString, bool /*deserialize*/,
                                 BFXTOutPoint&      result)
{
    std::vector<std::string> strs;
    boost::split(strs, outputString, boost::is_any_of(":"));
    if (strs.size() != 2) {
        throw std::runtime_error("The output string is of invalid format. The string given is: \"" +
                                 outputString + "\". The correct format is hash:index");
    }
    uint256 hash;
    hash.SetHex(strs.at(0));
    result = BFXTOutPoint(hash, FromString<unsigned int>(strs.at(1)));
}

json_spirit::Value BFXTWallet::__ValToJson(const BFXTTokenMetaData& input, bool)
{
    return input.exportDatabaseJsonData();
}

void BFXTWallet::__ValFromJson(const json_spirit::Value& input, bool /*deserialize*/,
                               BFXTTokenMetaData&        result)
{
    result.setNull();
    result.importDatabaseJsonData(input);
}

json_spirit::Value BFXTWallet::__ValToJson(const BFXTTransaction& input, bool)
{
    return input.exportDatabaseJsonData();
}

void BFXTWallet::__ValFromJson(const json_spirit::Value& input, bool /*deserialize*/,
                               BFXTTransaction&          result)
{
    result.setNull();
    result.importDatabaseJsonData(input);
}

json_spirit::Value BFXTWallet::__ValToJson(const std::string& input, bool serialize)
{
    if (serialize) {
        std::string res;
        boost::algorithm::hex(input.begin(), input.end(), std::back_inserter(res));
        return res;
    } else {
        return input;
    }
}

void BFXTWallet::__ValFromJson(const json_spirit::Value& input, bool deserialize, std::string& result)
{
    if (deserialize) {
        result.clear();
        std::string inputStr = input.get_str();
        boost::algorithm::unhex(inputStr.begin(), inputStr.end(), std::back_inserter(result));
    } else {
        result = input.get_str();
    }
}

json_spirit::Value BFXTWallet::__ValToJson(const int64_t& input, bool)
{
    return json_spirit::Value(input);
}

json_spirit::Value BFXTWallet::__ValToJson(const BFXTInt& input, bool)
{
    return json_spirit::Value(ToString(input));
}

void BFXTWallet::__ValFromJson(const json_spirit::Value& input, bool /*deserialize*/, int64_t& result)
{
    result = input.get_int64();
}

void BFXTWallet::__ValFromJson(const json_spirit::Value& input, bool /*deserialize*/, BFXTInt& result)
{
    result = FromString<BFXTInt>(input.get_str());
}

template <typename Container>
json_spirit::Value BFXTWallet::SerializeMap(const Container& TheMap, bool serializeKey,
                                            bool serializeValue)
{
    json_spirit::Object json_obj;
    for (typename Container::const_iterator it = TheMap.begin(); it != TheMap.end(); it++) {
        std::string        first  = __KeyToString(it->first, serializeKey);
        json_spirit::Value second = __ValToJson(it->second, serializeValue);
        json_obj.push_back(json_spirit::Pair(first, second));
    }
    json_spirit::Value json_value(json_obj);
    return json_value;
}

template <typename Container>
Container BFXTWallet::DeserializeMap(const json_spirit::Value& json_val, bool deserializeKey,
                                     bool deserializeValue)
{
    Container           result;
    json_spirit::Object json_obj = json_val.get_obj();
    for (typename json_spirit::Object::const_iterator it = json_obj.begin(); it != json_obj.end();
         ++it) {
        typename Container::key_type first;
        __KeyFromString(it->name_, deserializeKey, first);
        typename Container::mapped_type second;
        __ValFromJson(it->value_, deserializeValue, second);
        result[first] = second;
    }
    return result;
}
