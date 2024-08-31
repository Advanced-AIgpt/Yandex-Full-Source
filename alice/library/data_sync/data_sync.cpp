#include "data_sync.h"

namespace NAlice::NDataSync {

namespace {

constexpr TStringBuf BROADCAST_URL_SUFFIX = "/v1/batch/request";
constexpr TStringBuf KV_URL_SUFFIX = "/v1/personality/profile/alisa/kv";

constexpr TStringBuf ENROLLMENT_SPECIFIC_KEY_PREFIX = "enrollment";

constexpr TStringBuf CONTENT_TYPE_JSON = "application/json; charset=utf-8";

struct TDataSyncKeyBuilder {
    const TDataSyncAPI::IDelegate& Delegate;

    TString operator()(EUserSpecificKey key) {
        return ToString(key);
    }

    TString operator()(EUserDeviceSpecificKey key) {
        return TStringBuilder() << Delegate.GetDeviceModel() << "_" << Delegate.GetDeviceId() << "_" << ToString(key);
    }

    TString operator()(EEnrollmentSpecificKey key) {
        return TStringBuilder() << ENROLLMENT_SPECIFIC_KEY_PREFIX << "__" << Delegate.GetPersId() << "__" << ToString(key);
    }
};

TResult VerifyResponse(const NHttpFetcher::TResponse::TRef response) {
    if (!response || response->IsError()) {
        return TError{TError::EType::RequestFailed, response ? response->GetErrorText() : TString()};
    }

    return {};
}

TResult VerifyBatchResponse(const NHttpFetcher::TResponse::TRef response) {
    if (const auto error = VerifyResponse(response)) {
        return error;
    }
    auto responseData = NSc::TValue::FromJson(response->Data);

    if (!responseData.Has("items")) {
        return TError{TError::EType::InvalidResponse, TStringBuilder() << "Response data has no items: " << response->Data};
    }

    const auto& items = responseData.Get("items");

    if (!items.IsArray()) {
        return TError{TError::EType::InvalidResponse, TStringBuilder() << "Items is not an array: " << response->Data};
    }

    for (size_t requestNumber = 0; requestNumber < items.ArraySize(); ++requestNumber) {
        const auto& item = items.Get(requestNumber);
        if (!item.Has("code")) {
            return TError{TError::EType::InvalidResponse, TStringBuilder() << "Item has no code: " << response->Data};
        }
        auto code = item.Get("code");
        if (!code.IsIntNumber()) {
            return TError{TError::EType::InvalidResponse, TStringBuilder()
                                                              << "Item code is not a number: " << response->Data};
        }
        i64 codeNumber = code.GetIntNumber();
        if (IsUserError(codeNumber) || IsServerError(codeNumber)) {
            return TError{TError::EType::RequestFailed, TStringBuilder() << "Error in " << ToString(requestNumber)
                                                                         << " request, code " << ToString(codeNumber)};
        }
    }

    return {};
}

TResult GetDataSyncKeyValueFromResponse(const NHttpFetcher::TResponse::TRef response, TString& value) {
    if (response && response->IsError() && response->Code == HttpCodes::HTTP_NOT_FOUND) {
        return TResult(TError::EType::NoDataSyncKeyFound);
    }

    if (const auto error = VerifyResponse(response)) {
        return TResult(*error);
    }

    Y_ASSERT(response);
    const NSc::TValue d = NSc::TValue::FromJson(response->Data);

    if (!d.Has("value")) {
        return TError{TError::EType::InvalidResponse,
                      TStringBuilder() << "DataSync response doesn't have value field: " << response->Data};
    }

    const auto& v = d["value"];
    if (!v.IsString()) {
        return TError{TError::EType::InvalidResponse,
                      TStringBuilder() << "DataSync response value is not string: " << response->Data};
    }

    value = v.GetString();
    return {};
}

NSc::TValue PrepareDataSyncBatchRequestContent(const TDataSyncAPI::IDelegate& delegate,
                                               const TVector<TKeyValue>& kvs) {
    // Format is described here:
    // https://wiki.yandex-team.ru/disk/mpfs/platformapibatchrequests/
    NSc::TValue request;

    NSc::TArray& items = request["items"].GetArrayMutable();

    for (const auto& kv : kvs) {
        NSc::TValue body;
        body["value"] = kv.Value;

        NSc::TValue item;
        item["method"] = "PUT";
        item["relative_url"] = ToDataSyncKey(delegate, kv.Key);
        item["body"] = body.ToJson();

        items.push_back(item);
    }

    return request;
}

NHttpFetcher::THandle::TRef GetDataSyncKeyRequestHandle(TStringBuf userId, const TDataSyncAPI::IDelegate& delegate,
                                                        EKey key) {
    auto request = delegate.Request(ToDataSyncKey(delegate, key));
    delegate.AddTVM2AuthorizationHeaders(userId, request);

    return request->SetMethod("GET").Fetch();
}

TResult SaveDataSyncBatch(TStringBuf userId, const TDataSyncAPI::IDelegate& delegate, TStringBuf body) {
    if (userId.empty()) {
        return TError{TError::EType::InvalidResponse, "No uid for datasync"};
    }

    auto request = delegate.Request(BROADCAST_URL_SUFFIX);
    delegate.AddTVM2AuthorizationHeaders(userId, request);

    request->SetMethod("POST");
    request->SetContentType(CONTENT_TYPE_JSON);
    request->SetBody(body);

    auto response = request->Fetch()->Wait();

    return VerifyBatchResponse(response);
}

} // namespace

// TDataSyncAPI ----------------------------------------------------------------
TDataSyncAPI::TDataSyncAPI(const IDelegate& delegate)
    : Delegate(delegate) {
}

TResult TDataSyncAPI::Get(TStringBuf userId, EKey key, TString& value) {
    const auto response = GetDataSyncKeyRequestHandle(userId, Delegate, key)->Wait();
    return GetDataSyncKeyValueFromResponse(response, value);
}

TResult TDataSyncAPI::Save(TStringBuf userId, EKey key, const NSc::TValue& jsonValue) {
    TKeyValue keyValue{key, jsonValue.ToJson()};
    return SaveBatch(userId, {keyValue});
}

TResult TDataSyncAPI::SaveBatch(TStringBuf userId, const TVector<TKeyValue>& kvs) {
    if (kvs.empty())
        return {};

    return SaveDataSyncBatch(userId, Delegate, PrepareDataSyncBatchRequestContent(Delegate, kvs).ToJson());
}

TString ToDataSyncKey(const TDataSyncAPI::IDelegate& delegate, EKey key) {
    return TStringBuilder() << KV_URL_SUFFIX << '/' << std::visit(TDataSyncKeyBuilder{delegate}, key);
}

} // namespace NAlice::NDataSync
