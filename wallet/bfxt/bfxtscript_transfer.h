#ifndef BFXTSCRIPT_TRANSFER_H
#define BFXTSCRIPT_TRANSFER_H

#include "bfxtscript.h"

class BFXTScript_Transfer : public BFXTScript
{
    std::string metadata;

protected:
    std::vector<TransferInstruction> transferInstructions;

public:
    BFXTScript_Transfer();

    std::string                      getHexMetadata() const override;
    std::string                      getRawMetadata() const override;
    std::string                      getInflatedMetadata() const override;
    unsigned                         getTransferInstructionsCount() const;
    TransferInstruction              getTransferInstruction(unsigned index) const;
    std::vector<TransferInstruction> getTransferInstructions() const;

    static std::shared_ptr<BFXTScript_Transfer> ParseTransferPostHeaderData(std::string ScriptBin,
                                                                            std::string OpCodeBin);
    static std::shared_ptr<BFXTScript_Transfer> ParseBFXTv3TransferPostHeaderData(std::string ScriptBin);
    static std::string                          Create_OpCodeFromMetadata(const std::string& metadata);
    static std::shared_ptr<BFXTScript_Transfer>
    CreateScript(const std::vector<BFXTScript::TransferInstruction>& transferInstructions,
                 const std::string&                                  Metadata);

    // BFXTScript interface
public:
    std::string            calculateScriptBin() const override;
    std::set<unsigned int> getBFXTOutputIndices() const override;
};

#endif // BFXTSCRIPT_TRANSFER_H
