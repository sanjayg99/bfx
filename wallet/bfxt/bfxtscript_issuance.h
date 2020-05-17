#ifndef BFXTSCRIPT_ISSUANCE_H
#define BFXTSCRIPT_ISSUANCE_H

#include "bfxtscript.h"

class BFXTScript_Issuance : public BFXTScript
{
    std::string   tokenSymbol;
    std::string   metadata;
    BFXTInt       amount;
    IssuanceFlags issuanceFlags;

    friend class BFXTScript;

    std::string __getAggregAndLockStatusTokenIDHexValue() const;

protected:
    std::vector<TransferInstruction> transferInstructions;

public:
    BFXTScript_Issuance();

    std::string getHexMetadata() const override;
    std::string getRawMetadata() const override;
    std::string getInflatedMetadata() const override;

    int                                         getDivisibility() const;
    bool                                        isLocked() const;
    IssuanceFlags::AggregationPolicy            getAggregationPolicy() const;
    std::string                                 getAggregationPolicyStr() const;
    std::string                                 getTokenSymbol() const;
    BFXTInt                                     getAmount() const;
    unsigned                                    getTransferInstructionsCount() const;
    TransferInstruction                         getTransferInstruction(unsigned index) const;
    std::vector<TransferInstruction>            getTransferInstructions() const;
    static std::shared_ptr<BFXTScript_Issuance> ParseIssuancePostHeaderData(std::string ScriptBin,
                                                                            std::string OpCodeBin);
    static std::shared_ptr<BFXTScript_Issuance> ParseBFXTv3IssuancePostHeaderData(std::string ScriptBin);
    std::string getTokenID(std::string input0txid, unsigned int input0index) const;

    static std::shared_ptr<BFXTScript_Issuance>
                       CreateScript(const std::string& Symbol, BFXTInt amount,
                                    const std::vector<TransferInstruction>& transferInstructions,
                                    const std::string& Metadata, bool locked, unsigned int divisibility,
                                    IssuanceFlags::AggregationPolicy aggrPolicy);
    static std::string Create_OpCodeFromMetadata(const std::string& metadata);
    static std::string Create_ProcessTokenSymbol(const std::string& symbol);

    // BFXTScript interface
public:
    std::string            calculateScriptBin() const override;
    std::set<unsigned int> getBFXTOutputIndices() const override;
};

#endif // BFXTSCRIPT_ISSUANCE_H
