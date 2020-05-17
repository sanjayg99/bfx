#ifndef BFXTSENDTOKENSONERECIPIENTDATA_H
#define BFXTSENDTOKENSONERECIPIENTDATA_H

#include "json_spirit.h"
#include "bfxtscript.h"
#include <string>

class BFXTSendTokensOneRecipientData
{
public:
    std::string         tokenId;
    std::string         destination;
    BFXTInt             amount;
    json_spirit::Object exportJsonData() const;
    std::string         exportJsonDataAsString() const;
};

#endif // BFXTSENDTOKENSONERECIPIENTDATA_H
