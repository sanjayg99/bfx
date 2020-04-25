#include "bfxversion.h"
#include "util.h"
#include "clientversion.h"

#include <stdexcept>

void BFXVersion::checkInitialization()
{
    if(major < 0 || minor < 0 || revision < 0 || build < 0)
        throw std::runtime_error("BFXVersion object is not initialized.");
}

BFXVersion::BFXVersion(int Major, int Minor, int Revision, int Build)
{
    major = Major;
    minor = Minor;
    revision = Revision;
    build = Build;
}

bool BFXVersion::operator>(const BFXVersion &rhs)
{
    checkInitialization();
    if(this->major > rhs.major)
        return true;
    else if(this->major < rhs.major)
        return false;

    if(this->minor > rhs.minor)
        return true;
    else if(this->minor < rhs.minor)
        return false;

    if(this->revision > rhs.revision)
        return true;
    else if(this->revision < rhs.revision)
        return false;

    if(this->build > rhs.build)
        return true;
    else if(this->build < rhs.build)
        return false;

    return false;
}

bool BFXVersion::operator<(const BFXVersion &rhs)
{
    return (!(*this > rhs) && !(*this == rhs));
}

bool BFXVersion::operator>=(const BFXVersion &rhs)
{
    return !(*this < rhs);
}

bool BFXVersion::operator<=(const BFXVersion &rhs)
{
    return !(*this > rhs);
}

bool BFXVersion::operator==(const BFXVersion &rhs)
{
    return (major    == rhs.major &&
            minor    == rhs.minor &&
            revision == rhs.revision &&
            build    == rhs.build);
}

bool BFXVersion::operator!=(const BFXVersion &rhs)
{
    return !(*this == rhs);
}

std::string BFXVersion::toString()
{
    return ToString(major)    + "." +
           ToString(minor)    + "." +
           ToString(revision) + "." +
           ToString(build);

}

void BFXVersion::clear()
{
    *this = BFXVersion();
}

void BFXVersion::setMajor(int value)
{
    major = value;
}

void BFXVersion::setMinor(int value)
{
    minor = value;
}

void BFXVersion::setRevision(int value)
{
    revision = value;
}

void BFXVersion::setBuild(int value)
{
    build = value;
}

int BFXVersion::getMajor() const
{
    return major;
}

int BFXVersion::getMinor() const
{
    return minor;
}

int BFXVersion::getRevision() const
{
    return revision;
}

int BFXVersion::getBuild() const
{
    return build;
}

BFXVersion BFXVersion::GetCurrentBFXVersion()
{
    return BFXVersion(CLIENT_VERSION_MAJOR,
                         CLIENT_VERSION_MINOR,
                         CLIENT_VERSION_REVISION,
                         CLIENT_VERSION_BUILD);
}
