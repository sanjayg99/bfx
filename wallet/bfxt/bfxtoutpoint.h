#ifndef BFXTOUTPOINT_H
#define BFXTOUTPOINT_H

#include <string>

#include "uint256.h"
#include "util.h"

#include <boost/functional/hash.hpp>

#include "json/json_spirit.h"

class BFXTOutPoint
{
    uint256      hash;
    unsigned int index;

    std::string hashStr; // for debugging

public:
    BFXTOutPoint();
    BFXTOutPoint(const uint256& hashIn, unsigned int indexIn);
    void               setNull();
    bool               isNull() const;
    uint256            getHash() const;
    unsigned int       getIndex() const;
    friend inline bool operator==(const BFXTOutPoint& lhs, const BFXTOutPoint& rhs);
    json_spirit::Value exportDatabaseJsonData() const;
    void               importDatabaseJsonData(const json_spirit::Value& data);

    // clang-format off
    IMPLEMENT_SERIALIZE(
                        READWRITE(hash);
                        READWRITE(index);
                       )
    // clang-format on
};

namespace std {

template <>
struct hash<BFXTOutPoint>
{
    std::size_t operator()(const BFXTOutPoint& k) const
    {
        std::string toHash = k.getHash().ToString() + ":" + ToString(k.getIndex());
        return boost::hash<std::string>()(toHash);
    }
};

} // namespace std

bool operator==(const BFXTOutPoint& lhs, const BFXTOutPoint& rhs)
{
    return (lhs.hash == rhs.hash && lhs.index == rhs.index);
}

#endif // BFXTOUTPOINT_H
