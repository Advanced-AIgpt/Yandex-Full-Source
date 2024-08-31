#include <alice/library/biometry/biometry.h>
#include <alice/library/json/json.h>
#include <alice/library/unittest/ut_helpers.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NTestingHelpers;

namespace NAlice::NBiometry {

namespace {

class TBiometryDelegateStub final : public TBiometry::IDelegate {
public:
    TBiometryDelegateStub(TMaybe<TString> blackBoxUserId, TMaybe<TString> userId, TMaybe<TString> guestId)
    : BlackBoxUserId(blackBoxUserId)
    , UserId(userId)
    , GuestId(guestId) {
    }

    TMaybe<TString> GetUid() const override {
        return BlackBoxUserId;
    }

    NAlice::NBiometry::TResult GetUserName(TStringBuf /*userId*/, TString& userName) const override {
        userName = "Vasya";
        return {};
    }

    NAlice::NBiometry::TResult GetGuestId(TStringBuf userId, TString& guestId) const override {
        if (UserId.Defined() && GuestId.Defined() && userId == *UserId) {
            guestId = *GuestId;
            return {};
        }
        return NAlice::NBiometry::TError{NAlice::NBiometry::EErrorType::Logic};
    }

private:
    TMaybe<TString> BlackBoxUserId;
    TMaybe<TString> UserId;
    TMaybe<TString> GuestId;
};

class TRequestBuilder {
public:
    TRequestBuilder& AddBiometricsScores() {
        result["meta"]["biometrics_scores"]["status"] = "ok";
        result["meta"]["biometrics_scores"]["request_id"] = "12345";
        return *this;
    }

    TRequestBuilder& AddBiometricsScoresScores(bool empty) {
        if (empty) {
            result["meta"]["biometrics_scores"]["scores"] = NSc::TValue::FromJson(R"([])");
        } else {
            result["meta"]["biometrics_scores"]["scores"] = NSc::TValue::FromJson(R"([
                {"score": 0.3, "user_id": "2"},
                {"score": 0.1, "user_id": "1"}
            ])");
        }
        return *this;
    }

    TRequestBuilder& AddBiometricsScoresWithMode(TStringBuf mode, bool empty) {
        auto& scoresWithMode = result["meta"]["biometrics_scores"]["scores_with_mode"];
        if (!scoresWithMode.IsArray()) {
            scoresWithMode.SetArray();
        }
        auto newScores = NSc::TValue::FromJson(R"([
                "scores": []
            ])");
        newScores["mode"] = mode;
        if (!empty) {
            newScores["scores"] = NSc::TValue::FromJson(R"([
                {"score": 0.3, "user_id": "2"},
                {"score": 0.1, "user_id": "1"}
            ])");
        }
        scoresWithMode.Push(newScores);
        return *this;
    }

    NSc::TValue Build() const {
        return result;
    }

private:
    NSc::TValue result;
};

Y_UNIT_TEST_SUITE(HasNonEmptyBiometricsScoresUnitTest) {
Y_UNIT_TEST(HasAllScores) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScores()
        .AddBiometricsScoresScores(false /* empty */)
        .AddBiometricsScoresWithMode("max_accuracy", false /* empty */)
        .AddBiometricsScoresWithMode("high_tpr", false /* empty */)
        .AddBiometricsScoresWithMode("high_tnr", false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasBiometricsScores(meta));
    UNIT_ASSERT(HasNonEmptyBiometricsScores(meta));
}

Y_UNIT_TEST(HasScores) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScores()
        .AddBiometricsScoresScores(false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasNonEmptyBiometricsScores(meta));
}
Y_UNIT_TEST(HasScoresWithMode) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScoresWithMode("max_accuracy", false /* empty */)
        .AddBiometricsScoresWithMode("high_tpr", false /* empty */)
        .AddBiometricsScoresWithMode("high_tnr", false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasBiometricsScores(meta));
    UNIT_ASSERT(HasNonEmptyBiometricsScores(meta));
}
Y_UNIT_TEST(EmptyScoresWithMode) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScores()
        .AddBiometricsScoresScores(false /* empty */)
        .AddBiometricsScoresWithMode("max_accuracy", false /* empty */)
        .AddBiometricsScoresWithMode("high_tpr", true /* empty */)
        .AddBiometricsScoresWithMode("high_tnr", false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasBiometricsScores(meta));
    UNIT_ASSERT(!HasNonEmptyBiometricsScores(meta));
}
Y_UNIT_TEST(NotAllModesAvailible) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScores()
        .AddBiometricsScoresScores(false /* empty */)
        .AddBiometricsScoresWithMode("max_accuracy", false /* empty */)
        .AddBiometricsScoresWithMode("high_tnr", false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasBiometricsScores(meta));
    UNIT_ASSERT(!HasNonEmptyBiometricsScores(meta));
}
Y_UNIT_TEST(ExtraModesAvailible) {
    auto requestJson = TRequestBuilder()
        .AddBiometricsScoresWithMode("max_accuracy", false /* empty */)
        .AddBiometricsScoresWithMode("eer", false /* empty */)
        .AddBiometricsScoresWithMode("high_tpr", false /* empty */)
        .AddBiometricsScoresWithMode("high_tnr", false /* empty */)
        .Build();

    TBassMetaConstScheme meta(&requestJson["meta"]);
    UNIT_ASSERT(HasBiometricsScores(meta));
    UNIT_ASSERT(HasNonEmptyBiometricsScores(meta));
}
}

Y_UNIT_TEST_SUITE(TBiometryTest) {

TBiometryScoring ProtoFromJson(const TStringBuf jsonStr) {
    NJson::TJsonValue json;
    NJson::ReadJsonTree(jsonStr, &json, /* throwOnError = */true);
    return NAlice::JsonToProto<TBiometryScoring>(json);
}

template<typename TData>
void DoTestDifferentModes(const TData& data) {
    TBiometryDelegateStub delegate{/* blackBoxUserId = */ "12345", /* userId = */ "12345", /* guestId = */ "3"};
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::MaxAccuracy);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(biometry.IsGuestUser());
        UNIT_ASSERT(biometry.GetBestScore().Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(*biometry.GetBestScore(), 0.6, 0.01);
        TString userId;
        UNIT_ASSERT(!biometry.GetUserId(userId).Defined());
        UNIT_ASSERT_VALUES_EQUAL(userId, "3");
    }
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::HighTNR);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(biometry.IsGuestUser());
        UNIT_ASSERT(biometry.GetBestScore().Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(*biometry.GetBestScore(), 0.8, 0.01);
        TString userId;
        UNIT_ASSERT(!biometry.GetUserId(userId));
        UNIT_ASSERT_VALUES_EQUAL(userId, "3");
    }
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::HighTPR);
        UNIT_ASSERT(biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(biometry.GetBestScore().Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(*biometry.GetBestScore(), 0.61, 0.01);
        TString userId;
        UNIT_ASSERT(!biometry.GetUserId(userId));
        UNIT_ASSERT_VALUES_EQUAL(userId, "2");
    }
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::NoGuest);
        UNIT_ASSERT(biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(biometry.GetBestScore().Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(*biometry.GetBestScore(), 0.753, 0.01);
        TString userId;
        UNIT_ASSERT(!biometry.GetUserId(userId));
        UNIT_ASSERT_VALUES_EQUAL(userId, "2");
    }
}

Y_UNIT_TEST(TestDifferentModes) {
    auto json = NSc::TValue::FromJson(R"-({
            "meta": {
                "epoch": 1484311159,
                "tz": "Europe/Moscow",
                "client_id": "ru.yandex.quasar/7.90 (none; none none)",
                "biometrics_scores": {
                    "status": "ok",
                    "request_id": "12345",
                    "scores_with_mode": [
                        {
                            "mode": "max_accuracy",
                            "scores": [
                                {"score": 0.3, "user_id": "2"},
                                {"score": 0.1, "user_id": "1"}
                            ]
                        },
                        {
                            "mode": "high_tpr",
                            "scores": [
                                {"score": 0.61, "user_id": "2"},
                                {"score": 0.2, "user_id": "1"}
                            ]
                        },
                        {
                            "mode": "high_tnr",
                            "scores": [
                                {"score": 0.15, "user_id": "2"},
                                {"score": 0.05, "user_id": "1"}
                            ]
                        }
                    ]
                }
            }
        })-");
    TBassMetaConstScheme meta(&json["meta"]);
    DoTestDifferentModes(meta);
}

Y_UNIT_TEST(TestDifferentModesProto) {
    const auto proto = ProtoFromJson(R"-({
            "status": "ok",
            "request_id": "12345",
            "scores_with_mode": [
                {
                    "mode": "max_accuracy",
                    "scores": [
                        {"score": 0.3, "user_id": "2"},
                        {"score": 0.1, "user_id": "1"}
                    ]
                },
                {
                    "mode": "high_tpr",
                    "scores": [
                        {"score": 0.61, "user_id": "2"},
                        {"score": 0.2, "user_id": "1"}
                    ]
                },
                {
                    "mode": "high_tnr",
                    "scores": [
                        {"score": 0.15, "user_id": "2"},
                        {"score": 0.05, "user_id": "1"}
                    ]
                }
            ]
        })-");
    DoTestDifferentModes(proto);
}

template<typename TData>
void DoTestDifferentModesEmptyNoModes(const TData& data) {
    TBiometryDelegateStub delegate{/* blackBoxUserId = */ {}, /* userId = */ {}, /* guestId = */ {}};
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::MaxAccuracy);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::NoGuest);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
}

Y_UNIT_TEST(TestDifferentModesEmptyNoModes) {
    auto json = NSc::TValue::FromJson(R"-({
            "meta": {
                "epoch": 1484311159,
                "tz": "Europe/Moscow",
                "client_id": "ru.yandex.quasar/7.90 (none; none none)",
                "biometrics_scores": {
                    "status": "ok",
                    "request_id": "12345",
                    "scores_with_mode": []
                }
            }
        })-");
    TBassMetaConstScheme meta(&json["meta"]);
    DoTestDifferentModesEmptyNoModes(meta);
}

Y_UNIT_TEST(TestDifferentModesEmptyNoModesProto) {
    const auto proto = ProtoFromJson(R"-({
            "status": "ok",
                "request_id": "12345",
                "scores_with_mode": []
        })-");
    DoTestDifferentModesEmptyNoModes(proto);
}

Y_UNIT_TEST(TestNoBiometricScoresAtAllProto) {
    TBiometryDelegateStub delegate{/* blackBoxUserId = */ {}, /* userId = */ {}, /* guestId = */ {}};
    {
        auto biometry = TBiometry(delegate, TBiometry::EMode::MaxAccuracy);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
    {
        auto biometry = TBiometry(delegate, TBiometry::EMode::NoGuest);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
}

template<typename TData>
void DoTestDifferentModesEmptyWithModes(const TData& data) {
    TBiometryDelegateStub delegate{/* blackBoxUserId = */ {}, /* userId = */ {}, /* guestId = */ {}};
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::MaxAccuracy);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::NoGuest);
        UNIT_ASSERT(!biometry.IsKnownUser());
        UNIT_ASSERT(!biometry.IsGuestUser());
        UNIT_ASSERT(!biometry.GetBestScore().Defined());
    }
}

Y_UNIT_TEST(TestDifferentModesEmptyWithModes) {
    auto json = NSc::TValue::FromJson(R"-({
            "meta": {
                "epoch": 1484311159,
                "tz": "Europe/Moscow",
                "client_id": "ru.yandex.quasar/7.90 (none; none none)",
                "biometrics_scores": {
                    "status": "ok",
                    "request_id": "12345",
                    "scores_with_mode": [
                        {
                            "mode": "max_accuracy",
                            "scores": []
                        },
                        {
                            "mode": "high_tpr",
                            "scores": []
                        },
                        {
                            "mode": "high_tnr",
                            "scores": []
                        }
                    ]
                }
            }
        })-");
    TBassMetaConstScheme meta(&json["meta"]);
    DoTestDifferentModesEmptyWithModes(meta);
}

Y_UNIT_TEST(TestDifferentModesEmptyWithModesProto) {
    const auto proto = ProtoFromJson(R"-({
            "status": "ok",
            "request_id": "12345",
            "scores_with_mode": [
                {
                    "mode": "max_accuracy",
                    "scores": []
                },
                {
                    "mode": "high_tpr",
                    "scores": []
                },
                {
                    "mode": "high_tnr",
                    "scores": []
                }
            ]
        })-");
    DoTestDifferentModesEmptyWithModes(proto);
}

template<typename TData>
void DoTestNoGuest(const TData& data) {
    TBiometryDelegateStub delegate{/* blackBoxUserId = */ "12345", /* userId = */ "12345", /* guestId = */ "3"};
    {
        auto biometry = TBiometry(data, delegate, TBiometry::EMode::NoGuest);
        UNIT_ASSERT(biometry.IsKnownUser());
        UNIT_ASSERT(biometry.GetBestScore().Defined());
        UNIT_ASSERT_DOUBLES_EQUAL(*biometry.GetBestScore(), 0.3 / (0.3 + 0.1), 0.01);
        TString userId;
        UNIT_ASSERT(!biometry.GetUserId(userId));
        UNIT_ASSERT_VALUES_EQUAL(userId, "2");
    }
}

Y_UNIT_TEST(TestNoGuest) {
    auto json = NSc::TValue::FromJson(R"-({
            "meta": {
                "epoch": 1484311159,
                "tz": "Europe/Moscow",
                "client_id": "ru.yandex.quasar/7.90 (none; none none)",
                "biometrics_scores": {
                    "status": "ok",
                    "request_id": "12345",
                    "scores": [
                        {"score": 0.3, "user_id": "2"},
                        {"score": 0.1, "user_id": "1"}
                    ]
                }
            }
        })-");
    TBassMetaConstScheme meta(&json["meta"]);
    DoTestNoGuest(meta);
}

Y_UNIT_TEST(TestNoGuestProto) {
    const auto proto = ProtoFromJson(R"-({
            "status": "ok",
            "request_id": "12345",
            "scores": [
                {"score": 0.3, "user_id": "2"},
                {"score": 0.1, "user_id": "1"}
            ]
        })-");
    DoTestNoGuest(proto);
}

Y_UNIT_TEST(TestConvertBiometricsScoresScoresWithModeAndScores) {
    const auto biometricsScoresStr = R"-({
            "status": "ok",
            "request_id": "12345",
            "scores_with_mode": [
                {
                    "mode": "max_accuracy",
                    "scores": [{"score": 0.1, "user_id": "1"}]
                },
                {
                    "mode": "high_tpr",
                    "scores": [{"score": 0.2, "user_id": "2"}]
                }
            ],
            "scores": [{"score": 0.3, "user_id": "3"}]
        })-";
    const auto proto = ProtoFromJson(biometricsScoresStr);
    const auto meta = NImpl::ConvertBiometricsScores(proto);

    auto json = NSc::TValue::FromJson(TStringBuilder{} << "{\"biometrics_scores\": " << biometricsScoresStr << "}");
    UNIT_ASSERT(EqualJson(*meta->GetRawValue(), json));
}

Y_UNIT_TEST(TestConvertBiometricsScoresNoScoresNoScoresWithMode) {
    const auto biometricsScoresStr = R"-({
            "status": "ok",
            "request_id": "12345"
        })-";
    const auto proto = ProtoFromJson(biometricsScoresStr);
    const auto meta = NImpl::ConvertBiometricsScores(proto);

    auto json = NSc::TValue::FromJson(TStringBuilder{} << "{\"biometrics_scores\": " << biometricsScoresStr << "}");
    UNIT_ASSERT(EqualJson(*meta->GetRawValue(), json));
}

Y_UNIT_TEST(TestConvertBiometricsScoresEmptyScoresWithMode) {
    const auto biometricsScoresStr = R"-({
            "status": "ok",
            "request_id": "12345",
            "scores_with_mode": [
                {
                    "mode": "max_accuracy"
                },
                {
                    "mode": "high_tpr"
                },
                {
                    "mode": "high_tnr"
                }
            ]
        })-";
    const auto proto = ProtoFromJson(biometricsScoresStr);
    const auto meta = NImpl::ConvertBiometricsScores(proto);

    auto json = NSc::TValue::FromJson(TStringBuilder{} << "{\"biometrics_scores\": " << biometricsScoresStr << "}");
    UNIT_ASSERT(EqualJson(*meta->GetRawValue(), json));
}

Y_UNIT_TEST(TestConvertBiometricsScoresNoScoresWithMode) {
    const auto biometricsScoresStr = R"-({
            "status": "ok",
            "request_id": "12345",
            "scores": [
                {"score": 0.3, "user_id": "2"},
                {"score": 0.1, "user_id": "1"}
            ]
        })-";
    const auto proto = ProtoFromJson(biometricsScoresStr);
    const auto meta = NImpl::ConvertBiometricsScores(proto);

    auto json = NSc::TValue::FromJson(TStringBuilder{} << "{\"biometrics_scores\": " << biometricsScoresStr << "}");
    UNIT_ASSERT(EqualJson(*meta->GetRawValue(), json));
}

Y_UNIT_TEST(TestCreateEmptyMetaWithoutBiometricScores) {
    const auto meta = NImpl::CreateEmptyMetaWithoutBiometricScores();
    auto json = NSc::TValue::FromJson("{}");
    UNIT_ASSERT(EqualJson(*meta->GetRawValue(), json));
}

} // Y_UNIT_TEST_SUITE(TBiometryTest)

} // namespace
} // namespace namespace NAlice::NBiometry
