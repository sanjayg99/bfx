#include "bfxttxout.h"
#include "bfxttools.h"

std::string BFXTTxOut::getAddress() const { return address; }

void BFXTTxOut::setAddress(const std::string& Address) { address = Address; }

typename BFXTTxOut::OutputType BFXTTxOut::getType() const
{
    BFXTTxOut::OutputType type;
    if (scriptPubKeyAsm.empty()) {
        type = OutputType::NonStandard;
    } else if (scriptPubKeyAsm.find("OP_RETURN") != std::string::npos) {
        type = OutputType::OPReturn;
    } else {
        type = OutputType::NormalOutput;
    }

    return type;
}

std::string BFXTTxOut::getScriptPubKeyAsm() const { return scriptPubKeyAsm; }

void BFXTTxOut::__manualSet(int64_t NValue, std::string ScriptPubKeyHex, std::string ScriptPubKeyAsm,
                            std::vector<BFXTTokenTxData> Tokens, std::string Address)
{
    nValue          = NValue;
    scriptPubKeyHex = ScriptPubKeyHex;
    scriptPubKeyAsm = ScriptPubKeyAsm;
    tokens          = Tokens;
    address         = Address;
}

void BFXTTxOut::setNValue(const int64_t& value) { nValue = value; }

void BFXTTxOut::setScriptPubKeyHex(const std::string& value) { scriptPubKeyHex = value; }

void BFXTTxOut::setScriptPubKeyAsm(const std::string& value) { scriptPubKeyAsm = value; }

void BFXTTxOut::__addToken(const BFXTTokenTxData& token) { tokens.push_back(token); }

BFXTTxOut::BFXTTxOut() { setNull(); }

BFXTTxOut::BFXTTxOut(int64_t nValueIn, const std::string& scriptPubKeyIn)
{
    nValue          = nValueIn;
    scriptPubKeyHex = scriptPubKeyIn;
}

void BFXTTxOut::setNull()
{
    nValue = -1;
    scriptPubKeyHex.clear();
    tokens.clear();
}

bool BFXTTxOut::isNull() const { return (nValue == -1); }

void BFXTTxOut::importJsonData(const std::string& data)
{
    try {
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(data, parsedData);
        importJsonData(parsedData);
    } catch (std::exception& ex) {
        printf("%s", ex.what());
        throw;
    }
}

void BFXTTxOut::importJsonData(const json_spirit::Value& parsedData)
{
    try {
        nValue = BFXTTools::GetUint64Field(parsedData.get_obj(), "value");
        json_spirit::Object scriptPubKeyJsonObj =
            BFXTTools::GetObjectField(parsedData.get_obj(), "scriptPubKey");
        scriptPubKeyHex = BFXTTools::GetStrField(scriptPubKeyJsonObj, "hex");
        scriptPubKeyAsm = BFXTTools::GetStrField(scriptPubKeyJsonObj, "asm");
        if (getType() == OutputType::NormalOutput) {
            json_spirit::Array addresses = BFXTTools::GetArrayField(scriptPubKeyJsonObj, "addresses");
            if (addresses.size() != 1) {
                throw std::runtime_error(
                    "Addresses field in scriptPubKey has a size != 1 for normal outputs");
            }
            address = addresses[0].get_str();
        }
        json_spirit::Array tokens_list;
        if (!json_spirit::find_value(parsedData.get_obj(), "tokens").is_null()) {
            tokens_list = BFXTTools::GetArrayField(parsedData.get_obj(), "tokens");
        }
        tokens.clear();
        tokens.resize(tokens_list.size());
        for (unsigned long i = 0; i < tokens_list.size(); i++) {
            tokens[i].importJsonData(tokens_list[i]);
        }
    } catch (std::exception& ex) {
        printf("%s", ex.what());
        throw;
    }
}

json_spirit::Value BFXTTxOut::exportDatabaseJsonData() const
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("value", nValue));
    root.push_back(json_spirit::Pair("scriptPubKey", scriptPubKeyHex));
    root.push_back(json_spirit::Pair("scriptPubKeyAsm", scriptPubKeyAsm));
    root.push_back(json_spirit::Pair("address", address));
    json_spirit::Array tokensArray;
    for (long i = 0; i < static_cast<long>(tokens.size()); i++) {
        tokensArray.push_back(tokens[i].exportDatabaseJsonData());
    }
    root.push_back(json_spirit::Pair("tokens", json_spirit::Value(tokensArray)));

    return json_spirit::Value(root);
}

void BFXTTxOut::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    nValue                         = BFXTTools::GetUint64Field(data.get_obj(), "value");
    scriptPubKeyHex                = BFXTTools::GetStrField(data.get_obj(), "scriptPubKey");
    scriptPubKeyAsm                = BFXTTools::GetStrField(data.get_obj(), "scriptPubKeyAsm");
    address                        = BFXTTools::GetStrField(data.get_obj(), "address");
    json_spirit::Array tokens_list = BFXTTools::GetArrayField(data.get_obj(), "tokens");
    tokens.clear();
    tokens.resize(tokens_list.size());
    for (unsigned long i = 0; i < tokens_list.size(); i++) {
        tokens[i].importDatabaseJsonData(tokens_list[i]);
    }
}

int64_t BFXTTxOut::getValue() const { return nValue; }

const std::string& BFXTTxOut::getScriptPubKeyHex() const { return scriptPubKeyHex; }

const BFXTTokenTxData& BFXTTxOut::getToken(unsigned long index) const { return tokens[index]; }

BFXTTokenTxData& BFXTTxOut::getToken(unsigned long index) { return tokens[index]; }

unsigned long BFXTTxOut::tokenCount() const { return tokens.size(); }
