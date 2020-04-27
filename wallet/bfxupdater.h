#ifndef BFXUPDATER_H
#define BFXUPDATER_H

#include <string>
#include <boost/thread/future.hpp>

#include "version.h"
#include "clientversion.h"
#include "bfxversion.h"
#include "bfxreleaseinfo.h"

#include "curltools.h"

class BFXUpdater
{

public:
    static const std::string ClientVersionSrcFileLink;
    static const std::string ReleasesInfoURL;
    static const std::string LatestReleaseURL;

    BFXUpdater() = default;
    void checkIfUpdateIsAvailable(boost::promise<bool> &updateIsAvailablePromise, BFXReleaseInfo &lastRelease);

    static BFXVersion ParseVersion(const std::string& versionFile);
    static std::string GetDefineFromCFile(const std::string& fileData, const std::string &fieldName);
    static std::string RemoveCFileComments(const std::string& fileData);
};

struct RemovePreReleaseFunctor
{
    bool operator() (const BFXReleaseInfo& r)
    {
        return r.getIsPreRelease();
    }
};

struct BFXReleaseVersionGreaterComparator
{
    bool operator() (const BFXReleaseInfo& r1, const BFXReleaseInfo& r2)
    {
        return r1.getVersion() > r2.getVersion();
    }
};


#endif // BFXUPDATER_H
