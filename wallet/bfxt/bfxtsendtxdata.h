#ifndef BFXTSENDTXDATA_H
#define BFXTSENDTXDATA_H

#include "globals.h"
#include "bfxt/bfxtwallet.h"
#include "bfxtsendtokensonerecipientdata.h"
#include <deque>
#include <string>
#include <unordered_set>

struct IntermediaryTI
{
    static const unsigned int CHANGE_OUTPUT_FAKE_INDEX = 10000000;

    std::vector<BFXTScript::TransferInstruction> TIs;
    BFXTOutPoint                                 input;

    bool isBFXTTokenIssuance = false;
};

struct IssueTokenData
{
    IssueTokenData(BFXTInt Amount, std::string Symbol, std::string Metadata)
        : amount(std::move(Amount)), symbol(std::move(Symbol)), metadata(std::move(Metadata))
    {
    }
    BFXTInt     amount;
    std::string symbol;
    std::string metadata;
};

/**
 * This class locates available NPT1 tokens in an BFXTWallet object and either finds the required amount
 * of tokens in the wallet or simply ensures that the required amounts exist in the wallet
 *
 * @brief The BFXTSendTxData class
 */
class BFXTSendTxData
{
    // this is a vector because order must be preserved
    std::vector<BFXTOutPoint>                   tokenSourceInputs;
    std::vector<IntermediaryTI>                 intermediaryTIs;
    std::map<std::string, BFXTInt>              totalChangeTokens;
    std::map<std::string, BFXTInt>              totalTokenAmountsInSelectedInputs;
    std::vector<BFXTSendTokensOneRecipientData> recipientsList;
    boost::shared_ptr<BFXTWallet>               usedWallet;

    int64_t __addInputsThatCoversNeblAmount(uint64_t neblAmount);
    bool    ready = false;

    boost::optional<IssueTokenData> tokenToIssueData;

public:
    BFXTSendTxData();
    /**
     * @brief calculateSources
     * @param wallet
     * @param inputs: inputs to be used; if no inputs provided, everything in the wallet will be used
     * @param recipients
     * @param recalculateFee
     * @param neblAmount amount to be sent with the transaction
     */
    void selectBFXTTokens(boost::shared_ptr<BFXTWallet> wallet, std::vector<BFXTOutPoint> inputs,
                          std::vector<BFXTSendTokensOneRecipientData> recipients,
                          bool                                        addMoreInputsIfRequired);
    void selectBFXTTokens(boost::shared_ptr<BFXTWallet> wallet, const std::vector<COutPoint>& inputs,
                          const std::vector<BFXTSendTokensOneRecipientData>& recipients,
                          bool                                               addMoreInputsIfRequired);

    void                            issueBFXTToken(const IssueTokenData& data);
    boost::optional<IssueTokenData> getBFXTTokenIssuanceData() const;
    bool                            getWhetherIssuanceExists() const;

    static const std::string NEBL_TOKEN_ID;
    static const std::string TO_ISSUE_TOKEN_ID;

    // returns the total balance in the selected addresses (tokenSourceAddresses)
    static int64_t EstimateTxSizeInBytes(int64_t num_of_inputs, int64_t num_of_outputs);
    static int64_t EstimateTxFee(int64_t num_of_inputs, int64_t num_of_outputs);

    void
    verifyBFXTIssuanceRecipientsValidity(const std::vector<BFXTSendTokensOneRecipientData>& recipients);

    std::vector<BFXTOutPoint>      getUsedInputs() const;
    std::map<std::string, BFXTInt> getChangeTokens() const;
    std::map<std::string, BFXTInt> getTotalTokensInInputs() const;
    bool                           isReady() const;
    // list of recipients after removing Nebl recipients
    std::vector<BFXTSendTokensOneRecipientData> getBFXTTokenRecipientsList() const;
    boost::shared_ptr<BFXTWallet>               getWallet() const;

    std::vector<IntermediaryTI> getIntermediaryTIs() const;
    /**
     * @brief hasBFXTTokens
     * @return true if the resulting inputs have BFXT tokens
     */
    bool     hasBFXTTokens() const;
    uint64_t getRequiredNeblsForOutputs() const;

    static void FixTIsChangeOutputIndex(std::vector<BFXTScript::TransferInstruction>& TIs,
                                        int                                           changeOutputIndex);
};

#endif // BFXTSENDTXDATA_H
