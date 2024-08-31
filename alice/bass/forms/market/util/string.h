#pragma once

#include <alice/bass/forms/market/types.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <util/generic/string.h>
#include <util/system/yassert.h>
#include <util/system/type_name.h>

namespace NBASS {

namespace NMarket {

template<class T>
T FromStringWithLogging(const TStringBuf from, T default_)
{
    T ret;
    if (TryFromString<T>(from, ret)) {
        return ret;
    }
    LOG(ERR) << "Can't cast \"" << from << "\" to " << TypeName<T>() << ". Using default " << default_ << Endl;
    Y_ASSERT(false);
    return default_;
}

TString ToAddressString(const TAddressSchemeConst& address);
TString ToAddressString(const TAddressScheme& address);

} // namespace NMarket

} // namespace NBASS
