#pragma once

#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/util/generic_error.h>

#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/system/yassert.h>

namespace NAlice::NDataSync {

enum class EUserSpecificKey {
    UserName /* "user_name" */,
    Gender /* "gender" */,
    GuestUID /* "guest_uid" */,
    GuestsFolder /* "guests_folder" */,
    SkillRecommendation /* "skill_recommendation" */,
    TaxiHistory /* "taxi_history" */,
    PersonalTvScheduleHistory /* personal_tv_schedule_history */
};

enum class EUserDeviceSpecificKey {
    Location /* "location" */
};

enum class EEnrollmentSpecificKey {
    UserName /* "user_name" */,
    Gender /* "gender" */,
};

using EKey = std::variant<EUserSpecificKey, EUserDeviceSpecificKey, EEnrollmentSpecificKey>;

struct TKeyValue {
    TKeyValue(const EKey& key, TStringBuf value)
        : Key(key)
        , Value(value) {
    }

    TKeyValue(const EKey& key, const TString& value)
        : Key(key)
        , Value(value) {
    }

    bool operator==(const TKeyValue& rhs) const {
        return Key == rhs.Key && Value == rhs.Value;
    }

    EKey Key;
    TString Value;
};

enum class EErrorType {
    InvalidResponse /* invalid_response */,
    InvalidRequest /* invalid_request */,
    NoDataSyncKeyFound /* "no_data_sync_key_found" */,
    RequestFailed /* request_failed */
};

using TError = NBASS::TGenericError<EErrorType>;

using TResult = TMaybe<TError>;

class TDataSyncAPI final {
public:
    struct IDelegate {
        virtual ~IDelegate() = default;

        virtual NHttpFetcher::TRequestPtr Request(TStringBuf path) const = 0;
        virtual NHttpFetcher::TRequestPtr AttachRequest(TStringBuf path,
                                                        NHttpFetcher::IMultiRequest::TRef multiRequest) const = 0;

        virtual void AddTVM2AuthorizationHeaders(TStringBuf userId, NHttpFetcher::TRequestPtr& request) const = 0;

        virtual TString GetDeviceModel() const = 0;
        virtual TString GetDeviceId() const = 0;
        virtual TString GetPersId() const = 0;
    };

    explicit TDataSyncAPI(const IDelegate& delegate);

    TResult Get(TStringBuf userId, EKey key, TString& value);
    TResult Save(TStringBuf userId, EKey key, const NSc::TValue& jsonValue);
    TResult SaveBatch(TStringBuf userId, const TVector<TKeyValue>& kvs);

private:
    const IDelegate& Delegate;
};

TString ToDataSyncKey(const TDataSyncAPI::IDelegate& delegate, EKey key);

} // namespace NAlice::NDataSync
