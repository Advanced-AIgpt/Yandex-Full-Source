#include "key_converter.h"

#include <util/string/builder.h>

namespace NCachalot::NPrivate {

    constexpr TStringBuf REQUEST_POSTFIX = "_request";
    constexpr TStringBuf RESPONSE_POSTFIX = "_response";

    constexpr TStringBuf OK_FLAG_POSTFIX = "_ok";
    constexpr TStringBuf NO_CONTENT_FLAG_POSTFIX = "_no_content";
    constexpr TStringBuf NOT_FOUND_FLAG_POSTFIX = "_not_found";
    constexpr TStringBuf FAIL_FLAG_POSTFIX = "_fail";

    TStringBuf RemoveRequestPostfixIfExist(TStringBuf requestKey) {
        return requestKey.EndsWith(REQUEST_POSTFIX)
               ? requestKey.SubStr(0, requestKey.size() - REQUEST_POSTFIX.size())
               : requestKey;
    }

    TStringBuf ResolveFlagPostfixByStatus(EResponseStatus responseStatus) {
        switch (responseStatus) {
            case EResponseStatus::OK:
            case EResponseStatus::CREATED:
                return OK_FLAG_POSTFIX;
            case EResponseStatus::NO_CONTENT:
                return NO_CONTENT_FLAG_POSTFIX;
            case EResponseStatus::NOT_FOUND:
                return NOT_FOUND_FLAG_POSTFIX;
            default:
                return FAIL_FLAG_POSTFIX;
        }
    }

}

namespace NCachalot {

    TString MakeResponseKeyFromRequest(TStringBuf requestKey) {
        return TStringBuilder()
            << NPrivate::RemoveRequestPostfixIfExist(requestKey)
            << NPrivate::RESPONSE_POSTFIX;
    }

    TString MakeFlagFromRequestKey(TStringBuf requestKey, EResponseStatus responseStatus) {
        return TStringBuilder()
            << NPrivate::RemoveRequestPostfixIfExist(requestKey)
            << NPrivate::ResolveFlagPostfixByStatus(responseStatus);
    }

} // namespace NCachalot
