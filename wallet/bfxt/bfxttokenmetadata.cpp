#include "bfxttokenmetadata.h"

#include "bfxttools.h"

#include "bfxtscript_issuance.h"

BFXTTokenMetaData::BFXTTokenMetaData() { setNull(); }

void BFXTTokenMetaData::setTokenName(const std::string& value) { tokenName = value; }

void BFXTTokenMetaData::setNull()
{
    totalSupply = 0;
    tokenName.clear();
    tokenDescription.clear();
    tokenIssuer.clear();
    iconURL.clear();
    iconImageType.clear();
}

bool BFXTTokenMetaData::isNull() const { return getTokenId().size() == 0; }

void BFXTTokenMetaData::importRestfulAPIJsonData(const std::string& data)
{
    try {
        json_spirit::Value parsedData;
        json_spirit::read_or_throw(data, parsedData);
        importRestfulAPIJsonData(parsedData);
    } catch (std::exception& ex) {
        printf("%s\n", ex.what());
        throw;
    }
}

void BFXTTokenMetaData::importRestfulAPIJsonData(const json_spirit::Value& data)
{
    try {
        setTokenId(BFXTTools::GetStrField(data.get_obj(), "tokenId"));
        setIssuanceTxIdHex(BFXTTools::GetStrField(data.get_obj(), "issuanceTxid"));
        divisibility      = BFXTTools::GetUint64Field(data.get_obj(), "divisibility");
        lockStatus        = BFXTTools::GetBoolField(data.get_obj(), "lockStatus");
        aggregationPolicy = BFXTTools::GetStrField(data.get_obj(), "aggregationPolicy");
        totalSupply       = BFXTTools::GetUint64Field(data.get_obj(), "totalSupply");
        // fields inside metadata
        json_spirit::Object metadata = BFXTTools::GetObjectField(data.get_obj(), "metadataOfIssuence");
        // data inside metadata
        json_spirit::Object innerdata = BFXTTools::GetObjectField(metadata, "data");
        tokenName                     = BFXTTools::GetStrField(innerdata, "tokenName");
        tokenDescription              = BFXTTools::GetStrField(innerdata, "description");
        tokenIssuer                   = BFXTTools::GetStrField(innerdata, "issuer");
        try {
            json_spirit::Array urlsArray = BFXTTools::GetArrayField(innerdata, "urls");
            urls = json_spirit::Value(BFXTTools::GetArrayField(innerdata, "urls"));
            for (long i = 0; i < static_cast<long>(urlsArray.size()); i++) {
                std::string urlName = BFXTTools::GetStrField(urlsArray[i].get_obj(), "name");
                if (urlName == "icon") {
                    iconImageType = BFXTTools::GetStrField(urlsArray[i].get_obj(), "mimeType");
                    iconURL       = BFXTTools::GetStrField(urlsArray[i].get_obj(), "url");
                }
            }
            userData = BFXTTools::GetObjectField(innerdata, "userData");
        } catch (...) {
        }
    } catch (std::exception& ex) {
        printf("%s\n", ex.what());
        throw;
    }
}

json_spirit::Value BFXTTokenMetaData::exportDatabaseJsonData(bool for_rpc) const
{
    json_spirit::Object root;

    if (!for_rpc) {
        root.push_back(json_spirit::Pair("userData", userData));
        root.push_back(json_spirit::Pair("urls", urls));
        root.push_back(json_spirit::Pair("tokenId", getTokenId()));
        root.push_back(json_spirit::Pair("issuanceTxid", getIssuanceTxIdHex()));
        root.push_back(json_spirit::Pair("divisibility", divisibility));
        root.push_back(json_spirit::Pair("lockStatus", static_cast<bool>(lockStatus)));
        root.push_back(json_spirit::Pair("aggregationPolicy", aggregationPolicy));
        root.push_back(json_spirit::Pair("totalSupply", totalSupply.str()));
        root.push_back(json_spirit::Pair("tokenName", tokenName));
        root.push_back(json_spirit::Pair("tokenDescription", tokenDescription));
        root.push_back(json_spirit::Pair("tokenIssuer", tokenIssuer));
        root.push_back(json_spirit::Pair("iconURL", iconURL));
        root.push_back(json_spirit::Pair("iconImageType", iconImageType));

        return json_spirit::Value(root);
    } else {
        root.push_back(json_spirit::Pair("tokenName", tokenName));
        root.push_back(json_spirit::Pair("description", tokenDescription));
        root.push_back(json_spirit::Pair("urls", urls));
        root.push_back(json_spirit::Pair("issuer", tokenIssuer));
        root.push_back(json_spirit::Pair("userData", userData));
        json_spirit::Value  rV(root);
        json_spirit::Object data;
        data.push_back(json_spirit::Pair("data", rV));

        return data;
    }
}

void BFXTTokenMetaData::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    setTokenId(BFXTTools::GetStrField(data.get_obj(), "tokenId"));
    setIssuanceTxIdHex(BFXTTools::GetStrField(data.get_obj(), "issuanceTxid"));
    divisibility      = BFXTTools::GetUint64Field(data.get_obj(), "divisibility");
    lockStatus        = (int)BFXTTools::GetBoolField(data.get_obj(), "lockStatus");
    aggregationPolicy = BFXTTools::GetStrField(data.get_obj(), "aggregationPolicy");
    totalSupply       = FromString<BFXTInt>(BFXTTools::GetStrField(data.get_obj(), "totalSupply"));
    tokenName         = BFXTTools::GetStrField(data.get_obj(), "tokenName");
    tokenDescription  = BFXTTools::GetStrField(data.get_obj(), "tokenDescription");
    tokenIssuer       = BFXTTools::GetStrField(data.get_obj(), "tokenIssuer");
    iconURL           = BFXTTools::GetStrField(data.get_obj(), "iconURL");
    iconImageType     = BFXTTools::GetStrField(data.get_obj(), "iconImageType");
}

void BFXTTokenMetaData::readSomeDataFromStandardJsonFormat(const json_spirit::Value& data)
{
    json_spirit::Object dataObj = BFXTTools::GetObjectField(data.get_obj(), "data");

    this->tokenName        = BFXTTools::GetStrField(dataObj, "tokenName");
    this->tokenDescription = BFXTTools::GetStrField(dataObj, "description");
    this->tokenIssuer      = BFXTTools::GetStrField(dataObj, "issuer");
    try {
        json_spirit::Array urlsArray = BFXTTools::GetArrayField(dataObj, "urls");
        this->urls                   = json_spirit::Value(BFXTTools::GetArrayField(dataObj, "urls"));
        for (long i = 0; i < static_cast<long>(urlsArray.size()); i++) {
            std::string urlName = BFXTTools::GetStrField(urlsArray[i].get_obj(), "name");
            if (urlName == "icon") {
                this->iconImageType = BFXTTools::GetStrField(urlsArray[i].get_obj(), "mimeType");
                this->iconURL       = BFXTTools::GetStrField(urlsArray[i].get_obj(), "url");
                break;
            }
        }
        this->userData = BFXTTools::GetObjectField(dataObj, "userData");
    } catch (...) {
    }
}

void BFXTTokenMetaData::readSomeDataFromBFXTIssuanceScript(BFXTScript_Issuance* sd)
{
    this->totalSupply       = sd->getAmount();
    this->tokenName         = sd->getTokenSymbol();
    this->divisibility      = sd->getDivisibility();
    this->aggregationPolicy = sd->getAggregationPolicyStr();
    this->lockStatus        = sd->isLocked();
}

BFXTInt BFXTTokenMetaData::getTotalSupply() const { return totalSupply; }

const std::string& BFXTTokenMetaData::getTokenName() const { return tokenName; }

const std::string& BFXTTokenMetaData::getTokenDescription() const { return tokenDescription; }

const std::string& BFXTTokenMetaData::getTokenIssuer() const { return tokenIssuer; }

const std::string& BFXTTokenMetaData::getIconURL() const { return iconURL; }

const std::string& BFXTTokenMetaData::getIconImageType() const { return iconImageType; }
