#include "voiceprint_enroll.h"


#include <alice/bass/forms/common/blackbox_api.h>
#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/bass/ut/helpers.h>
#include <alice/bass/util/error.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/passport_api/passport_api.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/string/builder.h>

#include <cstddef>

using namespace NBASS;
using namespace NTestingHelpers;

using NAlice::TPassportAPI;

using TKeyValue = TDataSyncAPI::TKeyValue;
using EUserSpecificKey = TPersonalDataHelper::EUserSpecificKey;

namespace {

struct TPassportAPIFake : public TPassportAPI {
    TPassportAPIFake(const TPassportAPI::TResult& result)
        : Result(result) {
    }

    TResult RegisterKolonkish(NHttpFetcher::TRequestPtr /*request*/, TStringBuf /*consumer*/,
                              TStringBuf /*userAuthorizationHeader*/, TStringBuf /*userIP*/) override {
        ++NumCalls;
        return Result;
    }

    const TPassportAPI::TResult Result;
    size_t NumCalls = 0;
};

struct TDataSyncAPIFake : public TDataSyncAPI {
    explicit TDataSyncAPIFake(bool fail = false)
        : Fail(fail) {
    }

    explicit TDataSyncAPIFake(const TVector<TKeyValue>& expectedKVs, bool fail = false)
        : ExpectedKVs(expectedKVs)
        , Fail(fail) {
    }

    TResultValue Save(TContext& /* context */, TStringBuf /* uid */, const TVector<TKeyValue>& kvs) override {
        UNIT_ASSERT_VALUES_EQUAL(kvs, ExpectedKVs);
        ++NumCalls;

        if (Fail) {
            return {TError(TError::EType::SYSTEM, "DataSync error")};
        }

        return TResultValue{};
    }

    TVector<TKeyValue> ExpectedKVs;
    size_t NumCalls = 0;
    bool Fail = false;
};

struct TExpectedCalls {
    size_t BlackBox = 0;
    size_t Passport = 0;
    size_t DataSync = 0;
};
} // namespace

namespace NVoiceprintTesting {

template<typename THandler>
class TBlackboxDatasyncDependentHandlerWrapper final {
public:
    TBlackboxDatasyncDependentHandlerWrapper(const TExpectedCalls& expectedCalls = TExpectedCalls(), bool fail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TDataSyncAPIStub>())
        , ExpectedCalls(expectedCalls)
        , Fail(fail) {
        Y_ASSERT(expectedCalls.Passport == 0);
    }

    TBlackboxDatasyncDependentHandlerWrapper(TStringBuf uid, const TExpectedCalls& expectedCalls = TExpectedCalls(),
                                             bool fail = false)
            : Handler(MakeHolder<TBlackBoxAPIFake>(uid), MakeHolder<TDataSyncAPIStub>())
            , ExpectedCalls(expectedCalls)
            , Fail(fail) {
        Y_ASSERT(expectedCalls.Passport == 0);
    }

    ~TBlackboxDatasyncDependentHandlerWrapper() {
        {
            TBlackBoxAPIFake* api = static_cast<TBlackBoxAPIFake*>(Handler.BlackBoxAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumCalls, ExpectedCalls.BlackBox);
        }

        {
            TDataSyncAPIStub* api = static_cast<TDataSyncAPIStub*>(Handler.DataSyncAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumGetCalled, ExpectedCalls.DataSync);
        }
    }

    TDataSyncAPIStub& GetDataSyncAPI() {
        return *static_cast<TDataSyncAPIStub*>(Handler.DataSyncAPI.Get());
    }

    void Do(TContext& ctx) {
        const auto error = Handler.Do(ctx);
        UNIT_ASSERT_VALUES_EQUAL(error.Defined(), Fail);
    }

private:
    THandler Handler;
    TExpectedCalls ExpectedCalls;
    bool Fail;
};

class TVoiceprintEnrollCollectVoiceWrapper final {
public:
    TVoiceprintEnrollCollectVoiceWrapper(const TPassportAPI::TResult& passportResponse,
                                         const TExpectedCalls& expectedCalls = TExpectedCalls(),
                                         bool dataSyncFail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TPassportAPIFake>(passportResponse),
                  MakeHolder<TDataSyncAPIFake>(dataSyncFail))
        , ExpectedCalls(expectedCalls) {
    }

    TVoiceprintEnrollCollectVoiceWrapper(const TPassportAPI::TResult& passportResponse,
                                         const TVector<TDataSyncAPI::TKeyValue>& expectedKVs,
                                         const TExpectedCalls& expectedCalls = TExpectedCalls(),
                                         bool dataSyncFail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TPassportAPIFake>(passportResponse),
                  MakeHolder<TDataSyncAPIFake>(expectedKVs, dataSyncFail))
        , ExpectedCalls(expectedCalls) {
    }

    TVoiceprintEnrollCollectVoiceWrapper(TStringBuf uid, const TPassportAPI::TResult& passportResponse,
                                         const TVector<TDataSyncAPI::TKeyValue>& expectedKVs,
                                         const TExpectedCalls& expectedCalls = TExpectedCalls(),
                                         bool dataSyncFail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(uid), MakeHolder<TPassportAPIFake>(passportResponse),
                  MakeHolder<TDataSyncAPIFake>(expectedKVs, dataSyncFail))
        , ExpectedCalls(expectedCalls) {
    }

    ~TVoiceprintEnrollCollectVoiceWrapper() {
        {
            TBlackBoxAPIFake* api = static_cast<TBlackBoxAPIFake*>(Handler.BlackBoxAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumCalls, ExpectedCalls.BlackBox);
        }

        {
            TPassportAPIFake* api = static_cast<TPassportAPIFake*>(Handler.PassportAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumCalls, ExpectedCalls.Passport);
        }

        {
            TDataSyncAPIFake* api = static_cast<TDataSyncAPIFake*>(Handler.DataSyncAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumCalls, ExpectedCalls.DataSync);
        }
    }

    void Do(TContext& ctx) {
        const auto error = Handler.Do(ctx);
        UNIT_ASSERT(!error);
    }

private:
    TVoiceprintEnrollCollectVoiceHandler Handler;
    TExpectedCalls ExpectedCalls;
};

class TRequestResponseBuilder {
public:
    TRequestResponseBuilder& AddExperiment(TStringBuf experiment) {
        result["meta"]["experiments"][experiment].SetBool(true);
        return *this;
    }

    TRequestResponseBuilder& SetBasicMeta(bool expectBiometricsScores) {
        result["meta"]["epoch"] = 1484311159;
        result["meta"]["tz"] = "Europe/Moscow";
        if (expectBiometricsScores) {
            result["meta"]["client_id"] = "ru.yandex.quasar/7.90 (none; none none)";
        }
        return *this;
    }

    TRequestResponseBuilder& SetRussiaRegion() {
        result["meta"]["region_id"] = 225;
        return *this;
    }

    TRequestResponseBuilder& SetBelarusRegion() {
        result["meta"]["region_id"] = 149;
        return *this;
    }

    TRequestResponseBuilder& SetBiometricScores(const NSc::TValue& requestId, bool withMode) {
        if (withMode) {
            // Since in this test every scenario is using HighTPR mode, we set scores in a way
            // that only HighTPR mode produces non-guest scores
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.1, "user_id": "1"},
                            {"score": 0.3, "user_id": "2"},
                            {"score": 0.01, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tpr",
                        "scores": [
                            {"score": 0.2, "user_id": "1"},
                            {"score": 0.6, "user_id": "2"},
                            {"score": 0.02, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tnr",
                        "scores": [
                            {"score": 0.04, "user_id": "1"},
                            {"score": 0.15, "user_id": "2"},
                            {"score": 0.008, "user_id": "3"}
                        ]
                    }
                ]
            })");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores": [
                    {"score": 0.225, "user_id": "1"},
                    {"score": 0.506, "user_id": "2"},
                    {"score": 0.03, "user_id": "3"}
                ]
            })");
        }
        result["meta"]["biometrics_scores"]["request_id"] = requestId;
        return *this;
    }

    TRequestResponseBuilder& SetLowBiometricScores(const NSc::TValue& requestId, bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.001, "user_id": "1"},
                            {"score": 0.002, "user_id": "2"},
                            {"score": 0.003, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tpr",
                        "scores": [
                            {"score": 0.002, "user_id": "1"},
                            {"score": 0.003, "user_id": "2"},
                            {"score": 0.004, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tnr",
                        "scores": [
                            {"score": 0.0, "user_id": "1"},
                            {"score": 0.001, "user_id": "2"},
                            {"score": 0.002, "user_id": "3"}
                        ]
                    }
                ]
            })");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores": [
                    {"score": 0.001, "user_id": "1"},
                    {"score": 0.002, "user_id": "2"},
                    {"score": 0.003, "user_id": "3"}
                ]
            })");
        }
        result["meta"]["biometrics_scores"]["request_id"] = requestId;
        return *this;
    }

    TRequestResponseBuilder& SetEmptyBiometricScores(const NSc::TValue& requestId, bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "scores_with_mode": []
                }
            )");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "scores": []
                }
            )");
        }
        result["meta"]["biometrics_scores"]["request_id"] = requestId;
        return *this;
    }

    TRequestResponseBuilder& SetForm(TStringBuf formName) {
        result["form"]["name"] = TString("personal_assistant.scenarios.") + formName;
        return *this;
    }

    TRequestResponseBuilder& AddUserNameSlot(TStringBuf userName, bool frozen, bool swear = false) {
        TStringBuf slotName;
        if (frozen) {
            slotName = "user_name_frozen";
        } else {
            slotName = "user_name";
        }
        return AddOptionalSlot(slotName, userName, (swear ? "swear" : "string") /* type */);
    }

    TRequestResponseBuilder& AddReadySlot(const NSc::TValue& value, bool frozen) {
        TStringBuf slotName;
        if (frozen) {
            slotName = "ready_frozen";
        } else {
            slotName = "ready";
        }
        return AddOptionalSlot(slotName, value, "string" /* type */);
    }

    TRequestResponseBuilder& AddVoiceRequestsSlot(const std::initializer_list<TStringBuf>& requests) {
        NSc::TValue value;
        value.AppendAll(requests);
        return AddOptionalSlot("voice_requests" /* name */, value, "list" /* type */);
    }

    TRequestResponseBuilder& AddPhrasesCountSlot(int count) {
        return AddOptionalSlot("phrases_count" /* name */, count, "int" /* type */);
    }

    TRequestResponseBuilder& AddUidSlot(TStringBuf uid) {
        return AddOptionalSlot("created_uid" /* name */, uid, "string" /* type */);
    }

    TRequestResponseBuilder& AddUsernameRepeatCountSlot(int count) {
        return AddOptionalSlot("username_repeat_count" /* name */, count, "int" /* type */);
    }

    TRequestResponseBuilder& AddIsUsernameErrorSlot() {
        return AddOptionalSlot("is_username_error" /* name */, true /* value */, "bool" /* type */);
    }

    TRequestResponseBuilder& AddServerRepeatCountSlot(int count) {
        return AddOptionalSlot("server_repeat_count" /* name */, count, "int" /* type */);
    }

    TRequestResponseBuilder& AddOptionalSlot(TStringBuf name, const NSc::TValue& value, TStringBuf type) {
        NSc::TValue slot;
        slot["name"] = name;
        if (type == "bool") {
            slot["value"].SetBool(value.GetBool());
        } else {
            slot["value"] = value;
        }
        slot["type"] = type;
        slot["optional"].SetBool(true);
        result["form"]["slots"].Push(slot);
        return *this;
    }

    TRequestResponseBuilder& AddSaveAction(const std::initializer_list<NSc::TValue>& requests, TStringBuf user_id) {
        NSc::TValue block;
        block["command_type"] = "save_voiceprint";
        block["type"] = "uniproxy-action";
        block["data"]["requests"].AppendAll(requests);
        block["data"]["user_id"] = user_id;
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddPlayerPauseBlock() {
        NSc::TValue block;
        block["command_sub_type"] = "voiceprint_player_pause";
        block["command_type"] = "player_pause";
        block["data"]["listening_is_possible"].SetBool(true);
        block["type"] = "command";
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddAttentionBlock(TStringBuf attention_type) {
        NSc::TValue block;
        block["type"] = "attention";
        block["attention_type"] = attention_type;
        block["data"] = NSc::TValue();
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddErrorAttentionBlock(TStringBuf message) {
        NSc::TValue block;
        block["type"].SetString("error");
        block["data"] = NSc::TValue::Null();
        block["error"]["type"].SetString("system");
        block["error"]["msg"].SetString(message);
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddAnalyticsInfoBlock(const TString& intent, const TString& productScenarioName) {
        NAlice::NScenarios::TBassAnalyticsInfoBuilder analyticsInfoBuilder(intent);
        analyticsInfoBuilder.SetProductScenarioName(productScenarioName);
        NSc::TValue block;
        block["type"].SetString("scenario_analytics_info");
        block["data"].SetString(analyticsInfoBuilder.SerializeAsBase64());
        return AddBlock(block);
    }

    TRequestResponseBuilder& AddBlock(const NSc::TValue& block) {
        result["blocks"].Push(block);
        return *this;
    }

    TRequestResponseBuilder& AddSlots() {
        result["form"]["slots"].SetArray();
        return *this;
    }

    NSc::TValue Build() const {
        return result;
    }

private:
    NSc::TValue result;
};

template<typename THandler>
void RunAndCheckResponse(
        const NSc::TValue& request,
        const NSc::TValue& response,
        TBlackboxDatasyncDependentHandlerWrapper<THandler> wrapper) {
    TContext::TPtr ctx = MakeAuthorizedContext(request);
    wrapper.Do(*ctx);
    CheckResponse(*ctx, response);
}

} // namespace NVoiceprintTesting

using namespace NVoiceprintTesting;

Y_UNIT_TEST_SUITE(TVoiceprintEnrollUnitTest) {
    Y_UNIT_TEST(Biometry) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll")
                    .AddSlots()
                    .AddPlayerPauseBlock()
                    .Build();
            RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(BiometryNoAuth) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddOptionalSlot("is_need_login", true, "bool")
                    .Build();
            TContext::TPtr ctx = MakeContext(request);
            TBlackboxDatasyncDependentHandlerWrapper<TVoiceprintEnrollHandler> wrapper;
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(BiometryTooManyUsers) {
        for (auto withMode : {true, false}) {
            for (auto isGuest : {true, false}) {
                auto requestBuilder =
                    TRequestResponseBuilder()
                        .SetBasicMeta(true /* expectBiometricsScores */)
                        .SetForm("voiceprint_enroll")
                        .AddExperiment("quasar_biometry_limit_users");
                auto request = (
                        isGuest ?
                        requestBuilder.SetLowBiometricScores("456" /* requestId */, withMode) :
                        requestBuilder.SetBiometricScores("456" /* requestId */, withMode)
                    ).Build();
                auto responseBuilder =
                    TRequestResponseBuilder()
                        .SetForm("voiceprint_enroll__finish")
                        .AddOptionalSlot("is_too_many_enrolled_users", true, "bool");
                if (!isGuest) {
                    responseBuilder.AddAttentionBlock("known_user");
                }
                auto response = responseBuilder.Build();
                RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {});
            }
        }
    }

    Y_UNIT_TEST(NoBiometryKnown) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAttentionBlock("what_is_my_name__enroll_requested")
                .SetForm("what_is_my_name")
                .AddOptionalSlot("is_known", true, "bool")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TBlackboxDatasyncDependentHandlerWrapper<TVoiceprintEnrollHandler> wrapper("1", {.BlackBox = 1, .DataSync = 1});
        auto& dataSyncAPI = wrapper.GetDataSyncAPI();
        dataSyncAPI.Save(*ctx, "1", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Eve")}}});
        wrapper.Do(*ctx);
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(NoBiometryUnknown) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAttentionBlock("what_is_my_name__enroll_requested")
                .SetForm("what_is_my_name")
                .AddOptionalSlot("is_known", false, "bool")
                .Build();
        RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {"1", {.BlackBox = 1, .DataSync = 1}});
    }

    Y_UNIT_TEST(NoBiometryBlackboxError) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAttentionBlock("what_is_my_name__enroll_requested")
                .AddErrorAttentionBlock("Error requesting UID from BlackBox API")
                .SetForm("what_is_my_name")
                .AddOptionalSlot("is_known", false, "bool")
                .Build();
        RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {{.BlackBox = 1}, true /* fail */});
    }

    Y_UNIT_TEST(NoBiometryInvalidBiocontext) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetBiometricScores("456" /* requestId */, true /* withMode */)
                .SetForm("voiceprint_enroll")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__finish")
                .AddOptionalSlot("is_server_error" /* name */, true /* value */, "bool" /* type */)
                .Build();
        RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {{} /* expectedCalls */, false /* fail */});
    }

    Y_UNIT_TEST(ValidRegion) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetRussiaRegion()
                    .SetForm("voiceprint_enroll")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll")
                    .AddSlots()
                    .AddPlayerPauseBlock()
                    .Build();
            RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(NoRegion) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll")
                    .AddSlots()
                    .AddPlayerPauseBlock()
                    .Build();
            RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(InvalidRegion) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetBelarusRegion()
                    .SetForm("voiceprint_enroll")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddSlots()
                    .AddAttentionBlock("invalid_region")
                    .Build();
            RunAndCheckResponse<TVoiceprintEnrollHandler>(request, response, {});
        }
    }
}

Y_UNIT_TEST_SUITE(TVoiceprintEnrollCollectVoiceUnitTest) {
    Y_UNIT_TEST(GenderDetector) {
        using THandler = TVoiceprintEnrollCollectVoiceHandler;

        UNIT_ASSERT_VALUES_EQUAL(THandler::DetectGender(TString{"я готова"}), THandler::GenderFemale);
        UNIT_ASSERT_VALUES_EQUAL(THandler::DetectGender(TString{"я готов"}), THandler::GenderMale);
        UNIT_ASSERT_VALUES_EQUAL(THandler::DetectGender(TString{}), THandler::GenderUndefined);
    }

    Y_UNIT_TEST(UserNameError) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("" /* name */, false /* frozen */)
                    .SetBiometricScores("1" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("" /* name */, false /* frozen */)
                    .AddVoiceRequestsSlot({})
                    .AddUsernameRepeatCountSlot(1)
                    .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};
            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(UserNameError2) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Swear" /* name */, false /* frozen */, true /* swear */)
                    .AddUsernameRepeatCountSlot(2)
                    .SetBiometricScores("1" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddIsUsernameErrorSlot()
                    .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};
            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }
    Y_UNIT_TEST(UserNameError3) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Swear" /* name */, false /* frozen */, true /* swear */)
                    .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                    .AddUsernameRepeatCountSlot(1)
                    .SetBiometricScores("1" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Swear" /* name */, false /* frozen */, true /* swear */)
                    .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                    .AddUsernameRepeatCountSlot(2)
                    .AddVoiceRequestsSlot({})
                    .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};
            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FreezeUserName) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                    .SetBiometricScores("1" /* requestId */, withMode)
                    .Build();

            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                    .AddVoiceRequestsSlot({})
                    .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                    .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FreezeAnotherUserName) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Bob" /* name */, false /* frozen */)
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .SetBiometricScores("1" /* requestId */, withMode)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Bob" /* name */, false /* frozen */)
                .AddUserNameSlot("Bob" /* name */, true /* frozen */)
                .AddVoiceRequestsSlot({})
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(ReadyWithUserNameNoFrozenUserName) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* value */, false /* frozen */)
                .SetBiometricScores("1" /* requestId */, withMode)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddReadySlot(NSc::TValue::Null() /* value */, false /* frozen */)
                .AddVoiceRequestsSlot({})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(ReadyNoUserNameNoFrozenUserName) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddReadySlot("я готова" /* value */, false /* frozen */)
                .SetBiometricScores("1" /* requestId */, withMode)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddReadySlot(NSc::TValue::Null() /* value */, false /* frozen */)
                .AddVoiceRequestsSlot({})
                .AddUsernameRepeatCountSlot(1)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FreezeReady) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .SetBiometricScores("1" /* requestId */, withMode)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(0 /* count */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }
    Y_UNIT_TEST(IncreaseCounter) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(3 /* count */)
                .SetBiometricScores("4" /* requestId */, withMode)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(4 /* count */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }
    Y_UNIT_TEST(Repeat) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(3 /* count */)
                .SetBiometricScores("4" /* requestId */, withMode)
                .AddOptionalSlot("user_repeat" /* name */, "повтори" /* value */, "string" /* type */)
                .Build();

            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                    .AddVoiceRequestsSlot({"1", "2", "3"})
                    .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                    .AddReadySlot("я готова" /* name */, false /* frozen */)
                    .AddReadySlot("я готова" /* name */, true /* frozen */)
                    .AddPhrasesCountSlot(3 /* count */)
                    .AddOptionalSlot("user_repeat" /* name */, NSc::TValue::Null() /* value */, "string" /* type */)
                    .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }
    Y_UNIT_TEST(NoRequestId) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetBiometricScores(NSc::TValue::Null() /* requestId */, withMode)
                .AddPhrasesCountSlot(3 /* count */)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(3 /* count */)
                .AddServerRepeatCountSlot(1 /* count */)
                .AddOptionalSlot("is_server_repeat" /* name */, true /* value */, "bool" /* type */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoBiometry) {
        auto request = TRequestResponseBuilder()
                           .SetBasicMeta(false /* expectBiometricsScores */)
                           .SetForm("voiceprint_enroll__collect_voice")
                           .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                           .AddVoiceRequestsSlot({"1", "2", "3"})
                           .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                           .AddReadySlot("я готова" /* name */, false /* frozen */)
                           .AddReadySlot("я готова" /* name */, true /* frozen */)
                           .AddPhrasesCountSlot(3 /* count */)
                           .Build();

        auto response = TRequestResponseBuilder()
                            .SetForm("voiceprint_enroll__collect_voice")
                            .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                            .AddVoiceRequestsSlot({"1", "2", "3"})
                            .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                            .AddReadySlot("я готова" /* name */, false /* frozen */)
                            .AddReadySlot("я готова" /* name */, true /* frozen */)
                            .AddPhrasesCountSlot(3 /* count */)
                            .AddServerRepeatCountSlot(1 /* count */)
                            .AddOptionalSlot("is_server_repeat" /* name */, true /* value */, "bool" /* type */)
                            .Build();

        // PassportAPI should not be called.
        const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

        TVoiceprintEnrollCollectVoiceWrapper wrapper(result);
        TContext::TPtr ctx = MakeAuthorizedContext(request);
        wrapper.Do(*ctx);
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(FinishSuccessUser) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetEmptyBiometricScores("5" /* requestId */, withMode)
                .AddPhrasesCountSlot(4 /* count */)
                .Build();

            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                    .AddUidSlot("uid" /* uid */)
                    .AddSaveAction({"1", "2", "3", "4", "5"} /* requests */, "uid" /* user_id */)
                    .Build();

            const TPassportAPI::TResult result{"guest_uid", "guest_code"};

            TVector<TDataSyncAPI::TKeyValue> expectedKVs;
            expectedKVs.emplace_back(EUserSpecificKey::UserName, TStringBuf("Alice"));
            expectedKVs.emplace_back(EUserSpecificKey::Gender, TStringBuf("female"));
            expectedKVs.emplace_back(EUserSpecificKey::GuestUID, TStringBuf("guest_uid"));

            TVoiceprintEnrollCollectVoiceWrapper wrapper("uid", result, expectedKVs,
                                                         {.BlackBox = 1, .Passport = 1, .DataSync = 1});
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FinishSuccessKolonkish) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetBiometricScores("5" /* requestId */, withMode)
                .AddPhrasesCountSlot(4 /* count */)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__finish")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddUidSlot("new_user_uid" /* uid */)
                .AddSaveAction({"1", "2", "3", "4", "5"} /* requests */,
                               "new_user_uid" /* user_id */)
                .Build();

            const TPassportAPI::TResult result{"new_user_uid", "new_user_code"};

            TVector<TDataSyncAPI::TKeyValue> expectedKVs;
            expectedKVs.emplace_back(EUserSpecificKey::UserName, TStringBuf("Alice"));
            expectedKVs.emplace_back(EUserSpecificKey::Gender, TStringBuf("female"));

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result, expectedKVs, {.Passport = 1, .DataSync = 1});
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FinishBlackboxErrorTooManyRepeats) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetEmptyBiometricScores("5" /* requestId */, withMode)
                .AddPhrasesCountSlot(4 /* count */)
                .AddServerRepeatCountSlot(3 /* count */)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__finish")
                .AddOptionalSlot("is_server_error" /* name */, true /* value */, "bool" /* type */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result, {.BlackBox = 1});
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FinishPassportError) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetBiometricScores("5" /* requestId */, withMode)
                .AddPhrasesCountSlot(4 /* count */)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(4 /* count */)
                .AddServerRepeatCountSlot(1 /* count */)
                .AddOptionalSlot("is_server_repeat" /* name */, true /* value */, "bool" /* type */)
                .Build();

            // PassportAPI should not be called.
            const TPassportAPI::TResult result{TPassportAPI::TError::EType::ResponseError};

            TVoiceprintEnrollCollectVoiceWrapper wrapper(result, {.Passport = 1});
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FinishDataSyncError) {
        for (auto withMode : {true, false}) {
            auto request = TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .SetEmptyBiometricScores("5" /* requestId */, withMode)
                .AddPhrasesCountSlot(4 /* count */)
                .Build();

            auto response = TRequestResponseBuilder()
                .SetForm("voiceprint_enroll__collect_voice")
                .AddUserNameSlot("Alice" /* name */, false /* frozen */)
                .AddVoiceRequestsSlot({"1", "2", "3", "4"})
                .AddUserNameSlot("Alice" /* name */, true /* frozen */)
                .AddReadySlot("я готова" /* name */, false /* frozen */)
                .AddReadySlot("я готова" /* name */, true /* frozen */)
                .AddPhrasesCountSlot(4 /* count */)
                .AddServerRepeatCountSlot(1 /* count */)
                .AddOptionalSlot("is_server_repeat" /* name */, true /* value */, "bool" /* type */)
                .Build();

            const TPassportAPI::TResult result{"guest_uid", "guest_code"};

            TVector<TDataSyncAPI::TKeyValue> expectedKVs;
            expectedKVs.emplace_back(EUserSpecificKey::UserName, TStringBuf("Alice"));
            expectedKVs.emplace_back(EUserSpecificKey::Gender, TStringBuf("female"));
            expectedKVs.emplace_back(EUserSpecificKey::GuestUID, TStringBuf("guest_uid"));

            TVoiceprintEnrollCollectVoiceWrapper wrapper(
                "uid", result, expectedKVs, {.BlackBox = 1, .Passport = 1, .DataSync = 1}, true /* dataSyncFail */);
            TContext::TPtr ctx = MakeAuthorizedContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }
}

Y_UNIT_TEST_SUITE(TWhatIsMyNameUnitTest) {
    Y_UNIT_TEST(Valid) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddOptionalSlot("is_known", true, "bool")
                    .AddOptionalSlot("user_name", "bob", "string")
                    .Build();
            TContext::TPtr ctx = MakeContext(request);
            TBlackboxDatasyncDependentHandlerWrapper<TWhatIsMyNameHandler> wrapper({.DataSync = 1});

            auto& dataSyncAPI = wrapper.GetDataSyncAPI();
            dataSyncAPI.Save(*ctx, "1", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("ann")}}});
            dataSyncAPI.Save(*ctx, "2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("bob")}}});
            dataSyncAPI.Save(*ctx, "3", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("carol")}}});

            wrapper.Do(*ctx);

            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoData) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();

            TContext::TPtr ctx = MakeContext(request);

            TBlackboxDatasyncDependentHandlerWrapper<TWhatIsMyNameHandler> wrapper({.DataSync = 1}, true /* fail */);
            wrapper.Do(*ctx);
        }
    }

    Y_UNIT_TEST(LowScores) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .SetLowBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddOptionalSlot("is_known", false, "bool")
                    .Build();

            RunAndCheckResponse<TWhatIsMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(LowScoresLimitUsers) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .AddExperiment("quasar_biometry_limit_users")
                    .SetLowBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddOptionalSlot("is_too_many_enrolled_users", true, "bool")
                    .AddOptionalSlot("is_known", false, "bool")
                    .Build();

            RunAndCheckResponse<TWhatIsMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(NoScoresLimitUsers) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .AddExperiment("quasar_biometry_limit_users")
                    .SetEmptyBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddOptionalSlot("is_known", false, "bool")
                    .Build();

            RunAndCheckResponse<TWhatIsMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(HighScoresLimitUsers) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .AddExperiment("quasar_biometry_limit_users")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddOptionalSlot("is_known", true, "bool")
                    .AddOptionalSlot("user_name", "bob", "string")
                    .Build();
            TContext::TPtr ctx = MakeContext(request);

            TBlackboxDatasyncDependentHandlerWrapper<TWhatIsMyNameHandler> wrapper({.DataSync = 1});

            auto& dataSyncAPI = wrapper.GetDataSyncAPI();
            dataSyncAPI.Save(*ctx, "2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("bob")}}});

            wrapper.Do(*ctx);

            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoBiometryKnown) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("what_is_my_name")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .SetForm("what_is_my_name")
                .AddOptionalSlot("is_known", true, "bool")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        TContext::TPtr ctx = MakeAuthorizedContext(request);

        TBlackboxDatasyncDependentHandlerWrapper<TWhatIsMyNameHandler> wrapper("1", {.BlackBox = 1, .DataSync = 1});

        auto& dataSyncAPI = wrapper.GetDataSyncAPI();
        dataSyncAPI.Save(*ctx, "1", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Eve")}}});

        wrapper.Do(*ctx);

        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(NoBiometryUnknown) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("what_is_my_name")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .SetForm("what_is_my_name")
                .AddOptionalSlot("is_known", false, "bool")
                .Build();
        RunAndCheckResponse<TWhatIsMyNameHandler>(request, response, {"1", {.BlackBox = 1, .DataSync = 1}});
    }

    Y_UNIT_TEST(EmptyScoresInvalidRegion) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("what_is_my_name")
                    .SetBelarusRegion()
                    .SetEmptyBiometricScores("456" /* requestId */, withMode)
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("what_is_my_name")
                    .AddAttentionBlock("invalid_region")
                    .AddOptionalSlot("is_known", false, "bool")
                    .Build();
            RunAndCheckResponse<TWhatIsMyNameHandler>(request, response, {});
        }
    }
}

Y_UNIT_TEST_SUITE(TSetMyNameUnitTest) {
    Y_UNIT_TEST(NoBiometryRename) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAnalyticsInfoBlock(
                    /* intent= */ "personal_assistant.scenarios.set_my_name",
                    /* productScenarioName= */ NAlice::NProductScenarios::IDENTITY_COMMANDS)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .AddOptionalSlot("old_user_name", "Alice", "string")
                .Build();
        TContext::TPtr ctx = MakeAuthorizedContext(request);

        TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper("1", {.BlackBox = 1, .DataSync = 1});

        auto& dataSyncAPI = wrapper.GetDataSyncAPI();
        dataSyncAPI.Save(*ctx, "1", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});

        wrapper.Do(*ctx);

        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(NoBiometryFirstName) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAnalyticsInfoBlock(
                    /* intent= */ "personal_assistant.scenarios.set_my_name",
                    /* productScenarioName= */ NAlice::NProductScenarios::IDENTITY_COMMANDS)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        RunAndCheckResponse<TSetMyNameHandler>(request, response, {"1", {.BlackBox = 1, .DataSync = 1}});
    }

    Y_UNIT_TEST(BiometryNoScores) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .SetEmptyBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .AddPlayerPauseBlock()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .AddOptionalSlot("user_name_frozen", "Eve", "string")
                    .AddOptionalSlot("is_need_explain", true, "bool")
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.voiceprint_enroll__collect_voice",
                        /* productScenarioName= */ "identity_commands")
                    .Build();

            RunAndCheckResponse<TSetMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(BiometryLowScores) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .SetLowBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .AddPlayerPauseBlock()
                    .SetForm("voiceprint_enroll__collect_voice")
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .AddOptionalSlot("user_name_frozen", "Eve", "string")
                    .AddOptionalSlot("is_need_explain", true, "bool")
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.voiceprint_enroll__collect_voice",
                        /* productScenarioName= */ "identity_commands")
                    .Build();
            RunAndCheckResponse<TSetMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(BiometryLowScoresTooManyUsers) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .AddExperiment("quasar_biometry_limit_users")
                    .SetLowBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddOptionalSlot("is_too_many_enrolled_users", true, "bool")
                    .AddOptionalSlot("is_change_name", true, "bool")
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.voiceprint_enroll__finish",
                        /* productScenarioName= */ "identity_commands")
                    .Build();

            RunAndCheckResponse<TSetMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(BiometryLowScoresNoAuth) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .SetLowBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddOptionalSlot("is_need_login", true, "bool")
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.voiceprint_enroll__finish",
                        /* productScenarioName= */ "identity_commands")
                    .Build();

            TContext::TPtr ctx = MakeContext(request);
            TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper;
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(BiometryValid) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .SetBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.set_my_name",
                        /* productScenarioName= */ NAlice::NProductScenarios::IDENTITY_COMMANDS)
                    .SetForm("set_my_name")
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .AddOptionalSlot("old_user_name", "Alice", "string")
                    .Build();
            TContext::TPtr ctx = MakeAuthorizedContext(request);

            TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper({.DataSync = 1});

            auto& dataSyncAPI = wrapper.GetDataSyncAPI();
            dataSyncAPI.Save(*ctx, "1", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Bob")}}});
            dataSyncAPI.Save(*ctx, "2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
            dataSyncAPI.Save(*ctx, "3", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Caroline")}}});

            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoBiometryBlackboxError) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        TContext::TPtr ctx = MakeAuthorizedContext(request);

        TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper({.BlackBox = 1}, true /* fail */);
        wrapper.Do(*ctx);
    }

    Y_UNIT_TEST(NoBiometryError) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        TContext::TPtr ctx = MakeAuthorizedContext(request);
        TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper(
                "1", {.BlackBox = 1, .DataSync = 1}, true /* fail */);

        auto& dataSyncAPI = wrapper.GetDataSyncAPI();
        dataSyncAPI.FailOnGet("1", EUserSpecificKey::UserName);

        wrapper.Do(*ctx);
    }

    Y_UNIT_TEST(NoBiometryUnauthorized) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAttentionBlock("what_is_my_name__silent_enroll_mode")
                .AddAnalyticsInfoBlock(
                    /* intent= */ "personal_assistant.scenarios.set_my_name",
                    /* productScenarioName= */ NAlice::NProductScenarios::IDENTITY_COMMANDS)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", "Eve", "string")
                .Build();
        TContext::TPtr ctx = MakeContext(request);

        TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper("authorized-id", {.DataSync = 1});

        auto& dataSyncAPI = wrapper.GetDataSyncAPI();
        dataSyncAPI.FailOnGet("authorized-id", EUserSpecificKey::UserName);
        dataSyncAPI.Save(*ctx, "uuid:some_uuid",
                         TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});

        wrapper.Do(*ctx);
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(InvalidParam) {
        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .AddOptionalSlot("user_name", true, "bool")
                .Build();

        TContext::TPtr ctx = MakeContext(request);

        TBlackboxDatasyncDependentHandlerWrapper<TSetMyNameHandler> wrapper({} /* expectedCalls */, true /* fail */);

        wrapper.Do(*ctx);
    }

    Y_UNIT_TEST(BiometryEmptyScoresInvalidRegion) {
        for (auto withMode : {true, false}) {
            auto request =
                TRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetForm("set_my_name")
                    .SetBelarusRegion()
                    .SetEmptyBiometricScores("456" /* requestId */, withMode)
                    .AddOptionalSlot("user_name", "Eve", "string")
                    .Build();
            auto response =
                TRequestResponseBuilder()
                    .SetForm("voiceprint_enroll__finish")
                    .AddSlots()
                    .AddAttentionBlock("invalid_region")
                    .AddAnalyticsInfoBlock(
                        /* intent= */ "personal_assistant.scenarios.voiceprint_enroll__finish",
                        /* productScenarioName= */ "identity_commands")
                    .Build();

            RunAndCheckResponse<TSetMyNameHandler>(request, response, {});
        }
    }

    Y_UNIT_TEST(BiometryDistractor) {

        auto request =
            TRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetForm("set_my_name")
                .SetBiometricScores("456" /* requestId */, true /* withMode */)
                .AddOptionalSlot("distractor", "моего", "string")
                .Build();
        auto response =
            TRequestResponseBuilder()
                .AddAnalyticsInfoBlock(
                    /* intent= */ "personal_assistant.scenarios.set_my_name",
                    /* productScenarioName= */ NAlice::NProductScenarios::IDENTITY_COMMANDS)
                .SetForm("set_my_name")
                .AddOptionalSlot("distractor", "моего", "string")
                .Build();

        TContext::TPtr ctx = MakeAuthorizedContext(request);

        RunAndCheckResponse<TSetMyNameHandler>(request, response, {});
    }
}
