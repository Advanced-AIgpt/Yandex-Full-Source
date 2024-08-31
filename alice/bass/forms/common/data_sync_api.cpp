#include "data_sync_api.h"

#include <alice/bass/forms/context/context.h>
#include <util/string/cast.h>

namespace NBASS {

// TDataSyncAPI ----------------------------------------------------------------
TResultValue TDataSyncAPI::Save(TContext& context, TStringBuf uid, const TVector<TKeyValue>& kvs) {
    return TPersonalDataHelper(context).SaveDataSyncKeyValues(uid, kvs);
}

TResultValue TDataSyncAPI::Get(TContext& context, TStringBuf uid, EKey key, TString& value) {
    return TPersonalDataHelper(context).GetDataSyncKeyValue(uid, key, value);
}

TDataSyncAPI::TAsyncGetter TDataSyncAPI::GetAsync(TContext& context, TStringBuf uid,
                                                  EKey key, const TDuration& additionalTimeout)
{
    TPersonalDataHelper personalDataHelper = TPersonalDataHelper(context);
    auto requestHandle = personalDataHelper.GetDataSyncKeyRequestHandle(uid, key);
    TString uidCopy = ToString(uid);
    return
        [personalDataHelper, requestHandle, uidCopy, key, additionalTimeout]() mutable -> TDataSyncAPI::TAsyncResult {
            const auto response = requestHandle->WaitFor(additionalTimeout);

            if (!response) {
                return {TError{TError::EType::TIMEOUT}};
            }

            TString value;
            if (auto error = personalDataHelper.GetDataSyncKeyValueFromResponse(uidCopy, ToString(key), response, value)) {
                return {error.GetRef()};
            }
            return {value};
        };
}

// TDataSyncAPIStub ------------------------------------------------------------
TResultValue TDataSyncAPIStub::Save(TContext& /* context */, TStringBuf uid, const TVector<TKeyValue>& kvs) {
    return Save(uid, kvs);
}

TResultValue TDataSyncAPIStub::Save(TStringBuf uid, const TVector<TKeyValue>& kvs) {
    auto& data = Data[uid];
    for (const auto& kv : kvs)
        data[ToString(kv.Key)] = kv.Value;
    return {};
}

TResultValue TDataSyncAPIStub::Get(TContext& /* context */, TStringBuf uid, EKey key,
                                   TString& value) {
    ++NumGetCalled;
    const auto errorIterator = ErrorUidKeys.find(uid);
    if (errorIterator != ErrorUidKeys.end() && errorIterator->second.find(key) != errorIterator->second.end()) {
        return TError{TError::EType::SYSTEM, TStringBuilder()
                                                      << "Stubbed system error for " << uid << " " << key};
    }

    const auto eit = Data.find(uid);
    if (eit == Data.end()) {
        return TError{TError::EType::NODATASYNCKEYFOUND, TStringBuilder() << "No entry for " << uid};
    }

    const auto& data = eit->second;

    const auto vit = data.find(ToString(key));
    if (vit == data.end()) {
        return TError{TError::EType::NODATASYNCKEYFOUND, TStringBuilder()
                                                                  << "No entry for " << uid << " " << key};
    }

    value = vit->second;

    return {};
}

TDataSyncAPI::TAsyncGetter TDataSyncAPIStub::GetAsync(TContext& context, TStringBuf uid, EKey key,
                                                      const TDuration& /* additionalTimeout */) {
    return [this, &context, uid, key]() -> TDataSyncAPI::TAsyncResult {
        TString value;
        if (const auto error = Get(context, uid, key, value)) {
            return {error.GetRef()};
        }
        return {value};
    };
}

void TDataSyncAPIStub::FailOnGet(TStringBuf uid, EKey key) {
    ErrorUidKeys[uid].insert(key);
}

} // namespace NBASS
