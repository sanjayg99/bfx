#ifndef BFXTINPOINT_H
#define BFXTINPOINT_H

#include "bfxttransaction.h"

#include <boost/shared_ptr.hpp>

class BFXTInPoint
{
    boost::shared_ptr<BFXTTransaction> tx;
    unsigned int index;

public:
    BFXTInPoint();
    BFXTInPoint(boost::shared_ptr<BFXTTransaction> ptxIn, unsigned int nIn);
    void setNull();
    bool isNull() const;
};

#endif // BFXTINPOINT_H
