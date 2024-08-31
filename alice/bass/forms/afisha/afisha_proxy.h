#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <library/cpp/scheme/scheme.h>
#include <util/generic/maybe.h>


namespace NBASS {
namespace NAfisha {

class TAfishaProxy {
public:
    explicit TAfishaProxy(TContext& ctx);

    TMaybe<NSc::TValue> GetPlaceRepertory(const TStringBuf placeId, const int limit);

private:
    TMaybe<NSc::TValue> DoRequest(const NSc::TValue& postData, const TStringBuf queryName);

    TContext& Ctx;
};
}
}
