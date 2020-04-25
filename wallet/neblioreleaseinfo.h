#ifndef BFXRELEASEINFO_H
#define BFXRELEASEINFO_H

#include "json_spirit.h"
#include "bfxversion.h"

#include <string>

class BFXReleaseInfo
{
    std::string versionStr;
    BFXVersion version;
    std::string htmlURL;
    std::string bodyText;
    bool isPreRelease;

    static std::string GetStrField(const json_spirit::Object& data, const std::string& fieldName);
    static bool GetBoolField(const json_spirit::Object &data, const std::string &fieldName);
    static BFXVersion VersionTagStrToObj(std::string VersionStr);
    static BFXReleaseInfo ParseSingleReleaseData(const json_spirit::Object& data);

public:
    BFXReleaseInfo();

    static std::vector<BFXReleaseInfo> ParseAllReleaseDataFromJSON(const std::string& data);
    bool getIsPreRelease() const;
    BFXVersion getVersion() const;
    std::string getUpdateDescription() const;
    std::string getDownloadLink() const;
    void clear();
};


#endif // BFXRELEASEINFO_H
