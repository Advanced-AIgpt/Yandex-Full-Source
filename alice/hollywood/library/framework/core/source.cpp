//
// HOLLYWOOD FRAMEWORK
// TSource interfaces
//

#include "source.h"

#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/hollywood/library/util/service_context.h>

#include <alice/library/json/json.h>

namespace NAlice::NHollywoodFw {

/*
    Get response from http request
*/

TMaybe<TProtoStringType> TSource::GetRawHttpContent(TStringBuf responseKey, bool throwOnFailure) const {
    if (responseKey == "") {
        responseKey = NHollywood::PROXY_RESPONSE_KEY_DEFAULT;
    }
    TMaybe<NAppHostHttp::THttpResponse> response = NHollywood::GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(Ctx_, responseKey);
    if (!response) {
        const TStringBuf message = "Request failed, no response was received: ";
        LOG_ERROR(Request_.Debug().Logger()) << message << responseKey;
        if (throwOnFailure) {
            ythrow yexception() << message << responseKey;
        }
        return Nothing();
    }

    const bool success = (response->GetStatusCode() >= 200 && response->GetStatusCode() < 300);
    LOG_INFO(Request_.Debug().Logger()) << "Request " << (success ? "succeded" : "failed")
                                        << " with the status code: " << response->GetStatusCode();
    if (!success) {
        const TStringBuf message = "Response failed";
        LOG_ERROR(Request_.Debug().Logger()) << message;
        if (throwOnFailure) {
            ythrow yexception() << message;
        }
        return Nothing();
    }
    return *response->MutableContent();
}

const NJson::TJsonValue TSource::GetHttpResponseJson(TStringBuf responseKey /*= "" PROXY_RESPONSE_KEY_DEFAULT*/, bool throwOnFailure /*= true*/) const {
    TMaybe<TProtoStringType> content = GetRawHttpContent(responseKey, throwOnFailure);
    return content.Defined() ? JsonFromString(*content) : NJson::TJsonValue{};
}

} // namespace NAlice::NHollywoodFw {
