#include "bfxtscript_transfer.h"

#include <boost/algorithm/hex.hpp>

#include "init.h"
#include "main.h"
#include "util.h"

BFXTScript_Transfer::BFXTScript_Transfer() {}

std::string BFXTScript_Transfer::getHexMetadata() const { return boost::algorithm::hex(metadata); }

std::string BFXTScript_Transfer::getRawMetadata() const { return metadata; }

std::string BFXTScript_Transfer::getInflatedMetadata() const
{
    if (!metadata.empty())
        return ZlibDecompress(getRawMetadata());
    else
        return "";
}

unsigned BFXTScript_Transfer::getTransferInstructionsCount() const
{
    return transferInstructions.size();
}

BFXTScript_Transfer::TransferInstruction
BFXTScript_Transfer::getTransferInstruction(unsigned index) const
{
    return transferInstructions[index];
}

std::vector<BFXTScript::TransferInstruction> BFXTScript_Transfer::getTransferInstructions() const
{
    return transferInstructions;
}

std::shared_ptr<BFXTScript_Transfer>
BFXTScript_Transfer::ParseTransferPostHeaderData(std::string ScriptBin, std::string OpCodeBin)
{
    std::shared_ptr<BFXTScript_Transfer> result = std::make_shared<BFXTScript_Transfer>();

    // get metadata then drop it
    result->metadata = ParseMetadataFromLongEnoughString(ScriptBin, OpCodeBin);
    ScriptBin.erase(ScriptBin.begin(), ScriptBin.begin() + result->metadata.size());

    // parse transfer instructions
    int totalTransferInstructionsSize = 0;
    result->transferInstructions =
        ParseTransferInstructionsFromLongEnoughString(ScriptBin, totalTransferInstructionsSize);
    ScriptBin.erase(ScriptBin.begin(), ScriptBin.begin() + totalTransferInstructionsSize);

    return result;
}

std::shared_ptr<BFXTScript_Transfer>
BFXTScript_Transfer::ParseBFXTv3TransferPostHeaderData(std::string ScriptBin)
{
    std::shared_ptr<BFXTScript_Transfer> result = std::make_shared<BFXTScript_Transfer>();

    // parse transfer instructions
    int totalTransferInstructionsSize = 0;
    result->transferInstructions =
        ParseBFXTv3TransferInstructionsFromLongEnoughString(ScriptBin, totalTransferInstructionsSize);
    ScriptBin.erase(ScriptBin.begin(), ScriptBin.begin() + totalTransferInstructionsSize);

    result->metadata = ParseBFXTv3MetadataFromLongEnoughString(ScriptBin);
    if (result->metadata.size() > 0) {
        ScriptBin.erase(ScriptBin.begin(),
                        ScriptBin.begin() + result->metadata.size() + 4); // + 4 for size
    }

    if (ScriptBin.size() != 0) {
        throw std::runtime_error("Garbage data after the metadata (unaccounted for in size).");
    }

    return result;
}

std::string BFXTScript_Transfer::calculateScriptBin() const
{
    if (protocolVersion == 1) {
        std::string result;
        result += headerBin;
        result += opCodeBin;
        result += metadata;

        for (const auto& ti : transferInstructions) {
            result += TransferInstructionToBinScript(ti);
        }

        return result;
    } else if (protocolVersion == 3) {
        std::string result;
        result += headerBin;
        result += opCodeBin;

        if (transferInstructions.size() > 0xff) {
            throw std::runtime_error(
                "The number of transfer instructions exceeded the allowed maximum, 255");
        }

        unsigned char TIsSize = static_cast<unsigned char>(transferInstructions.size());
        result.push_back(static_cast<char>(TIsSize));
        for (const auto& ti : transferInstructions) {
            result += TransferInstructionToBinScript(ti);
        }

        if (metadata.size() > 0) {
            uint32_t metadataSize = metadata.size();

            MakeBigEndian(metadataSize);

            std::string metadataSizeStr;
            metadataSizeStr.resize(4);
            static_assert(sizeof(metadataSize) == 4,
                          "Metadata size must be 4 bytes accoring to BFXTv3 standard");
            memcpy(&metadataSizeStr.front(), &metadataSize, 4);

            result += metadataSizeStr;
            result += metadata;
        }

        if (isOpReturnSizeCheckEnabled() && result.size() > DataSize()) {
            throw std::runtime_error("Calculated script size (" + std::to_string(result.size()) +
                                     " bytes) is larger than the maximum allowed (" +
                                     std::to_string(DataSize()) + " bytes)");
        }

        return result;
    } else {
        throw std::runtime_error(
            "While calculating/constructing BFXTScript, an unknown protocol version was found");
    }
}

std::set<unsigned int> BFXTScript_Transfer::getBFXTOutputIndices() const
{
    std::set<unsigned int> result;
    for (const auto& ti : transferInstructions) {
        result.insert(ti.outputIndex);
    }
    return result;
}

std::shared_ptr<BFXTScript_Transfer> BFXTScript_Transfer::CreateScript(
    const std::vector<BFXTScript::TransferInstruction>& transferInstructions,
    const std::string&                                  Metadata)
{
    std::shared_ptr<BFXTScript_Transfer> script = std::make_shared<BFXTScript_Transfer>();

    script->protocolVersion      = 3;
    script->headerBin            = boost::algorithm::unhex(std::string("4e5403"));
    script->metadata             = (Metadata.empty() ? "" : ZlibCompress(Metadata));
    script->opCodeBin            = boost::algorithm::unhex(std::string("10"));
    script->transferInstructions = transferInstructions;
    script->txType               = TxType::TxType_Transfer;

    return script;
}

std::string BFXTScript_Transfer::Create_OpCodeFromMetadata(const std::string& metadata)
{
    const auto& sz = metadata.size();
    std::string result;
    if (sz == 0) {
        return std::string(1, static_cast<char>(uint8_t(0x15)));
    } else if (sz == 20) {
        return std::string(1, static_cast<char>(uint8_t(0x11)));
    } else if (sz == 52) {
        return std::string(1, static_cast<char>(uint8_t(0x10)));
    } else {
        throw std::runtime_error("Invalid metadata size; can only be 0, 20 or 52");
    }
    return boost::algorithm::unhex(result);
}
