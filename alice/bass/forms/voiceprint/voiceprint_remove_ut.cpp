#include "voiceprint_remove.h"

#include <alice/bass/ut/helpers.h>

using namespace NBASS;
using namespace NTestingHelpers;

namespace {

class TRemoveRequestResponseBuilder {
public:
    TRemoveRequestResponseBuilder& SetBasicMeta(bool expectBiometricsScores) {
        result["meta"]["epoch"] = 1484311159;
        result["meta"]["tz"] = "Europe/Moscow";
        if (expectBiometricsScores) {
            result["meta"]["client_id"] = "ru.yandex.quasar/7.90 (none; none none)";
        }
        return *this;
    }

    TRemoveRequestResponseBuilder& SetBiometricScores(bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.2, "user_id": "1"},
                            {"score": 0.6, "user_id": "2"},
                            {"score": 0.02, "user_id": "3"}
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
        return *this;
    }

    TRemoveRequestResponseBuilder& SetLowBiometricScores(bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores_with_mode": [
                    {
                        "mode": "max_accuracy",
                        "scores": [
                            {"score": 0.025, "user_id": "1"},
                            {"score": 0.006, "user_id": "3"},
                            {"score": 0.03, "user_id": "2"}
                        ]
                    },
                    {
                        "mode": "high_tpr",
                        "scores": [
                            {"score": 0.03, "user_id": "1"},
                            {"score": 0.01, "user_id": "3"},
                            {"score": 0.07, "user_id": "2"}
                        ]
                    },
                    {
                        "mode": "high_tnr",
                        "scores": [
                            {"score": 0.01, "user_id": "1"},
                            {"score": 0.001, "user_id": "3"},
                            {"score": 0.02, "user_id": "2"}
                        ]
                    }
                ]
            })");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                "status": "ok",
                "scores": [
                    {"score": 0.025, "user_id": "1"},
                    {"score": 0.006, "user_id": "3"},
                    {"score": 0.03, "user_id": "2"}
                ]
            })");
        }
        return *this;
    }

    TRemoveRequestResponseBuilder& SetEmptyBiometricScores(bool withMode) {
        if (withMode) {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "scores_with_mode": [
                        {"mode": "max_accuracy", "scores": []},
                        {"mode": "high_tpr", "scores": []},
                        {"mode": "high_tnr", "scores": []},
                    ]
                }
            )");
        } else {
            result["meta"]["biometrics_scores"] = NSc::TValue::FromJson(R"({
                    "status": "ok",
                    "scores": []
                }
            )");
        }
        return *this;
    }

    TRemoveRequestResponseBuilder& SetRemoveForm() {
        result["form"]["name"] = "personal_assistant.scenarios.voiceprint_remove";
        return *this;
    }

    TRemoveRequestResponseBuilder& SetConfirmForm() {
        result["form"]["name"] = "personal_assistant.scenarios.voiceprint_remove__confirm";
        return *this;
    }

    TRemoveRequestResponseBuilder& SetFinishForm() {
        result["form"]["name"] = "personal_assistant.scenarios.voiceprint_remove__finish";
        return *this;
    }

    TRemoveRequestResponseBuilder& AddUserNameSlot(TStringBuf userName) {
        return AddOptionalSlot("user_name", userName, "string" /* type */);
    }

    TRemoveRequestResponseBuilder& AddUidSlot(TStringBuf uid) {
        return AddOptionalSlot("uid" /* name */, uid, "string" /* type */);
    }

    TRemoveRequestResponseBuilder& AddIsNoUsersSlot() {
        return AddOptionalSlot("is_no_users" /* name */, true, "bool" /* type */);
    }

    TRemoveRequestResponseBuilder& AddConfirmSlot(bool null = false) {
        NSc::TValue value;
        if (!null) {
            value.SetString("Confirm");
        }
        return AddOptionalSlot("confirm" /* name */, value, "string" /* type */);
    }

    TRemoveRequestResponseBuilder& AddIsRemovedSlot(bool value) {
        return AddOptionalSlot("is_removed" /* name */, value, "bool" /* type */);
    }

    TRemoveRequestResponseBuilder& AddOptionalSlot(TStringBuf name, const NSc::TValue& value, TStringBuf type) {
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

    TRemoveRequestResponseBuilder& AddRemoveAction(TStringBuf user_id) {
        NSc::TValue block;
        block["command_type"] = "remove_voiceprint";
        block["type"] = "uniproxy-action";
        block["data"]["user_id"] = user_id;
        return AddBlock(block);
    }

    TRemoveRequestResponseBuilder& AddErrorAttentionBlock(TStringBuf message) {
        NSc::TValue block;
        block["type"].SetString("error");
        block["data"] = NSc::TValue::Null();
        block["error"]["type"].SetString("system");
        block["error"]["msg"].SetString(message);
        return AddBlock(block);
    }

    TRemoveRequestResponseBuilder& AddAttentionBlock(TStringBuf attention_type) {
        NSc::TValue block;
        block["type"] = "attention";
        block["attention_type"] = attention_type;
        block["data"] = NSc::TValue();
        return AddBlock(block);
    }

    TRemoveRequestResponseBuilder& AddBlock(const NSc::TValue& block) {
        result["blocks"].Push(block);
        return *this;
    }

    TRemoveRequestResponseBuilder& AddSlots() {
        result["form"]["slots"].SetArray();
        return *this;
    }

    TRemoveRequestResponseBuilder& AddAnalyticsInfo() {
        NSc::TValue analyticsInfo;
        analyticsInfo["data"] = "EjZwZXJzb25hbF9hc3Npc3RhbnQuc2NlbmFyaW9zLnZvaWNlcHJpbnRfcmVtb3ZlX19maW5pc2hKEWlkZW50aXR5X2NvbW1hbmRz";
        analyticsInfo["type"] = "scenario_analytics_info";
        result["blocks"].Push(analyticsInfo);
        return *this;
    };

    NSc::TValue Build() const {
        return result;
    }

private:
    NSc::TValue result;
};
} // namespace

namespace NVoiceprintTesting {

class TVoiceprintRemoveWrapper final {
public:
    explicit TVoiceprintRemoveWrapper(size_t expectedDataSyncCalls = 0, bool fail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TDataSyncAPIStub>())
        , ExpectedDataSyncCalls(expectedDataSyncCalls)
        , Fail(fail) {
    }

    TVoiceprintRemoveWrapper(TStringBuf uid, TStringBuf name, size_t expectedDataSyncCalls = 0, bool fail = false)
        : Handler(MakeHolder<TBlackBoxAPIFake>(), MakeHolder<TDataSyncAPIStub>())
        , ExpectedDataSyncCalls(expectedDataSyncCalls)
        , Fail(fail) {
        static_cast<TDataSyncAPIStub*>(Handler.DataSyncAPI.Get())
            ->Save(uid, {{TPersonalDataHelper::EUserSpecificKey::UserName, name}});
    }

    ~TVoiceprintRemoveWrapper() {
        {
            TBlackBoxAPIFake* api = static_cast<TBlackBoxAPIFake*>(Handler.BlackBoxAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumCalls, 0);
        }

        {
            TDataSyncAPIStub* api = static_cast<TDataSyncAPIStub*>(Handler.DataSyncAPI.Get());
            UNIT_ASSERT_VALUES_EQUAL(api->NumGetCalled, ExpectedDataSyncCalls);
        }
    }

    void Do(TContext& ctx) {
        const auto error = Handler.DoImpl(ctx);
        UNIT_ASSERT_VALUES_EQUAL(error.Defined(), Fail);
    }

private:
    TVoiceprintRemoveHandler Handler;
    size_t ExpectedDataSyncCalls;
    bool Fail;
};

class TVoiceprintRemoveConfirmWrapper final {
public:
    explicit TVoiceprintRemoveConfirmWrapper(bool fail = false)
        : Handler()
        , Fail(fail) {
    }

    void Do(TContext& ctx) {
        const auto error = Handler.Do(ctx);
        UNIT_ASSERT_VALUES_EQUAL(error.Defined(), Fail);
    }

private:
    TVoiceprintRemoveConfirmHandler Handler;
    bool Fail;
};

} // namespace NVoiceprintTesting

using namespace NVoiceprintTesting;

Y_UNIT_TEST_SUITE(TVoiceprintRemoveTest) {
    Y_UNIT_TEST(NoScoresOnQuasarConfirm) {
        // Scores are expected, but not present
        auto request =
            TRemoveRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetConfirmForm()
                .Build();
        auto response =
            TRemoveRequestResponseBuilder()
                .SetFinishForm()
                .AddSlots()
                .AddAttentionBlock("server_error")
                .AddAnalyticsInfo()
                .Build();

        TVoiceprintRemoveConfirmWrapper wrapper(false /* fail */);
        TContext::TPtr ctx = MakeContext(request);
        wrapper.Do(*ctx);
        NSc::TValue actual;
        ctx->ToJson(&actual);
        Cerr << actual.ToJson() << Endl;
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(EmptyScoresOnPhoneConfirm) {
        // Scores are not expected, but present
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(false /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetEmptyBiometricScores(withMode)
                    .Build();
            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddSlots()
                    .AddAttentionBlock("server_error")
                    .AddAnalyticsInfo()
                    .Build();

            TVoiceprintRemoveConfirmWrapper wrapper(false /* fail */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FilledScoresOnPhoneConfirm) {
        // Scores are not expected, but present
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(false /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetLowBiometricScores(withMode)
                    .Build();
            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddSlots()
                    .AddAttentionBlock("server_error")
                    .AddAnalyticsInfo()
                    .Build();

            TVoiceprintRemoveConfirmWrapper wrapper(false /* fail */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoScoresOnQuasarRemove) {
        // Scores are expected, but not present
        auto request =
            TRemoveRequestResponseBuilder()
                .SetBasicMeta(true /* expectBiometricsScores */)
                .SetRemoveForm()
                .Build();
        auto response =
            TRemoveRequestResponseBuilder()
                .SetFinishForm()
                .AddSlots()
                .AddAttentionBlock("server_error")
                .Build();

        TVoiceprintRemoveWrapper wrapper(0, false /* fail */);
        TContext::TPtr ctx = MakeContext(request);
        wrapper.Do(*ctx);
        CheckResponse(*ctx, response);
    }

    Y_UNIT_TEST(EmptyScoresOnPhoneRemove) {
        // Scores are not expected, but present
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(false /* expectBiometricsScores */)
                    .SetEmptyBiometricScores(withMode)
                    .SetRemoveForm()
                    .Build();
            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddSlots()
                    .AddAttentionBlock("server_error")
                    .Build();

            TVoiceprintRemoveWrapper wrapper(0, false /* true */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(FilledScoresOnPhoneRemove) {
        // Scores are not expected, but present
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(false /* expectBiometricsScores */)
                    .SetBiometricScores(withMode)
                    .SetRemoveForm()
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddSlots()
                    .AddAttentionBlock("server_error")
                    .Build();

            TVoiceprintRemoveWrapper wrapper(0, false /* fail */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(EmptyScores) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetRemoveForm()
                    .SetEmptyBiometricScores(withMode)
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddIsNoUsersSlot()
                    .Build();

            TVoiceprintRemoveWrapper wrapper;
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(LowScores) {
        for (auto withMode : {false, true}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetRemoveForm()
                    .SetLowBiometricScores(withMode)
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetConfirmForm()
                    .AddUidSlot("2")
                    .AddUserNameSlot("Eve")
                    .AddAttentionBlock("biometry_guest")
                    .Build();

            TVoiceprintRemoveWrapper wrapper("2" /* uid */, "Eve" /* name */, 1 /* expectedDataSyncCalls */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(HighScores) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetRemoveForm()
                    .SetBiometricScores(withMode)
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetConfirmForm()
                    .AddUidSlot("2")
                    .AddUserNameSlot("Eve")
                    .Build();

            TVoiceprintRemoveWrapper wrapper("2" /* uid */, "Eve" /* name */, 1 /* expectedDataSyncCalls */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(Confirm) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetBiometricScores(withMode)
                    .AddUidSlot("2")
                    .AddUserNameSlot("Eve")
                    .AddConfirmSlot()
                    .Build();
            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddRemoveAction("2")
                    .AddIsRemovedSlot(true)
                    .AddAnalyticsInfo()
                    .Build();
            TVoiceprintRemoveConfirmWrapper wrapper;
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(NoConfirm) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetBiometricScores(withMode)
                    .AddUidSlot("2")
                    .AddUserNameSlot("Eve")
                    .Build();
            auto response = TRemoveRequestResponseBuilder()
                .SetFinishForm()
                .AddIsRemovedSlot(false)
                .AddAnalyticsInfo()
                .Build();
            TVoiceprintRemoveConfirmWrapper wrapper;
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(DataSyncError) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetRemoveForm()
                    .SetBiometricScores(withMode)
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetConfirmForm()
                    .AddUidSlot("2")
                    .Build();
            TVoiceprintRemoveWrapper wrapper(1 /* expectedDataSyncCalls */, false /* fail */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(DataSyncErrorConfirm) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetBiometricScores(withMode)
                    .AddUidSlot("2")
                    .AddConfirmSlot()
                    .Build();
            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddRemoveAction("2")
                    .AddIsRemovedSlot(true)
                    .AddAnalyticsInfo()
                    .Build();
            TVoiceprintRemoveConfirmWrapper wrapper;
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(EmptyUidSlotConfirm) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetBiometricScores(withMode)
                    .AddConfirmSlot()
                    .Build();

            auto response =
                TRemoveRequestResponseBuilder()
                    .SetFinishForm()
                    .AddAttentionBlock("server_error")
                    .AddSlots()
                    .AddAnalyticsInfo()
                    .Build();
            TVoiceprintRemoveConfirmWrapper wrapper;
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
            CheckResponse(*ctx, response);
        }
    }

    Y_UNIT_TEST(InvalidParam) {
        for (auto withMode : {true, false}) {
            auto request =
                TRemoveRequestResponseBuilder()
                    .SetBasicMeta(true /* expectBiometricsScores */)
                    .SetConfirmForm()
                    .SetBiometricScores(withMode)
                    .AddOptionalSlot("uid", 12, "int")
                    .AddUserNameSlot("Eve")
                    .Build();
            TVoiceprintRemoveConfirmWrapper wrapper(true /* fail */);
            TContext::TPtr ctx = MakeContext(request);
            wrapper.Do(*ctx);
        }
    }
}
