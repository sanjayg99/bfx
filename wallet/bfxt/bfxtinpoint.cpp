#include "bfxtinpoint.h"


BFXTInPoint::BFXTInPoint()
{
    setNull();
}

BFXTInPoint::BFXTInPoint(boost::shared_ptr<BFXTTransaction> ptxIn, unsigned int indexIn)
{
    tx = ptxIn;
    index = indexIn;
}

void BFXTInPoint::setNull()
{
    tx.reset();
    index = (unsigned int) -1;
}

bool BFXTInPoint::isNull() const
{
    return (tx == nullptr && index == (unsigned int) -1);
}
