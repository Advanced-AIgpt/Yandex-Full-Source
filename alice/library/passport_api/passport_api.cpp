#include "passport_api.h"

#include <library/cpp/scheme/scheme.h>

#include <util/generic/strbuf.h>
#include <util/string/builder.h>

namespace NAlice {

namespace {
// When value represents a string, returns it's contents.
// When value represents an array, selects only string items and joins them with ", ".
// Otherwise, returns an empty string.
TString CollectErrors(const NSc::TValue& value) {
    if (value.IsString())
        return TString{value.GetString()};

    if (!value.IsArray())
        return TString{};

    TStringBuilder builder;
    bool empty = true;

    const NSc::TArray& array = value.GetArray();
    for (const NSc::TValue& item : array) {
        if (!item.IsString())
            continue;

        if (!empty)
            builder << ", ";

        builder << item.GetString();
        empty = false;
    }

    return builder;
}

} // namespace

TPassportAPI::TResult TPassportAPI::RegisterKolonkish(NHttpFetcher::TRequestPtr request, TStringBuf consumer,
                                                      TStringBuf userAuthorizationHeader, TStringBuf userIP) {
    if (!request)
        return TResult{TError{TError::EType::BadParams, "Can't make request"}};

    request->AddCgiParam("consumer", consumer);
    request->AddHeader("Ya-Consumer-Authorization", userAuthorizationHeader);
    request->AddHeader("Ya-Consumer-Client-Ip", userIP);
    request->SetMethod("POST");

    NHttpFetcher::THandle::TRef handle = request->Fetch();
    if (!handle)
        return TResult{TError{TError::EType::InternalError, "Can't make request"}};

    NHttpFetcher::TResponse::TRef response = handle->Wait();
    if (!response)
        return TResult{TError{TError::EType::NoResponse}};

    if (response->IsError())
        return TResult{TError{TError::EType::ResponseError, response->GetErrorText()}};

    const NSc::TValue data = NSc::TValue::FromJson(response->Data);

    const NSc::TValue& status = data.TrySelect("status");
    if (!status.IsString())
        return TResult{TError{TError::EType::ResponseError, "no status field in response"}};

    if (status.GetString() == "ok") {
        const TString uid = data.TrySelect("uid").ForceString();
        const TString code = data.TrySelect("code").ForceString();

        if (uid.empty())
            return TResult{TError{TError::EType::ResponseError, "no uid"}};
        if (code.empty())
            return TResult{TError{TError::EType::ResponseError, "no code"}};

        return TResult{uid, code};
    }

    if (status.GetString() == "error") {
        const NSc::TValue& error = data.TrySelect("errors");
        return TResult{TError{TError::EType::ResponseError, CollectErrors(error)}};
    }

    return TResult{TError{TError::EType::ResponseError, TStringBuilder() << "unknown status field value: " << status}};
}

} // namespace NAlice
