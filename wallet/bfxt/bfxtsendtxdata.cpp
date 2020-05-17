#include "bfxtsendtxdata.h"

#include "bfxtsendtxdata.h"

#include "init.h"
#include "util.h"
#include "wallet.h"
#include "json/json_spirit.h"
#include <algorithm>
#include <random>

const std::string BFXTSendTxData::NEBL_TOKEN_ID = "BFX";
// token id of new non-existent token (placeholder)
const std::string BFXTSendTxData::TO_ISSUE_TOKEN_ID = "NEW";

std::vector<BFXTOutPoint> BFXTSendTxData::getUsedInputs() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get used inputs");
    return tokenSourceInputs;
}

std::map<std::string, BFXTInt> BFXTSendTxData::getChangeTokens() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get change amounts");
    return totalChangeTokens;
}

BFXTSendTxData::BFXTSendTxData()
{ /*fee = 0;*/
}

std::map<std::string, BFXTInt>
CalculateRequiredTokenAmounts(const std::vector<BFXTSendTokensOneRecipientData>& recipients)
{
    std::map<std::string, BFXTInt> required_amounts;
    for (const auto& r : recipients) {
        if (r.tokenId == BFXTSendTxData::TO_ISSUE_TOKEN_ID) {
            // there's no required BFXT token amount for issuance, unlike transfer, because we're minting
            continue;
        }
        if (required_amounts.find(r.tokenId) == required_amounts.end()) {
            required_amounts[r.tokenId] = 0;
        }
        required_amounts[r.tokenId] += r.amount;
    }
    return required_amounts;
}

void BFXTSendTxData::verifyBFXTIssuanceRecipientsValidity(
    const std::vector<BFXTSendTokensOneRecipientData>& recipients)
{
    int issuanceCount = 0;
    for (const BFXTSendTokensOneRecipientData& r : recipients) {
        if (r.tokenId == BFXTSendTxData::TO_ISSUE_TOKEN_ID) {
            issuanceCount++;
        }
    }
    if (issuanceCount > 1) {
        throw std::runtime_error("Only one recipient of an issuance transaction can be present.");
    }
    if (issuanceCount > 0 && !tokenToIssueData) {
        throw std::runtime_error("While a recipient was spicified to receive newly minted tokens, no "
                                 "issuance data was speicified.");
    }
    if (issuanceCount == 0 && tokenToIssueData) {
        throw std::runtime_error("While issuance data was provided, no recipient for issued/minted "
                                 "tokens was specified in the list of recipients.");
    }
}

// get available balances, either from inputs (if provided) or from the wallet
std::map<std::string, BFXTInt> GetAvailableTokenBalances(boost::shared_ptr<BFXTWallet>    wallet,
                                                         const std::vector<BFXTOutPoint>& inputs,
                                                         bool useBalancesFromWallet)
{
    std::map<std::string, BFXTInt> balancesMap;
    if (useBalancesFromWallet) {
        // get token balances from the wallet
        balancesMap = wallet->getBalancesMap();
    } else {
        // loop over all inputs and collect the total amount of tokens available
        for (const auto& input : inputs) {
            std::unordered_map<BFXTOutPoint, BFXTTransaction> availableOutputsMap =
                wallet->getWalletOutputsWithTokens();
            auto it = availableOutputsMap.find(input);
            if (it != availableOutputsMap.end()) {
                const BFXTTransaction& bfxttx = it->second;
                if (input.getIndex() + 1 > bfxttx.getTxOutCount()) {
                    throw std::runtime_error("An output you have of transaction " +
                                             bfxttx.getTxHash().ToString() +
                                             " claims that you have an invalid output number: " +
                                             ::ToString(input.getIndex()));
                }
                // loop over tokens
                for (int i = 0; i < (int)bfxttx.getTxOut(input.getIndex()).tokenCount(); i++) {
                    const BFXTTokenTxData& tokenT    = bfxttx.getTxOut(input.getIndex()).getToken(i);
                    auto                   balanceIt = balancesMap.find(tokenT.getTokenId());
                    if (balanceIt == balancesMap.end()) {
                        balancesMap[tokenT.getTokenId()] = 0;
                    }
                    balancesMap[tokenT.getTokenId()] += tokenT.getAmount();
                }
            }
        }
    }
    return balancesMap;
}

int64_t CalculateTotalNeblsInInputs(std::vector<BFXTOutPoint> inputs)
{
    {
        std::unordered_set<BFXTOutPoint> inputsSet(inputs.begin(), inputs.end());
        inputs = std::vector<BFXTOutPoint>(inputsSet.begin(), inputsSet.end());
    }

    int64_t currentTotalNeblsInSelectedInputs = 0;
    for (const auto& input : inputs) {
        auto it = pwalletMain->mapWallet.find(input.getHash());
        if (it == pwalletMain->mapWallet.end()) {
            throw std::runtime_error("The transaction: " + input.getHash().ToString() +
                                     " was not found in the wallet.");
        }

        const CTransaction& tx = it->second;
        if (input.getIndex() + 1 > tx.vout.size()) {
            throw std::runtime_error("An invalid output index: " + ::ToString(input.getIndex()) +
                                     " of transaction " + input.getHash().ToString() + " was used.");
        }
        currentTotalNeblsInSelectedInputs += static_cast<int64_t>(tx.vout.at(input.getIndex()).nValue);
    }
    return currentTotalNeblsInSelectedInputs;
}

void BFXTSendTxData::selectBFXTTokens(boost::shared_ptr<BFXTWallet>                      wallet,
                                      const std::vector<COutPoint>&                      inputs,
                                      const std::vector<BFXTSendTokensOneRecipientData>& recipients,
                                      bool addMoreInputsIfRequired)
{
    std::vector<BFXTOutPoint> bfxtOutPoints;
    std::transform(inputs.begin(), inputs.end(), std::back_inserter(bfxtOutPoints),
                   [](const COutPoint& o) { return BFXTOutPoint(o.hash, o.n); });

    selectBFXTTokens(wallet, bfxtOutPoints, recipients, addMoreInputsIfRequired);
}

void BFXTSendTxData::issueBFXTToken(const IssueTokenData& data)
{
    if (ready) {
        throw std::runtime_error("You should register issuing a token before processing BFXT tokens, in "
                                 "order for the new tokens to be taken into account");
    }
    tokenToIssueData = data;
}

boost::optional<IssueTokenData> BFXTSendTxData::getBFXTTokenIssuanceData() const
{
    return tokenToIssueData;
}

bool BFXTSendTxData::getWhetherIssuanceExists() const { return tokenToIssueData.is_initialized(); }

void BFXTSendTxData::selectBFXTTokens(boost::shared_ptr<BFXTWallet>               wallet,
                                      std::vector<BFXTOutPoint>                   inputs,
                                      std::vector<BFXTSendTokensOneRecipientData> recipients,
                                      bool addMoreInputsIfRequired)
{
    totalTokenAmountsInSelectedInputs.clear();
    tokenSourceInputs.clear();
    totalChangeTokens.clear();
    intermediaryTIs.clear();
    recipientsList.clear();
    usedWallet.reset();

    // remove non-BFXT recipients (nebl recipients)
    recipients.erase(std::remove_if(recipients.begin(), recipients.end(),
                                    [](const BFXTSendTokensOneRecipientData& r) {
                                        return (r.tokenId == BFXTSendTxData::NEBL_TOKEN_ID);
                                    }),
                     recipients.end());

    verifyBFXTIssuanceRecipientsValidity(recipients);

    // remove inputs duplicates
    {
        std::unordered_set<BFXTOutPoint> inputsSet(inputs.begin(), inputs.end());
        inputs = std::vector<BFXTOutPoint>(inputsSet.begin(), inputsSet.end());
    }

    // collect all required amounts in one map, with tokenId vs amount
    std::map<std::string, BFXTInt> targetAmounts = CalculateRequiredTokenAmounts(recipients);

    // get available balances, either from inputs (if provided) or from the wallet
    std::map<std::string, BFXTInt> balancesMap =
        GetAvailableTokenBalances(wallet, inputs, addMoreInputsIfRequired);

    // check whether the required amounts can be covered by the available balances
    for (const auto& required_amount : targetAmounts) {
        if (required_amount.first == BFXTSendTxData::NEBL_TOKEN_ID) {
            // ignore nebls, deal only with tokens
            continue;
        }
        if (required_amount.first == BFXTSendTxData::TO_ISSUE_TOKEN_ID) {
            // ignore newly issued tokens, as no inputs will ever satisfy them
            continue;
        }
        auto available_balance = balancesMap.find(required_amount.first);
        if (available_balance != balancesMap.end()) {
            if (required_amount.second > available_balance->second) {
                throw std::runtime_error("Your balance/selected inputs is not sufficient to cover for " +
                                         wallet->getTokenName(required_amount.first));
            }
        } else {
            throw std::runtime_error("You're trying to spend tokens that you don't own or are not "
                                     "included in the inputs you selected; namely: " +
                                     wallet->getTokenName(required_amount.first));
        }
    }

    // calculate reserved balances to be used in this transaction
    const std::unordered_map<BFXTOutPoint, BFXTTransaction> walletOutputsMap =
        wallet->getWalletOutputsWithTokens();
    std::deque<BFXTOutPoint> availableOutputs;
    if (addMoreInputsIfRequired) {
        // assume that inputs automatically has to be gathered from the wallet
        for (const auto& el : walletOutputsMap) {
            availableOutputs.push_back(el.first);
        }
        for (const auto& el : inputs) {
            tokenSourceInputs.push_back(el);
        }
    } else {
        for (const auto& el : inputs) {
            tokenSourceInputs.push_back(el);
            availableOutputs.push_back(el);
        }
    }

    // remove inputs duplicates
    {
        std::unordered_set<BFXTOutPoint> inputsSet(tokenSourceInputs.begin(), tokenSourceInputs.end());
        tokenSourceInputs = std::vector<BFXTOutPoint>(inputsSet.begin(), inputsSet.end());
    }

    {
        std::random_device rd;
        std::mt19937       g(rd());
        // to improve privacy, shuffle inputs; pseudo-random is good enough here
        std::shuffle(availableOutputs.begin(), availableOutputs.end(), g);
    }

    // this container will be filled and must have tokens that are higher than the required amounts
    // reset fulfilled amounts and change to zero
    for (const std::pair<std::string, BFXTInt>& el : targetAmounts) {
        totalTokenAmountsInSelectedInputs[el.first] = 0;
    }

    // fill tokenSourceInputs if inputs are not given
    for (const std::pair<std::string, BFXTInt>& targetAmount : targetAmounts) {
        for (int i = 0; i < (int)availableOutputs.size(); i++) {
            const auto& output   = availableOutputs.at(i);
            auto        bfxtTxIt = walletOutputsMap.find(output);
            if (bfxtTxIt == walletOutputsMap.end()) {
                // if the output is not found the BFXT wallet outputs, it means that it doesn't have BFXT
                // tokens, so skip
                continue;
            }
            const BFXTTransaction& txData    = bfxtTxIt->second;
            const BFXTTxOut&       bfxttxOut = txData.getTxOut(output.getIndex());

            auto numOfTokensInOutput = bfxttxOut.tokenCount();
            bool takeThisOutput      = false;
            if (addMoreInputsIfRequired) {
                for (auto i = 0u; i < numOfTokensInOutput; i++) {
                    std::string outputTokenId = bfxttxOut.getToken(i).getTokenId();
                    // if token id matches in the transaction with the required one, take it into account
                    BFXTInt required_amount_still =
                        targetAmount.second - totalTokenAmountsInSelectedInputs[outputTokenId];
                    if (targetAmount.first == outputTokenId && required_amount_still > 0) {
                        takeThisOutput = true;
                        break;
                    }
                }
            } else {
                // take all prev outputs
                takeThisOutput = true;
            }

            // take this transaction by
            // 1. remove it from the vector of available outputs
            // 2. add its values to fulfilledTokenAmounts
            // 3. add the address to the list of inputs to use (pointless if a list of inputs was
            // provided)
            if (takeThisOutput) {
                for (auto i = 0u; i < numOfTokensInOutput; i++) {
                    std::string outputTokenId = bfxttxOut.getToken(i).getTokenId();
                    totalTokenAmountsInSelectedInputs[outputTokenId] +=
                        bfxttxOut.getToken(i).getAmount();
                }
                tokenSourceInputs.push_back(output);
                availableOutputs.erase(availableOutputs.begin() + i);
                i--;
                if (availableOutputs.size() == 0) {
                    break;
                }
            }
        }
    }

    recipientsList.assign(recipients.begin(), recipients.end());

    // remove empty elements from total from inputs
    for (auto it = totalTokenAmountsInSelectedInputs.begin();
         it != totalTokenAmountsInSelectedInputs.end();) {
        if (it->second == 0)
            it = totalTokenAmountsInSelectedInputs.erase(it);
        else
            ++it;
    }

    // remove inputs duplicates
    {
        std::unordered_set<BFXTOutPoint> inputsSet(tokenSourceInputs.begin(), tokenSourceInputs.end());
        tokenSourceInputs = std::vector<BFXTOutPoint>(inputsSet.begin(), inputsSet.end());
    }

    const std::unordered_map<BFXTOutPoint, BFXTTransaction> walletOutputs =
        wallet->getWalletOutputsWithTokens();

    // sort inputs by which has more tokens first
    std::sort(
        tokenSourceInputs.begin(), tokenSourceInputs.end(),
        [&walletOutputs](const BFXTOutPoint& o1, const BFXTOutPoint& o2) {
            auto it1    = walletOutputs.find(o1);
            auto it2    = walletOutputs.find(o2);
            int  count1 = 0;
            int  count2 = 0;
            if (it1 != walletOutputs.end()) {
                const BFXTTransaction& tx1 = it1->second;
                if (o1.getIndex() + 1 > tx1.getTxOutCount()) {
                    throw std::runtime_error(
                        "While sorting inputs in BFXT selector, output index is out of range for: " +
                        o1.getHash().ToString() + ":" + ::ToString(o1.getIndex()));
                }
                count1 = tx1.getTxOut(o1.getIndex()).tokenCount();
            }
            if (it2 != walletOutputs.end()) {
                const BFXTTransaction& tx2 = it2->second;
                if (o2.getIndex() + 1 > tx2.getTxOutCount()) {
                    throw std::runtime_error(
                        "While sorting inputs in BFXT selector, output index is out of range for: " +
                        o2.getHash().ToString() + ":" + ::ToString(o2.getIndex()));
                }
                count2 = tx2.getTxOut(o2.getIndex()).tokenCount();
            }
            return count1 > count2;
        });

    // this map has depletable balances to be consumed while filling TIs
    std::unordered_map<BFXTOutPoint, BFXTTxOut> decreditMap;
    for (const auto& in : tokenSourceInputs) {
        // get the output
        auto it = walletOutputs.find(in);

        if (it == walletOutputs.end()) {
            // No BFXT token in this input
            continue;
        }
        // extract the transaction from the output
        const BFXTTransaction& bfxttx = it->second;
        if (in.getIndex() + 1 > bfxttx.getTxOutCount()) {
            throw std::runtime_error(
                "While attempting to credit recipients, input index is out of range for: " +
                in.getHash().ToString() + ":" + ::ToString(in.getIndex()));
        }

        decreditMap[in] = bfxttx.getTxOut(in.getIndex());
    }

    // if this is an issuance transaction, add the issuance TI
    if (tokenToIssueData.is_initialized()) {
        IntermediaryTI iti;

        BFXTScript::TransferInstruction ti;

        // issuance output is always the first one (will be transformed in CreateTransaction)
        ti.outputIndex = 0;
        ti.skipInput   = false;
        ti.amount      = tokenToIssueData.get().amount;

        iti.isBFXTTokenIssuance = true;
        iti.TIs.push_back(ti);

        intermediaryTIs.push_back(iti);
    }

    // copy of the recipients to deduce the amounts they recieved
    std::vector<BFXTSendTokensOneRecipientData> recps = recipients;

    // for every input, for every BFXT token kind, move them to the recipients
    // loop u: looping over inputs
    // loop i: looping over token kinds inside input "u"
    // loop j: looping over recipients, and give them the tokens they require,
    //         from input "u", and token kind "i"
    for (int u = 0; u < (int)tokenSourceInputs.size(); u++) {
        const auto& in = tokenSourceInputs[u];

        IntermediaryTI iti;
        iti.input = in;

        // "in" is guaranteed to be in the map because it comes from tokenSourceInputs
        BFXTTxOut& bfxttxOut = decreditMap[in];
        for (int i = 0; i < (int)bfxttxOut.tokenCount(); i++) {
            BFXTTokenTxData& token = bfxttxOut.getToken(i);
            for (int j = 0; j < (int)recps.size(); j++) {

                // if the token id matches and the recipient needs more, give them that amount (by
                // substracting the amount from the recipient)
                if (bfxttxOut.getToken(i).getTokenId() == recps[j].tokenId && recps[j].amount > 0) {

                    if (recps[j].tokenId == TO_ISSUE_TOKEN_ID) {
                        throw std::runtime_error("An issuance transaction cannot have transfer elements "
                                                 "in it except for the issued transaction. Everything "
                                                 "else should go into change.");
                    }

                    BFXTScript::TransferInstruction ti;

                    // there's still more for the recipient. Aggregate from possible adjacent tokens!
                    // aggregation: loop over inputs and tokens, check the ids, and add them to the
                    // current recipient
                    bool stop = false;
                    for (int v = u; v < (int)tokenSourceInputs.size(); v++) {
                        // "inComp" is guaranteed to be in the map because it comes from
                        // tokenSourceInputs
                        const auto& inComp        = tokenSourceInputs[v];
                        BFXTTxOut&  bfxttxOutComp = decreditMap[inComp];
                        for (int k = (v == u ? i : 0); k < (int)bfxttxOutComp.tokenCount(); k++) {
                            // if the adjacent token id is not the same, break and move on
                            if (bfxttxOut.getToken(i).getTokenId() !=
                                bfxttxOutComp.getToken(k).getTokenId()) {
                                stop = true;
                                break;
                            }
                            // the token slot that the recipient will take from for aggregation
                            BFXTTokenTxData& tokenComp = bfxttxOutComp.getToken(k);
                            if (recps[j].amount >= tokenComp.getAmount()) {
                                // the token amount required by the recipient is larger than the
                                // amount in the token slot, hence the amount in the slot is set to
                                // zero

                                recps[j].amount -= tokenComp.getAmount();
                                ti.amount += tokenComp.getAmount();
                                tokenComp.setAmount(0);
                            } else {
                                // the token amount required by the recipient is smaller than the
                                // amount in the token slot, hence the recipient is set to zero

                                tokenComp.setAmount(tokenComp.getAmount() - recps[j].amount);
                                ti.amount += recps[j].amount;
                                recps[j].amount = 0;

                                // recipient amount is fulfilled. Break and move on
                                stop = true;
                                break;
                            }
                        }
                        if (stop) {
                            break;
                        }
                    }

                    // add that this input will go to recipient j
                    ti.outputIndex = j;
                    ti.skipInput   = false;

                    if (ti.amount > 0) {
                        iti.TIs.push_back(ti);
                    }
                }
            }

            // after having gone through all recipients and given them all their amounts of the token
            // "in", now we see if there's more to be added to change
            if (token.getAmount() > 0) {
                BFXTScript::TransferInstruction ti;

                // Aggregate ajacent change tokens. Aggregate from possible adjacent tokens!
                bool stop = false;
                for (int v = u; v < (int)tokenSourceInputs.size(); v++) {
                    // "inComp" is guaranteed to be in the map because it comes from
                    // tokenSourceInputs
                    const auto& inComp        = tokenSourceInputs[v];
                    BFXTTxOut&  bfxttxOutComp = decreditMap[inComp];
                    for (int k = (v == u ? i : 0); k < (int)bfxttxOutComp.tokenCount(); k++) {
                        // if the adjacent token id is not the same, break and move on
                        if (bfxttxOut.getToken(i).getTokenId() !=
                            bfxttxOutComp.getToken(k).getTokenId()) {
                            stop = true;
                            break;
                        }
                        // the token slot that the recipient will take from for aggregation
                        BFXTTokenTxData& tokenComp = bfxttxOutComp.getToken(k);
                        ti.amount += tokenComp.getAmount();

                        // add change to total change
                        const std::string tokenId = bfxttxOut.getToken(i).getTokenId();
                        if (totalChangeTokens.find(tokenId) == totalChangeTokens.end()) {
                            totalChangeTokens[tokenId] = 0;
                        }
                        totalChangeTokens[tokenId] += tokenComp.getAmount();

                        tokenComp.setAmount(0);
                    }
                    if (stop) {
                        break;
                    }
                }

                // add that this input will go to recipient j
                ti.outputIndex = IntermediaryTI::CHANGE_OUTPUT_FAKE_INDEX;
                ti.skipInput   = false;
                iti.TIs.push_back(ti);
            }
        }

        // ITIs can have zero TIs, because they carry important input information still
        intermediaryTIs.push_back(iti);
    }

    // make sure that all recipients have received their tokens
    for (const auto r : recps) {
        // we don't select nebls
        if (r.tokenId == BFXTSendTxData::NEBL_TOKEN_ID) {
            continue;
        }
        // we ignore tokens to issue, those are to be minted
        if (r.tokenId == BFXTSendTxData::TO_ISSUE_TOKEN_ID) {
            continue;
        }
        if (r.amount != 0) {
            throw std::runtime_error("The recipient " + r.destination + "; of token: " + r.tokenId +
                                     "; still has an unfulfilled amount: " + ::ToString(r.amount) +
                                     ". This should've been spotted earlier.");
        }
    }

    // remove empty elements from change
    for (auto it = totalChangeTokens.begin(); it != totalChangeTokens.end();) {
        if (it->second == 0)
            it = totalChangeTokens.erase(it);
        else
            ++it;
    }

    usedWallet = wallet;

    ready = true;
}

std::map<std::string, BFXTInt> BFXTSendTxData::getTotalTokensInInputs() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get total tokens in inputs");
    return totalTokenAmountsInSelectedInputs;
}

bool BFXTSendTxData::isReady() const { return ready; }

std::vector<BFXTSendTokensOneRecipientData> BFXTSendTxData::getBFXTTokenRecipientsList() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get the recipients list");
    return recipientsList;
}

boost::shared_ptr<BFXTWallet> BFXTSendTxData::getWallet() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get the wallet used in calculations");
    return usedWallet;
}

std::vector<IntermediaryTI> BFXTSendTxData::getIntermediaryTIs() const { return intermediaryTIs; }

int64_t BFXTSendTxData::__addInputsThatCoversNeblAmount(uint64_t neblAmount)
{

    // get nebls that fulfill the fee (if required)

    uint64_t currentTotalNeblsInSelectedInputs = CalculateTotalNeblsInInputs(tokenSourceInputs);

    // check if the total amount in selected addresses is sufficient for the amount
    if (neblAmount > currentTotalNeblsInSelectedInputs) {
        std::vector<COutput> availableOutputs;
        pwalletMain->AvailableCoins(availableOutputs);

        {
            std::random_device rd;
            std::mt19937       g(rd());
            // shuffle outputs to select randomly
            std::shuffle(availableOutputs.begin(), availableOutputs.end(), g);
        }

        // add more outputs
        for (const auto& output : availableOutputs) {
            BFXTOutPoint outPoint(output.tx->GetHash(), output.i);
            // skip if already in
            if (std::find(tokenSourceInputs.begin(), tokenSourceInputs.end(), outPoint) !=
                tokenSourceInputs.end()) {
                continue;
            }
            tokenSourceInputs.push_back(outPoint);
            currentTotalNeblsInSelectedInputs = CalculateTotalNeblsInInputs(tokenSourceInputs);
            if (currentTotalNeblsInSelectedInputs >= neblAmount) {
                break;
            }
        }
    }

    return currentTotalNeblsInSelectedInputs;
}

bool BFXTSendTxData::hasBFXTTokens() const
{
    uint64_t total =
        std::accumulate(intermediaryTIs.begin(), intermediaryTIs.end(), 0,
                        [](uint64_t curr, const IntermediaryTI& iti) { return curr + iti.TIs.size(); });

    return (total != 0);
}

uint64_t BFXTSendTxData::getRequiredNeblsForOutputs() const
{
    if (!ready)
        throw std::runtime_error("BFXTSendTxData not ready; cannot get required fees");
    if (intermediaryTIs.size() > 0) {
        int64_t issuanceFee = (tokenToIssueData.is_initialized() ? BFXTTransaction::IssuanceFee : 0);
        int64_t changeCount = (this->getChangeTokens().size() > 0 ? 1 : 0);

        // + 1 is for OP_RETURN output
        return MIN_TX_FEE * (recipientsList.size() + 1 + changeCount) + issuanceFee;
    } else {
        return 0;
    }
}

int64_t BFXTSendTxData::EstimateTxSizeInBytes(int64_t num_of_inputs, int64_t num_of_outputs)
{
    return num_of_inputs * 181 + num_of_outputs * 34 + 10;
}

int64_t BFXTSendTxData::EstimateTxFee(int64_t num_of_inputs, int64_t num_of_outputs)
{
    double Fee = static_cast<double>(MIN_TX_FEE) *
                 (static_cast<double>(EstimateTxSizeInBytes(num_of_inputs, num_of_outputs)) / 1000.);
    // nearest 10000
    return static_cast<int64_t>(std::ceil(Fee / 10000) * 10000);
}

void BFXTSendTxData::FixTIsChangeOutputIndex(std::vector<BFXTScript::TransferInstruction>& TIs,
                                             int changeOutputIndex)
{
    for (auto& ti : TIs) {
        if (ti.outputIndex == IntermediaryTI::CHANGE_OUTPUT_FAKE_INDEX) {
            ti.outputIndex = changeOutputIndex;
        }
    }
}
