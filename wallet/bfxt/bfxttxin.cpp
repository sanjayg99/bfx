#include "bfxttxin.h"

void BFXTTxIn::setPrevout(const BFXTOutPoint& value) { prevout = value; }

BFXTOutPoint BFXTTxIn::getPrevout() const { return prevout; }

void BFXTTxIn::setSequence(const uint64_t& value) { nSequence = value; }

BFXTTxIn::BFXTTxIn() { nSequence = std::numeric_limits<unsigned int>::max(); }

void BFXTTxIn::setNull()
{
    prevout.setNull();
    scriptSigHex.clear();
    nSequence = 0;
    tokens.clear();
}

void BFXTTxIn::importJsonData(const json_spirit::Value& parsedData)
{
    try {
        std::string txid_str = BFXTTools::GetStrField(parsedData.get_obj(), "txid");
        uint256     txid;
        txid.SetHex(txid_str);
        nSequence                 = BFXTTools::GetUint64Field(parsedData.get_obj(), "sequence");
        unsigned int        index = BFXTTools::GetUint64Field(parsedData.get_obj(), "vout");
        json_spirit::Object scriptSigJsonObj =
            BFXTTools::GetObjectField(parsedData.get_obj(), "scriptSig");
        prevout      = BFXTOutPoint(txid, index);
        scriptSigHex = BFXTTools::GetStrField(scriptSigJsonObj, "hex");
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

json_spirit::Value BFXTTxIn::exportDatabaseJsonData() const
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("prevout", prevout.exportDatabaseJsonData()));
    root.push_back(json_spirit::Pair("scriptSig", scriptSigHex));
    root.push_back(json_spirit::Pair("sequence", nSequence));
    json_spirit::Array tokensArray;

    for (long i = 0; i < static_cast<long>(tokens.size()); i++) {
        tokensArray.push_back(tokens[i].exportDatabaseJsonData());
    }
    root.push_back(json_spirit::Pair("tokens", json_spirit::Value(tokensArray)));

    return json_spirit::Value(root);
}

void BFXTTxIn::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    prevout.importDatabaseJsonData(BFXTTools::GetObjectField(data.get_obj(), "prevout"));
    scriptSigHex                   = BFXTTools::GetStrField(data.get_obj(), "scriptSig");
    nSequence                      = BFXTTools::GetUint64Field(data.get_obj(), "sequence");
    json_spirit::Array tokens_list = BFXTTools::GetArrayField(data.get_obj(), "tokens");
    tokens.clear();
    tokens.resize(tokens_list.size());
    for (unsigned long i = 0; i < tokens_list.size(); i++) {
        tokens[i].importDatabaseJsonData(tokens_list[i]);
    }
}
void BFXTTxIn::importJsonData(const std::string& data)
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

BFXTOutPoint BFXTTxIn::getOutPoint() const { return prevout; }

std::string BFXTTxIn::getScriptSigHex() const { return scriptSigHex; }

void BFXTTxIn::setScriptSigHex(const std::string& s) { scriptSigHex = s; }

uint64_t BFXTTxIn::getSequence() const { return nSequence; }

const BFXTTokenTxData& BFXTTxIn::getToken(unsigned long index) const { return tokens[index]; }

unsigned long BFXTTxIn::getNumOfTokens() const { return tokens.size(); }

void BFXTTxIn::__addToken(const BFXTTokenTxData& token) { tokens.push_back(token); }
