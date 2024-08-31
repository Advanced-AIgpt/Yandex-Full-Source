#include "context.h"

#include <alice/bass/ut/helpers.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/memento/proto/api.pb.h>
#include <alice/protos/data/scenario/reminders/state.pb.h>

#include <alice/library/unittest/ut_helpers.h>
#include <alice/library/proto/protobuf.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/scheme/scheme.h>


using namespace NBASS;

namespace {

const TString FORM_NAME = "personal_assistant.scenarios.find_poi_address";

const NSc::TValue VALID_REQUEST = NSc::TValue::FromJson(R"(
{
    "form": {
        "name": "personal_assistant.scenarios.find_poi_address",
        "slots": [
            {
                "name": "what",
                "type": "string",
                "value": "рестораны",
                "optional": true
            },
            {
                "name": "where",
                "type": "string",
                "value": "москва",
                "optional": true
            }
        ]
    },
    "meta": {
        "epoch":1484311159,
        "tz": "Europe/Moscow"
    }
})");

const NSc::TValue VALID_REQUEST_WITH_DATA_SOURCES = NSc::TValue::FromJson(R"(
{
    "form": {
        "name": "personal_assistant.scenarios.find_poi_address",
        "slots": [
            {
                "name": "what",
                "type": "string",
                "value": "рестораны",
                "optional": true
            },
            {
                "name": "where",
                "type": "string",
                "value": "москва",
                "optional": true
            }
        ]
    },
    "meta": {
        "epoch":1484311159,
        "tz": "Europe/Moscow"
    },
    "data_sources": {
        "2": {
            "user_info": {
                "uid": "12345"
            }
        },
        "6": {
            "begemot_external_markup": {
                "Porn": true
            }
        }
    }
})");

class TContextFixture : public NUnitTest::TBaseFixture {
private:
    TGlobalContextPtr GlobalCtx;

public:
    struct TCtx {
        TCtx(const NSc::TValue& request, TGlobalContextPtr globalCtx) {
            TContext::TInitializer init{globalCtx, "reqid" /* reqid */, {} /* authToken */,
                                        {} /* appInfoHeader */, {} /* fakeTimeHeader */, {} /* speechKitEvent */,
                                        {} /* userTicketHeader */};
            Result = TContext::FromJson(request, init, &Context);
        }

        TResultValue Result;
        TContext::TPtr Context;
    };

public:
    TContextFixture()
        : GlobalCtx(IGlobalContext::MakePtr<NTestingHelpers::TTestGlobalContext>())
    {
    }

    TCtx MakeCtx(const NSc::TValue& request) {
        return TCtx(request, GlobalCtx);
    }
};


using namespace ru::yandex::alice::memento::proto;
template <typename T>
TConfigKeyAnyPair CreateUserConfigs(EConfigKey key, const T& proto) {
    TConfigKeyAnyPair configPair;
    configPair.SetKey(key);

    ::google::protobuf::Any protoAny;
    protoAny.PackFrom(proto);
    *configPair.MutableValue() = protoAny;

    return configPair;
}

Y_UNIT_TEST_SUITE_F(BassContextUnitTest, TContextFixture) {
    Y_UNIT_TEST(InvalidRequest) {
        {
            TCtx ctx = MakeCtx(NSc::Null());
            UNIT_ASSERT(ctx.Result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(ctx.Result->Type, TError::EType::INVALIDPARAM);
            UNIT_ASSERT(!ctx.Context);
        }

        {
            TCtx ctx = MakeCtx(NSc::TValue::FromJson("{}"));
            UNIT_ASSERT(ctx.Result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(ctx.Result->Type, TError::EType::INVALIDPARAM);
            UNIT_ASSERT(!ctx.Context);
        }

        {
            NSc::TValue invalidSlot = VALID_REQUEST;
            invalidSlot["form"]["slots"][0].Delete("type");
            invalidSlot["form"]["slots"][1].Delete("name");
            TCtx ctx = MakeCtx(invalidSlot);
            UNIT_ASSERT(ctx.Result.Defined());
            UNIT_ASSERT_VALUES_EQUAL(ctx.Result->Type, TError::EType::INVALIDPARAM);
            UNIT_ASSERT(!ctx.Context);
            NSc::TValue out;
            ctx.Result->ToJson(out);
            UNIT_ASSERT_VALUES_EQUAL(out.ToJson(NSc::TValue::EJsonOpts::JO_SORT_KEYS), R"({"error":{"msg":"/form/slots/0/type: is a required field and is not found; /form/slots/1/name: is a required field and is not found","type":"invalidparam"}})");
        }
    }

    Y_UNIT_TEST(ReadValidRequest) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        UNIT_ASSERT(!ctx.Result.Defined());
        UNIT_ASSERT(ctx.Context);
    }

    Y_UNIT_TEST(Modifications) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        UNIT_ASSERT(ctx.Context);

        NSc::TValue out;
        TContext::TSlot* slot = ctx.Context->CreateSlot("creation-test-slot-1", "string", false /* optional */, "test-value");
        UNIT_ASSERT(slot);
        UNIT_ASSERT(slot->ToJson(&out, nullptr));
        UNIT_ASSERT_VALUES_EQUAL(out.ToJson(NSc::TValue::EJsonOpts::JO_SORT_KEYS), R"({"name":"creation-test-slot-1","optional":false,"type":"string","value":"test-value"})");
    }

    Y_UNIT_TEST(ValidActionRequest) {
        const NSc::TValue validActionRequest = NSc::TValue::FromJson(R"({"action": {"data": {"reason" : "pleeeease!", "severity" : "major"}, "name": "quasar.play_next_track"}, "meta": {"epoch":1484311159,"tz": "Europe/Moscow"}})");
        const NSc::TValue validActionResponse = NSc::TValue::FromJson(R"({"blocks":[{"attention_type":"wow-wow-wow","data":null,"type":"attention"}]})");
        const NSc::TValue validAnalyticsResponse = NSc::TValue::FromJson(R"({"blocks":[{"web_responses": {"provider":"important_data"},"type":"analytics_info"}]})");

        UNIT_ASSERT(!validActionRequest.IsNull());
        UNIT_ASSERT(!validActionResponse.IsNull());
        UNIT_ASSERT(!validAnalyticsResponse.IsNull());

        TCtx ctx = MakeCtx(validActionRequest);
        UNIT_ASSERT(ctx.Context);
        ctx.Context->AddAttention("wow-wow-wow");

        NSc::TValue out;
        ctx.Context->ToJson(&out);

        constexpr auto opts = NSc::TValue::EJsonOpts::JO_SORT_KEYS;
        UNIT_ASSERT_VALUES_EQUAL(out.ToJson(opts), validActionResponse.ToJson(opts));
    }

    Y_UNIT_TEST(ExperimentFlagsList) {
        NSc::TValue listExperiments = VALID_REQUEST.Clone();
        listExperiments["meta"]["experiments"][0].SetString("super_flag");

        TCtx ctx = MakeCtx(listExperiments);
        UNIT_ASSERT(ctx.Context);

        UNIT_ASSERT(ctx.Context->HasExpFlag("super_flag"));
        UNIT_ASSERT(ctx.Context->ExpFlag("super_flag").Defined());
        UNIT_ASSERT_VALUES_EQUAL(*ctx.Context->ExpFlag("super_flag"), "");
    }

    Y_UNIT_TEST(ExperimentFlagsDict) {
        NSc::TValue dictExperiments = VALID_REQUEST.Clone();
        dictExperiments["meta"]["experiments"]["super_flag"].SetString("super_value");
        dictExperiments["meta"]["experiments"]["duper_flag"].SetString("");
        dictExperiments["meta"]["experiments"]["int_flag"].SetIntNumber(100); // check for fallback to string
        dictExperiments["meta"]["experiments"]["null_flag"].SetNull();

        TCtx ctx = MakeCtx(dictExperiments);
        UNIT_ASSERT(ctx.Context);

        UNIT_ASSERT(ctx.Context->HasExpFlag("super_flag"));
        UNIT_ASSERT(ctx.Context->ExpFlag("super_flag").Defined());
        UNIT_ASSERT_VALUES_EQUAL(*ctx.Context->ExpFlag("super_flag"), "super_value");

        UNIT_ASSERT(ctx.Context->HasExpFlag("duper_flag"));
        UNIT_ASSERT(ctx.Context->ExpFlag("duper_flag").Defined());
        UNIT_ASSERT_VALUES_EQUAL(*ctx.Context->ExpFlag("duper_flag"), "");

        UNIT_ASSERT(ctx.Context->HasExpFlag("int_flag"));
        UNIT_ASSERT(ctx.Context->ExpFlag("int_flag").Defined());
        UNIT_ASSERT_VALUES_EQUAL(*ctx.Context->ExpFlag("int_flag"), "100");

        UNIT_ASSERT(!ctx.Context->HasExpFlag("null_flag"));
    }

    Y_UNIT_TEST(ChangeForm) {
        for (const bool suppress : {false, true}) {
            for (const bool setCurrentFormAsCallback : {false, true}) {
                auto request = VALID_REQUEST.Clone();
                if (suppress) {
                    request["meta"]["suppress_form_changes"] = true;
                }
                TCtx ctx = MakeCtx(request);
                UNIT_ASSERT_VALUES_EQUAL_C(
                    !suppress, static_cast<bool>(ctx.Context->SetResponseForm("foo", setCurrentFormAsCallback)),
                    "setCurrentFormAsCallback = " << setCurrentFormAsCallback << ", suppress = " << suppress);
            }
        }
    }

    Y_UNIT_TEST(MultipleChangeForm) {
        for (const bool setCurrentFormAsCallback : {false, true}) {
            TCtx ctx = MakeCtx(VALID_REQUEST);
            UNIT_ASSERT_NO_EXCEPTION_C(ctx.Context->SetResponseForm("new_form_1", setCurrentFormAsCallback),
                                       "Thrown an exception during first changeform!");
            UNIT_ASSERT_NO_EXCEPTION_C(ctx.Context->SetResponseForm("new_form_2", setCurrentFormAsCallback),
                                       "Thrown an exception during second changeform!");
            UNIT_ASSERT_NO_EXCEPTION_C(ctx.Context->SetResponseForm("new_form_3", setCurrentFormAsCallback),
                                       "Thrown an exception during third changeform!");
            UNIT_ASSERT_VALUES_EQUAL(ctx.Context->OriginalFormName(), FORM_NAME);
        }
    }

    Y_UNIT_TEST(ContextChain) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        UNIT_ASSERT_VALUES_EQUAL(ctx.Context->FormName(), FORM_NAME);
        UNIT_ASSERT_VALUES_EQUAL(ctx.Context->ParentFormName(), "no_parent");
        UNIT_ASSERT_VALUES_EQUAL(ctx.Context->OriginalFormName(), FORM_NAME);

        const TString form2 = "form2";
        TContext context2(*ctx.Context, form2);
        UNIT_ASSERT_VALUES_EQUAL(context2.FormName(), form2);
        UNIT_ASSERT_VALUES_EQUAL(context2.ParentFormName(), FORM_NAME);
        UNIT_ASSERT_VALUES_EQUAL(context2.OriginalFormName(), FORM_NAME);

        const TString form3 = "form3";
        TContext context3(context2, form3);
        UNIT_ASSERT_VALUES_EQUAL(context3.FormName(), form3);
        UNIT_ASSERT_VALUES_EQUAL(context3.ParentFormName(), form2);
        UNIT_ASSERT_VALUES_EQUAL(context3.OriginalFormName(), FORM_NAME);
    }

    Y_UNIT_TEST(DataSources) {
        const NSc::TValue validDataSources = NSc::TValue::FromJson(R"({"4":{"user_region":213,"user_tld":"ru"}})");

        NSc::TValue dictDataSources = VALID_REQUEST.Clone();
        dictDataSources["data_sources"] = validDataSources;

        TCtx ctx = MakeCtx(dictDataSources);
        UNIT_ASSERT(ctx.Context);

        const auto& dataSources = ctx.Context->DataSources();
        UNIT_ASSERT(dataSources.contains(NAlice::EDataSourceType::USER_LOCATION));
        const NSc::TValue& userLocation = dataSources.at(NAlice::EDataSourceType::USER_LOCATION);
        UNIT_ASSERT_VALUES_EQUAL(213, userLocation["user_region"].GetIntNumber());
        UNIT_ASSERT_VALUES_EQUAL("ru", userLocation["user_tld"].GetString());
    }

    Y_UNIT_TEST(RestrictionLevel) {
        NSc::TValue request = VALID_REQUEST.Clone();

        for (const auto& [mode, expected_level] : THashMap<TString, NAlice::EContentSettings>{
            {"children", NAlice::EContentSettings::children},
            {"without", NAlice::EContentSettings::without},
            {"medium", NAlice::EContentSettings::medium},
            {"safe", NAlice::EContentSettings::safe}
        }) {
            request["meta"]["device_state"]["device_config"]["content_settings"] = mode;

            TCtx ctx = MakeCtx(request);
            UNIT_ASSERT_C(ctx.Context, ctx.Result);

            const auto& actual = ctx.Context->ContentRestrictionLevel();
            UNIT_ASSERT_EQUAL_C(expected_level, actual,
                                "Expected " << NAlice::EContentSettings_Name(expected_level) <<
                                " but actual is " << NAlice::EContentSettings_Name(actual));
        }
    }

    Y_UNIT_TEST(ChildRestrictionLevel) {
        NSc::TValue request = VALID_REQUEST.Clone();

        auto& device_config = request["meta"]["device_state"]["device_config"];
        device_config["content_settings"] = TStringBuf("medium");

        auto& biometryClassification = request["meta"]["biometry_classification"]["simple"][0];
        biometryClassification["tag"] = "children";
        biometryClassification["classname"] = "child";

        for (const auto& [mode, expected_level] : THashMap<TString, NAlice::EContentSettings>{
            {"children", NAlice::EContentSettings::children},
            {"safe", NAlice::EContentSettings::safe}
        }) {

            device_config["child_content_settings"] = mode;

            TCtx ctx = MakeCtx(request);
            UNIT_ASSERT_C(ctx.Context, ctx.Result);

            const auto& actual = ctx.Context->ContentRestrictionLevel();
            UNIT_ASSERT_EQUAL_C(expected_level, actual,
                                "Expected " << NAlice::EContentSettings_Name(expected_level) <<
                                " but actual is " << NAlice::EContentSettings_Name(actual));
            UNIT_ASSERT(ctx.Context->GetIsClassifiedAsChildRequest());
        }
    }

    Y_UNIT_TEST(CommitCandidateBlock) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        NSc::TValue commitPayload;
        commitPayload["hello"].SetString("world");

        NSc::TValue expected;
        expected["type"].SetString("commit_candidate");
        expected["data"] = commitPayload;

        ctx.Context->AddCommitCandidateBlock(commitPayload);
        UNIT_ASSERT(ctx.Context->HasAnyBlockOfType("commit_candidate"));
        UNIT_ASSERT(NTestingHelpers::EqualJson(expected, *ctx.Context->GetBlocks().begin()));
    }

    Y_UNIT_TEST(DataSourcesFlag) {
        TCtx ctx = MakeCtx(VALID_REQUEST_WITH_DATA_SOURCES);
        UNIT_ASSERT(ctx.Context);

        const auto* wizData = ctx.Context->DataSources().FindPtr(BEGEMOT_EXTERNAL_MARKUP_TYPE);
        const auto* bbData = ctx.Context->DataSources().FindPtr(BLACK_BOX_TYPE);
        UNIT_ASSERT(wizData);
        UNIT_ASSERT(bbData);

        UNIT_ASSERT_VALUES_EQUAL((*bbData)["user_info"]["uid"].GetString(), "12345");
        UNIT_ASSERT_VALUES_EQUAL((*wizData)["begemot_external_markup"]["Porn"].GetBool(), true);

        auto json = ctx.Context->ToJson(TContext::EJsonOut::DataSources);
        auto expected = VALID_REQUEST_WITH_DATA_SOURCES.Clone();
        expected.GetDictMutable().erase("meta");
        UNIT_ASSERT(NTestingHelpers::EqualJson(expected, json));
    }

    Y_UNIT_TEST(DontResubmitPositive) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        ctx.Context->SetDontResubmit();
        const auto json = ctx.Context->ToJson();
        UNIT_ASSERT(json["form"]["dont_resubmit"].GetBool());
    }

    Y_UNIT_TEST(DontResubmitNegative) {
        TCtx ctx = MakeCtx(VALID_REQUEST);
        // ctx.Context->SetDontResubmit();  // we are *not* doing it
        const auto json = ctx.Context->ToJson();
        UNIT_ASSERT(json["form"]["dont_resubmit"].IsNull());
    }

    Y_UNIT_TEST(UpdateMemento) {
        TCtx ctx = MakeCtx(VALID_REQUEST);

        NAlice::NData::NReminders::TState state;
        auto& r = *state.AddReminders();
        r.SetId("1234-456-1234");
        r.SetText("Test");
        r.SetShootAt(123456789);
        r.SetTimeZone("Europe/Moscow");

        NAlice::NScenarios::TMementoChangeUserObjectsDirective directive;

        auto userConfigs = CreateUserConfigs(EConfigKey::CK_REMINDERS, state);
        directive.MutableUserObjects()->AddUserConfigs()->Swap(&userConfigs);
        ctx.Context->AddMementoUpdateBlock(std::move(directive));
        const auto json = ctx.Context->ToJson();

        const auto& block = json["blocks"][0];
        UNIT_ASSERT_VALUES_EQUAL(block["command_type"].GetString(), "memento_change_user_objects_directive");
        UNIT_ASSERT_VALUES_EQUAL(block["type"].GetString(), "uniproxy-action");
        UNIT_ASSERT_VALUES_EQUAL(block["data"]["protobuf"].GetString(), "CmcKZQgXEmEKMnR5cGUuZ29vZ2xlYXBpcy5jb20vTkFsaWNlLk5EYXRhLk5SZW1pbmRlcnMuVFN0YXRlEisKKQoNMTIzNC00NTYtMTIzNBIEVGVzdCINRXVyb3BlL01vc2NvdzCVmu86");
    }

    Y_UNIT_TEST(ContextLocale) {
        const auto testCases = TVector<std::tuple<TStringBuf, TStringBuf, TStringBuf, TStringBuf>> {
            {"", "", "ru-RU", "ru"},

            {"ru-RU", "", "ru-RU", "ru"},
            {"tr-TR", "", "tr-TR", "tr"},
            {"ar-SA", "", "ru-RU", "ru"},

            {"", "ru", "ru-RU", "ru"},
            {"", "tr", "tr-TR", "tr"},
            {"", "ar", "ar", "ar"},

            {"ru-RU", "ru", "ru-RU", "ru"},
            {"tr-TR", "ru", "ru-RU", "ru"},
            {"ar-SA", "ru", "ru-RU", "ru"},

            {"ru-RU", "tr", "tr-TR", "tr"},
            {"tr-TR", "tr", "tr-TR", "tr"},
            {"ar-SA", "tr", "tr-TR", "tr"},

            {"ru-RU", "ar", "ar", "ar"},
            {"tr-TR", "ar", "ar", "ar"},
            {"ar-SA", "ar", "ar-SA", "ar"},
        };

        for (const auto& [reqClientLocale, reqUserLang, expectedLocale, expectedLang] : testCases) {
            NSc::TValue request = VALID_REQUEST.Clone();
            request["meta"]["lang"] = reqClientLocale;
            request["meta"]["user_lang"] = reqUserLang;

            const auto ctx = MakeCtx(request);
            UNIT_ASSERT_C(ctx.Result.Empty(), "Failed to build context for lang = " << reqClientLocale << ", user_lang = " << reqUserLang);
            UNIT_ASSERT_C(ctx.Context, "Failed to build context for lang = " << reqClientLocale << ", user_lang = " << reqUserLang);

            const auto locale = ctx.Context->MetaLocale();

            UNIT_ASSERT_EQUAL_C(expectedLocale, locale.ToString(), "Context for lang = " << reqClientLocale << ", user_lang = " << reqUserLang <<
                " is parsed with Locale = " << locale.ToString() << ", but expected " << expectedLocale);
            UNIT_ASSERT_EQUAL_C(expectedLang, locale.Lang, "Context for lang = " << reqClientLocale << ", user_lang = " << reqUserLang <<
                " is parsed with Lang = " << locale.Lang << ", but expected " << expectedLang);
        }
    }
}

} // namespace
