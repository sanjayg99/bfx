#include "bfxtoutpoint.h"
#include "bfxttools.h"

BFXTOutPoint::BFXTOutPoint() { setNull(); }

BFXTOutPoint::BFXTOutPoint(const uint256& hashIn, unsigned int indexIn)
{
    hash    = hashIn;
    index   = indexIn;
    hashStr = hashIn.ToString();
}

void BFXTOutPoint::setNull()
{
    hash    = 0;
    index   = (unsigned int)-1;
    hashStr = "";
}

bool BFXTOutPoint::isNull() const { return (hash == 0 && index == (unsigned int)-1); }

uint256 BFXTOutPoint::getHash() const { return hash; }

unsigned int BFXTOutPoint::getIndex() const { return index; }

json_spirit::Value BFXTOutPoint::exportDatabaseJsonData() const
{
    json_spirit::Object root;

    root.push_back(json_spirit::Pair("hash", hash.ToString()));
    root.push_back(json_spirit::Pair("index", uint64_t(index)));

    return json_spirit::Value(root);
}

void BFXTOutPoint::importDatabaseJsonData(const json_spirit::Value& data)
{
    setNull();

    index = BFXTTools::GetUint64Field(data.get_obj(), "index");
    hash.SetHex(BFXTTools::GetStrField(data.get_obj(), "hash"));
}
