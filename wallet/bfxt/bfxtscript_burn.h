#ifndef BFXTSCRIPT_BURN_H
#define BFXTSCRIPT_BURN_H

#include "bfxtscript.h"

class BFXTScript_Burn : public BFXTScript
{
    std::string metadata;

protected:
    std::vector<TransferInstruction> transferInstructions;

public:
    BFXTScript_Burn();

    std::string                      getHexMetadata() const override;
    std::string                      getRawMetadata() const override;
    std::string                      getInflatedMetadata() const override;
    unsigned                         getTransferInstructionsCount() const;
    TransferInstruction              getTransferInstruction(unsigned index) const;
    std::vector<TransferInstruction> getTransferInstructions() const;

    static std::shared_ptr<BFXTScript_Burn> ParseBurnPostHeaderData(std::string ScriptBin,
                                                                    std::string OpCodeBin);
    static std::shared_ptr<BFXTScript_Burn> ParseBFXTv3BurnPostHeaderData(std::string ScriptBin);
    static std::string                      Create_OpCodeFromMetadata(const std::string& metadata);
    static std::shared_ptr<BFXTScript_Burn>
    CreateScript(const std::vector<BFXTScript::TransferInstruction>& transferInstructions,
                 const std::string&                                  Metadata);

    // BFXTScript interface
public:
    std::string            calculateScriptBin() const override;
    std::set<unsigned int> getBFXTOutputIndices() const override;
};

#endif // BFXTSCRIPT_BURN_H
