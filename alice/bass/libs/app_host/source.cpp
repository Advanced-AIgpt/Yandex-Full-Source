#include "source.h"

namespace NBASS {

TGenericAppHostSource::TGenericAppHostSource(NSc::TValue data)
    : Data(std::move(data))
{
}

NSc::TValue TGenericAppHostSource::GetSourceInit() const {
    return Data;
}

} // NBASS
