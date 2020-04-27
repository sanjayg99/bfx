#include "bfxupdater.h"
#include "util.h"

#include <iostream>
#include <vector>
#include <boost/regex.hpp>
#include <sstream>
#include <boost/algorithm/string.hpp>

const std::string BFXUpdater::ClientVersionSrcFileLink  = "https://raw.githubusercontent.com/bfxcrypto/bfx/master/src/clientversion.h";
const std::string BFXUpdater::ReleasesInfoURL = "https://api.github.com/repos/bfxcrypto/bfx/releases";
const std::string BFXUpdater::LatestReleaseURL = "https://github.com/bfxcrypto/bfx/releases/latest";

void BFXUpdater::checkIfUpdateIsAvailable(boost::promise<bool> &updateIsAvailablePromise, BFXReleaseInfo& lastRelease)
{
    BFXReleaseInfo remoteRelease;
    BFXVersion localVersion;
    std::string releaseData;
    std::vector<BFXReleaseInfo> bfxReleases;
    try {
        releaseData = cURLTools::GetFileFromHTTPS(ReleasesInfoURL, 30, 0);
        bfxReleases = BFXReleaseInfo::ParseAllReleaseDataFromJSON(releaseData);

        // remove prerelease versions
        bfxReleases.erase(std::remove_if(bfxReleases.begin(), bfxReleases.end(),
                RemovePreReleaseFunctor()), bfxReleases.end());
//        std::for_each(bfxReleases.begin(), bfxReleases.end(), [](const BFXReleaseInfo& v) {std::cout<<v.versionStr<<std::endl;});
        // sort in descending order
        std::sort(bfxReleases.begin(), bfxReleases.end(), BFXReleaseVersionGreaterComparator());
//        std::for_each(bfxReleases.begin(), bfxReleases.end(), [](const BFXReleaseInfo& v) {std::cout<<v.versionStr<<std::endl;});
        if(bfxReleases.size() <= 0) {
            throw std::length_error("The list of releases retrieved is empty.");
        }
    } catch (std::exception& ex) {
        std::string msg("Unable to download update file: " + std::string(ex.what()) + "\n");
        printf("%s", msg.c_str());
        updateIsAvailablePromise.set_exception(boost::current_exception());
        return;
    }

    try {
        remoteRelease = bfxReleases[0]; // get highest version
        localVersion  = BFXVersion::GetCurrentBFXVersion();
    } catch (std::exception& ex) {
        std::stringstream msg;
        msg << "Unable to parse version data during update check: " << ex.what() << std::endl;
        printf("%s", msg.str().c_str());
        updateIsAvailablePromise.set_exception(boost::current_exception());
        return;
    }
    lastRelease = remoteRelease;
    updateIsAvailablePromise.set_value(remoteRelease.getVersion() > localVersion);
}

BFXVersion BFXUpdater::ParseVersion(const std::string &versionFile)
{
    int majorVersion    = FromString<int>(GetDefineFromCFile(versionFile, "CLIENT_VERSION_MAJOR"));
    int minorVersion    = FromString<int>(GetDefineFromCFile(versionFile, "CLIENT_VERSION_MINOR"));
    int revisionVersion = FromString<int>(GetDefineFromCFile(versionFile, "CLIENT_VERSION_REVISION"));
    int buildVersion    = FromString<int>(GetDefineFromCFile(versionFile, "CLIENT_VERSION_BUILD"));
    return BFXVersion(majorVersion, minorVersion, revisionVersion, buildVersion);
}

std::string BFXUpdater::GetDefineFromCFile(const std::string &fileData, const std::string& fieldName)
{
    //regex of define in one or multiple lines
    const std::string regex_str = ".*\\s*#define\\s+" + fieldName + "\\s+[\\s*|(\\n)]+([^\\s]+)\\s*.*";
    boost::regex pieces_regex(regex_str);
    boost::smatch pieces_match;
    std::string piece;
    bool match_found = boost::regex_match(fileData, pieces_match, pieces_regex);
    if (match_found) {
        piece = pieces_match[1];
    } else {
        std::string error = "Unable to find match for " + fieldName + " in the downloaded file.";
        throw std::runtime_error(error.c_str());
    }
    return piece;
}

std::string BFXUpdater::RemoveCFileComments(const std::string &fileData)
{
    std::string result = fileData;

    //remove carriage return, as they could hinder detecting new lines
    std::string carriage_return_regex_str("\\r", boost::match_not_dot_newline);
    boost::regex carriage_return_regex(carriage_return_regex_str);
    result = boost::regex_replace(result, carriage_return_regex, "");

    //remove single line comments (//)
    std::string line_comments_regex_str("\\/\\/.*\\n");
    boost::regex line_comments_regex(line_comments_regex_str);
    result = boost::regex_replace(result, line_comments_regex, "", boost::match_not_dot_newline);

    //remove multi-line comments (/* */)
    std::string multiline_comments_regex_str("/\\*(.*?)\\*/"); // The "?" is to turn off greediness
    boost::regex multiline_comments_regex(multiline_comments_regex_str);
    result = boost::regex_replace(result, multiline_comments_regex, "");

    return result;
}
