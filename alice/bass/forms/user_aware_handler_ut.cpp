#include "user_aware_handler.h"

#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/ut/helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/system/yassert.h>
#include <util/thread/pool.h>

namespace NBASS {
namespace {

using NTestingHelpers::CheckResponse;
using NTestingHelpers::EqualJson;
using NTestingHelpers::MakeAuthorizedContext;
using NTestingHelpers::MakeContext;

using TKeyValue = TDataSyncAPI::TKeyValue;
using EUserSpecificKey = TPersonalDataHelper::EUserSpecificKey;

class THandlerFake : public IHandler {
public:
    THandlerFake(const TResultValue& result, bool useResponseForm)
        : NumCalls(0)
        , Result(result)
        , UseResponseForm(useResponseForm) {
    }

    TResultValue Do(TRequestHandler& r) override {
        ++NumCalls;
        if (UseResponseForm) {
            r.Ctx().SetResponseForm("another_form_name", false /* setCurrentFormAsCallback */);
        }
        return Result;
    }

    int GetNumCalls() const {
        return NumCalls;
    }

private:
    int NumCalls;
    TResultValue Result;
    bool UseResponseForm;
};

class TRequestResponseBuilder {
public:
    TRequestResponseBuilder() {
        result.SetDict();
    }

    TRequestResponseBuilder& SetForm(TStringBuf formName) {
        result["form"]["name"] = formName;
        result["form"]["slots"].SetArray();
        return *this;
    }

    TRequestResponseBuilder& SetFormSomeForm() {
        return SetForm("some_form_name");
    }

    TRequestResponseBuilder& SetFormAnotherForm() {
        return SetForm("another_form_name");
    }

    TRequestResponseBuilder& SetBasicMeta(bool expectBiometricsScores) {
        result["meta"]["epoch"] = 1484311159;
        result["meta"]["tz"] = "Europe/Moscow";
        result["meta"]["uuid"] = "some_uuid";
        if (expectBiometricsScores) {
            result["meta"]["client_id"] = "ru.yandex.quasar/7.90 (none; none none)";
        }
        return *this;
    }

    TRequestResponseBuilder& SetBiometricScores(bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.13, "user_id": "1"},
                            {"score": 0.6, "user_id": "2"},
                            {"score": 0.01, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tpr",
                        "scores": [
                            {"score": 0.15, "user_id": "1"},
                            {"score": 0.8, "user_id": "2"},
                            {"score": 0.02, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tnr",
                        "scores": [
                            {"score": 0.1, "user_id": "1"},
                            {"score": 0.5, "user_id": "2"},
                            {"score": 0.01, "user_id": "3"}
                        ]
                    }
                ]
            })");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "request_id": "456",
                    "scores": [
                        {"score": 0.225, "user_id": "1"},
                        {"score": 0.506, "user_id": "2"},
                        {"score": 0.03, "user_id": "3"}
                    ]
                }
            )");
        }
        return *this;
    }

    TRequestResponseBuilder& SetGuestBiometricScores(bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.13, "user_id": "1"},
                            {"score": 0.6, "user_id": "2"},
                            {"score": 0.01, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tpr",
                        "scores": [
                            {"score": 0.15, "user_id": "1"},
                            {"score": 0.8, "user_id": "2"},
                            {"score": 0.02, "user_id": "3"}
                        ]
                    },
                    {
                        "mode": "high_tnr",
                        "scores": [
                            {"score": 0.1, "user_id": "1"},
                            {"score": 0.3, "user_id": "2"},
                            {"score": 0.01, "user_id": "3"}
                        ]
                    }
                ]
            })");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "request_id": "456",
                    "scores": [
                        {"score": 0.05, "user_id": "1"},
                        {"score": 0.02, "user_id": "2"},
                        {"score": 0.03, "user_id": "3"}
                    ]
                }
            )");
        }
        return *this;
    }

    TRequestResponseBuilder& SetExperiment(TStringBuf experimentName) {
        auto& experiments = result["meta"]["experiments"];

        if (experiments.IsNull()) {
            experiments.SetArray();
        }
        experiments.Push(experimentName);
        return *this;
    }

    TRequestResponseBuilder& SetBasicExperiments() {
        SetExperiment(PERSONALIZATION_EXPERIMENT);
        SetExperiment(AUTO_INSERT_EXPERIMENT);
        return *this;
    }

    TRequestResponseBuilder& SetLastUserInfoTimestamp(i64 timestamp) {
        result["session_state"]["last_user_info_timestamp"] = timestamp;
        return *this;
    }

    TRequestResponseBuilder& SetMusicPersonalizationState(
            TMaybe<std::vector<i64>> timestamps, TMaybe<i64> count) {
        if (timestamps.Defined()) {
            auto& jsonTimestamps = result["session_state"]["user_info_music_pronounce_timestamps"];
            jsonTimestamps.SetArray();
            for (const auto& timestamp: *timestamps) {
                jsonTimestamps.Push(timestamp);
            }
        }
        if (count.Defined()) {
            result["session_state"]["user_info_no_pronounce_count"] = *count;
        }
        return *this;
    }

    TRequestResponseBuilder& SetName(bool isSilent) {
        NSc::TValue newBlock;
        newBlock["type"] = "user_info";
        newBlock["user_name"] = "Alice";
        newBlock["is_silent"].SetBool(isSilent);
        result["blocks"].Push(newBlock);
        return *this;
    }

    NSc::TValue Build() {
        return result;
    }

private:
    NSc::TValue result;
};

struct TDelegateStub : public TUserAwareHandler::TDelegate {
    TInstant GetTimestamp() override {
        ++NumCalled;
        if (NumCalled == 1) {
            return INITIAL_TIMESTAMP;
        }
        if (NumCalled == 2) {
            return SUBSEQUENT_TIMESTAMP;
        }
        UNIT_FAIL("More than two calls to TUserAwareHandler::TDelegate");
        return INITIAL_TIMESTAMP;
    }
    static constexpr TInstant INITIAL_TIMESTAMP = TInstant::MilliSeconds(1530272395576);
    static constexpr TInstant SUBSEQUENT_TIMESTAMP = INITIAL_TIMESTAMP + TDuration::Seconds(1);
    size_t NumCalled = 0;
};

constexpr TInstant TDelegateStub::INITIAL_TIMESTAMP;
constexpr TInstant TDelegateStub::SUBSEQUENT_TIMESTAMP;

struct TestRunner {
    void Run(const NSc::TValue& request, const NSc::TValue& response) {
        UNIT_ASSERT(!Tested);
        Tested = true;
        TContext::TPtr ctx = IsAuthorized ? MakeAuthorizedContext(request) : MakeContext(request);

        // ownership of slaveHandler will be taken by userAwareHandler
        THolder<THandlerFake> ownedSlaveHandler = MakeHolder<THandlerFake>(SlaveResult, UseResponseForm);

        // Since ownership of BlackBoxAPI, DataSyncAPI and ownedSlaveHandler
        // will be taken by userAwareHandler, we save copies of raw pointers for future checks
        auto* blackBoxAPI = BlackBoxAPI.Get();
        auto* dataSyncAPI = DataSyncAPI.Get();
        auto* slaveHandler = ownedSlaveHandler.Get();

        TUserAwareHandler userAwareHandler(std::move(ownedSlaveHandler),
                                           {NameDelay, BiometryThreshold, 0.0 /* PersonalizationDropProbabilty */,
                                            TDuration::MilliSeconds(0) /* PersonalizationAdditionalDataSyncTimeout */},
                                           MakeHolder<TDelegateStub>(),
                                           std::move(BlackBoxAPI),
                                           std::move(DataSyncAPI));
        TRequestHandler requestHandler(ctx);
        auto error = userAwareHandler.Do(requestHandler);
        UNIT_ASSERT_VALUES_EQUAL(error, SlaveResult);
        UNIT_ASSERT_VALUES_EQUAL(slaveHandler->GetNumCalls(), 1);
        UNIT_ASSERT_VALUES_EQUAL(dataSyncAPI->NumGetCalled, DataSyncGetCalls);
        UNIT_ASSERT_VALUES_EQUAL(blackBoxAPI->NumCalls, BlackBoxCalls);

        CheckResponse(*ctx, response);
    }

    ~TestRunner() {
        // If Tested == true then TUserAwareHandler was created and ownership
        // of BlackBoxAPI*, DataSyncAPI* was taken.
        UNIT_ASSERT(Tested);
    }

    bool IsAuthorized = false;
    bool UseResponseForm = false;
    size_t DataSyncGetCalls = 0;
    size_t BlackBoxCalls = 0;
    TResultValue SlaveResult = {};

    // Ownership of BlackBoxAPI and DataSyncAPI will be taken by UserAwareHandler inside Run method
    THolder<TBlackBoxAPIFake> BlackBoxAPI = MakeHolder<TBlackBoxAPIFake>();
    THolder<TDataSyncAPIStub> DataSyncAPI = MakeHolder<TDataSyncAPIStub>();

    TDuration NameDelay = TDuration::Hours(24);
    double BiometryThreshold = 0.9;

private:
    bool Tested = false;
};

} // namespace

Y_UNIT_TEST_SUITE(TUserAwareHandlerUnitTest) {
    Y_UNIT_TEST(TestUnauthorizedAttemptNotFound) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetFormSomeForm()
            .SetBasicMeta(false /* expectBiometricsScores */)
            .SetBasicExperiments()
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormSomeForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                                   .Build();
        TestRunner testRunner;
        testRunner.DataSyncGetCalls = 1;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestUnauthorizedAttemptFound) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetFormSomeForm()
            .SetBasicMeta(false /* expectBiometricsScores */)
            .SetBasicExperiments()
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormSomeForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                                   .SetName(false /* isSilent */)
                                   .Build();
        TestRunner testRunner;
        testRunner.DataSyncAPI->Save("uuid:some_uuid",
                                     TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestAuthorizedAttemptFound) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetFormSomeForm()
            .SetBasicMeta(false /* expectBiometricsScores */)
            .SetBasicExperiments()
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormSomeForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                                   .SetName(false /* isSilent */)
                                   .Build();
        TestRunner testRunner;
        testRunner.IsAuthorized = true;
        testRunner.DataSyncAPI->Save("some_uid",
                                     TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.BlackBoxAPI->UID = "some_uid";
        testRunner.DataSyncGetCalls = 1;
        testRunner.BlackBoxCalls = 1;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestBiometryAttemptFound) {
        for (auto withMode : {true, false}) {
            NSc::TValue request = TRequestResponseBuilder()
                .SetFormSomeForm()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetBasicExperiments()
                .SetBiometricScores(withMode)
                .Build();
            NSc::TValue response = TRequestResponseBuilder()
                .SetFormSomeForm()
                .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                .SetName(false /* isSilent */)
                .Build();
            TestRunner testRunner;
            testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
            testRunner.DataSyncGetCalls = 1;
            if (withMode) {
                // biometry threshold should not matter
                testRunner.BiometryThreshold = 1.0;
            } else {
                testRunner.BiometryThreshold = 0.5;
            }
            testRunner.Run(request, response);
        }
    }

    Y_UNIT_TEST(TestBiometryLowScores) {
        // only relevant for scores without mode
        for (auto withMode : {false}) {
            NSc::TValue request = TRequestResponseBuilder()
                .SetFormSomeForm()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetBasicExperiments()
                .SetBiometricScores(withMode)
                .Build();
            NSc::TValue response = TRequestResponseBuilder().SetFormSomeForm().Build();
            TestRunner testRunner;
            testRunner.BiometryThreshold = 0.9;
            testRunner.Run(request, response);
        }
    }

    Y_UNIT_TEST(TestBiometryGuest) {
        for (auto withMode : {true, false}) {
            NSc::TValue request = TRequestResponseBuilder()
                .SetFormSomeForm()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetBasicExperiments()
                .SetGuestBiometricScores(withMode)
                .Build();
            NSc::TValue response = TRequestResponseBuilder().SetFormSomeForm().Build();
            TestRunner testRunner;
            testRunner.BiometryThreshold = 0;
            testRunner.Run(request, response);
        }
    }

    Y_UNIT_TEST(TestPropagateError) {
        NSc::TValue request = TRequestResponseBuilder().SetFormSomeForm().SetBasicMeta(false /* expectBiometricsScores */).SetBasicExperiments().Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormSomeForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                                   .Build();
        TestRunner testRunner;
        testRunner.SlaveResult = TError{TError::EType::SYSTEM};
        testRunner.DataSyncGetCalls = 1;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestNameDelay) {
        i64 timestamp = (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Hours(1)).MilliSeconds();
        NSc::TValue request = TRequestResponseBuilder()
                                  .SetFormSomeForm()
                                  .SetBasicMeta(false /* expectBiometricsScores */)
                                  .SetBasicExperiments()
                                  .SetLastUserInfoTimestamp(timestamp)
                                  .Build();
        NSc::TValue response = TRequestResponseBuilder().SetFormSomeForm().SetLastUserInfoTimestamp(timestamp).Build();
        TestRunner testRunner;
        testRunner.DataSyncGetCalls = 0;
        testRunner.NameDelay = TDuration::Hours(24);
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestNameDelayRenew) {
        i64 timestamp = (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Hours(1)).MilliSeconds();
        NSc::TValue request = TRequestResponseBuilder()
                                  .SetFormSomeForm()
                                  .SetBasicMeta(false /* expectBiometricsScores */)
                                  .SetBasicExperiments()
                                  .SetLastUserInfoTimestamp(timestamp)
                                  .Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormSomeForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::SUBSEQUENT_TIMESTAMP.MilliSeconds())
                                   .Build();
        TestRunner testRunner;

        testRunner.DataSyncGetCalls = 1;
        testRunner.NameDelay = TDuration::Minutes(30);
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestResponseForm) {
        NSc::TValue request =
            TRequestResponseBuilder()
                .SetFormAnotherForm()
                .SetBasicMeta(false /* expectBiometricsScores */)
                .SetBasicExperiments()
                .Build();
        NSc::TValue response = TRequestResponseBuilder()
                                   .SetFormAnotherForm()
                                   .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
                                   .SetName(false /* isSilent */)
                                   .Build();
        TestRunner testRunner;
        testRunner.DataSyncAPI->Save("uuid:some_uuid",
                                     TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.UseResponseForm = true;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationPronounce) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            .SetBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
            .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds()}}, {0})
            .SetName(false /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationNoPronounceSmallCount) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            .SetMusicPersonalizationState(
                {{(TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(1)).MilliSeconds()}}, {0})
            .SetBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
            .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{(TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(1)).MilliSeconds()}}, {1})
            .SetName(true /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationPronounceSecondTime) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            .SetMusicPersonalizationState(
                {{(TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(10)).MilliSeconds()}}, {3})
            .SetBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
            .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{(TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(10)).MilliSeconds(),
                   TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds()}}, {0})
            .SetName(false /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationNoPronounceFullTimestamps) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            .SetMusicPersonalizationState(
                {{
                    (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(10)).MilliSeconds(),
                    (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(9)).MilliSeconds(),
                    (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(8)).MilliSeconds(),
                    (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(7)).MilliSeconds(),
                    (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(6)).MilliSeconds()
                 }}, {3})
            .SetBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
            // INITIAL_TIMESTAMP was used to check if pronounce is needed
            .SetLastUserInfoTimestamp(TDelegateStub::SUBSEQUENT_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(10)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(9)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(8)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(7)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(6)).MilliSeconds()
                 }}, {3})
            .SetName(true /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationPronounceFullTimestamps) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            .SetMusicPersonalizationState(
                {{
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(61)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(9)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(8)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(7)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(6)).MilliSeconds()
                 }}, {3})
            .SetBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
                // INITIAL_TIMESTAMP was used to check if pronounce is needed
            .SetLastUserInfoTimestamp(TDelegateStub::SUBSEQUENT_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(9)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(8)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(7)).MilliSeconds(),
                     (TDelegateStub::INITIAL_TIMESTAMP - TDuration::Minutes(6)).MilliSeconds(),
                     TDelegateStub::SUBSEQUENT_TIMESTAMP.MilliSeconds()
                 }}, {0})
            .SetName(false /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }

    Y_UNIT_TEST(TestMusicPersonalizationNotSoHighBiometry) {
        NSc::TValue request = TRequestResponseBuilder()
            .SetForm("personal_assistant.scenarios.music_play")
            .SetBasicMeta(true /* expectBiometricsScores */)
            .SetBasicExperiments()
            .SetExperiment("music_personalization")
            // Guest is only guest in high_tnr mode
            .SetGuestBiometricScores(true /* withMode */)
            .Build();
        NSc::TValue response = TRequestResponseBuilder()
            .SetFormAnotherForm()
            .SetLastUserInfoTimestamp(TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds())
            .SetMusicPersonalizationState(
                {{TDelegateStub::INITIAL_TIMESTAMP.MilliSeconds()}}, {0})
            .SetName(false /* isSilent */)
            .Build();
        TestRunner testRunner;
        testRunner.UseResponseForm = true;
        testRunner.DataSyncAPI->Save("2", TVector<TKeyValue>{{{EUserSpecificKey::UserName, TStringBuf("Alice")}}});
        testRunner.DataSyncGetCalls = 1;
        testRunner.BiometryThreshold = 1.0;
        testRunner.Run(request, response);
    }
}

} // namespace NBASS
