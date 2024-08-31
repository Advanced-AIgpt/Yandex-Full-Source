#pragma once

#include "saved_address.h"

#include <alice/library/blackbox/blackbox.h>

#include <alice/bass/forms/context/context.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/util/error.h>

#include <library/cpp/scheme/scheme.h>

#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NUnitTest {
struct TTestContext;
} // namespace NUnitTest

namespace NTestSuiteTPersonalDataHelperUnitTest {
struct TTestCaseDataSyncBatchRequestSmoke;
struct TTestCaseDataSyncBatchRequestSimple;
struct TTestCaseVerifyBatchResponseCorrect;
struct TTestCaseVerifyBatchResponseMalformed;
struct TTestCaseVerifyBatchResponseMalformedJson;
struct TTestCaseVerifyBatchResponseError;
} // namespace NTestSuiteTPersonalDataHelperUnitTest

namespace NBASS {
class TContext;

class TPersonalDataHelper {
public:
    // WARNING: be careful when modifying this enum, because these
    // keys are used as keys in the DataSync, and also may be used
    // as parts of URL.
    //
    // It seems safe to add new keys, but keep in mind when
    // deleting or modifying existing keys that there already may
    // be key-value pairs in the DataSync, and they may become
    // unreachable.
    enum class EUserSpecificKey {
        UserName /* "user_name" */,
        Gender /* "gender" */,
        GuestUID /* "guest_uid" */,
        SkillRecommendation /* "skill_recommendation" */,
        TaxiHistory /* "taxi_history" */,
        PersonalTvScheduleHistory /* personal_tv_schedule_history */,
        AutomotivePromoCounters /* automotive_promo_counters */,
    };

    enum class EUserDeviceSpecificKey {
        Location /* "location" */
    };

    using EKey = std::variant<EUserSpecificKey, EUserDeviceSpecificKey>;

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

    using TUserInfo = NAlice::TBlackBoxFullUserInfoProto::TUserInfo;

    explicit TPersonalDataHelper(TContext& context, bool isTestBlackBox = false);

    bool GetUid(TString& uid) const;
    TMaybe<TString> GetUid() const;
    bool GetUserInfo(TUserInfo& info) const;
    bool GetTVM2UserTicket(TString& ticket) const;
    bool GetTVM2ServiceTicket(TStringBuf serviceId, TString& ticket) const;

    TSavedAddress GetDataSyncUserAddress(TStringBuf addressId) const;
    TResultValue SaveDataSyncUserAddress(TStringBuf addressId, const TSavedAddress& address);
    TResultValue UpdateDataSyncDeviceGeoPoint(TStringBuf uid, TStringBuf geoPointName);
    TResultValue DeleteDataSyncUserAddress(TStringBuf addressId) const;

    TResultValue SaveDataSyncKeyValues(TStringBuf uid, const TVector<TKeyValue>& kvs);
    TResultValue SaveDataSyncJsonValue(TStringBuf uid, EKey key, const NSc::TValue &jsonValue);

    TResultValue GetDataSyncJsonValue(TStringBuf uid, EKey key, NSc::TValue &jsonValue);

    NHttpFetcher::THandle::TRef GetDataSyncKeyRequestHandle(TStringBuf uid, EKey key);

    TResultValue GetDataSyncKeyValueFromResponse(TStringBuf uid, TStringBuf key,
                                                 const NHttpFetcher::TResponse::TRef response, TString& value);

    TResultValue GetDataSyncKeyValue(TStringBuf uid, EKey key, TString& value);

    TSavedAddress GetNavigatorUserAddress(TStringBuf addressId, TStringBuf searchText) const;
    TResultValue SaveNavigatorUserAddress(TStringBuf addressId, const TSavedAddress& address) const;

    // This function is used by /test_user handler to cleanup test user with tag 'kolonkish'.
    static bool CleanupTestUserKolonkish(TContext& ctx);

private:
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseDataSyncBatchRequestSmoke;
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseDataSyncBatchRequestSimple;
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseVerifyBatchResponseCorrect;
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseVerifyBatchResponseMalformed;
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseVerifyBatchResponseMalformedJson;
    friend struct NTestSuiteTPersonalDataHelperUnitTest::TTestCaseVerifyBatchResponseError;

    NSc::TValue PrepareDataSyncBatchRequestContent(const TVector<TKeyValue>& kvs);

    void AddTVM2AuthorizationHeaders(NHttpFetcher::TRequestBuilder& builder, TStringBuf uid) const;
    void AddTVM2AuthorizationHeaders(NHttpFetcher::TRequestBuilder& builder, bool isAuthorized) const;

    void AddTVM2AuthorizationHeaders(NHttpFetcher::TRequest& request, TStringBuf uid) const;
    void AddTVM2AuthorizationHeaders(NHttpFetcher::TRequest& request, bool isAuthorized) const;

    // If there's an error in |response|, logs it and returns TError.
    static TResultValue VerifyResponse(const NHttpFetcher::TResponse::TRef response,
                                       const std::function<void(TStringBuilder&)>& fn);

    // Same as VerifyResponse, but checks result of batch request, logs and returns first error if any
    static TResultValue VerifyBatchResponse(const NHttpFetcher::TResponse::TRef response,
                                            const std::function<void(TStringBuilder&)>& fn);

    TResultValue SaveDataSyncBatch(TStringBuf uid, TStringBuf body);

    TResultValue GetPrefetchedDataSyncKeyValue(TStringBuf key, TString& value);

    NHttpFetcher::TRequestPtr CreateDataSyncRequest(TStringBuf path) const;

private:
    TContext& Ctx;
    const bool IsTestBlackBox;
    const NSc::TValue& PersonalData;
    const bool UseUniproxyDatasyncProtocol;
};

} // namespace NBASS
