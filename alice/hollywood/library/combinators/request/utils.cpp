#include "utils.h"

#include <library/cpp/http/misc/httpcodes.h>

namespace NAlice::NHollywood::NCombinators {

TMaybe<TBlackBoxFullUserInfoProto>
ParseBlackBoxHttpResponse(const NAppHostHttp::THttpResponse& blackBoxHttpResponse,
                          TRTLogger& logger) {
    if (blackBoxHttpResponse.GetStatusCode() != HTTP_OK) {
        LOG_ERROR(logger) << "Blackbox response is not HTTP_OK: "
                          << blackBoxHttpResponse.ShortUtf8DebugString();
        return Nothing();
    }
    TBlackBoxFullUserInfoProto result;
    if (auto err = TBlackBoxApi{}.ParseFullInfo(blackBoxHttpResponse.GetContent()).MoveTo(result)) {
        LOG_ERROR(logger) << err->Message();
        return Nothing();
    }
    Y_ASSERT(result.IsInitialized());
    return result;
}

} // NAlice::NHollywood::NCombinators
