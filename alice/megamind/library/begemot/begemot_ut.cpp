#include "begemot.h"

#include <alice/megamind/library/apphost_request/util.h>
#include <alice/megamind/library/experiments/flags.h>
#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/mock_context.h>
#include <alice/megamind/library/testing/mock_session.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <alice/protos/data/entity_meta/video_nlu_meta.pb.h>
#include <alice/protos/data/language/language.pb.h>
#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/begemot/lib/api/params/wizextra.h>
#include <alice/library/unittest/fake_fetcher.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/json/json.h>

#include <library/cpp/testing/gmock_in_unittest/gmock.h>
#include <library/cpp/testing/unittest/registar.h>

#include <search/begemot/rules/alice/session/proto/alice_session.pb.h>
#include <search/begemot/rules/alice/response/proto/alice_response.pb.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <google/protobuf/util/message_differencer.h>

#include <util/charset/wide.h>
#include <util/generic/deque.h>
#include <util/random/mersenne.h>
#include <util/string/cast.h>


namespace NAlice::NMegamind {

using namespace NAlice::NImpl;
using namespace NAlice::NScenarios;
using namespace ::testing;

namespace {

constexpr size_t MAX_DIALOG_HISTORY_PHRASE_TOKEN_COUNT = 20;
constexpr size_t MAX_TEXT_TOKEN_COUNT = 64;

void BuildLongRequest(size_t maxTokenCount, TString& phrase, TString& strippedPhrase) {
    for (size_t i = 1; i < maxTokenCount + 10; ++i) {
        phrase += ToString(i) + " ";

        if (i < maxTokenCount) {
            strippedPhrase += ToString(i) + " ";
        } else if (i == maxTokenCount) {
            strippedPhrase += ToString(i);
        }
    }
}

const auto GALLERY = TString{"gallery"};
const auto WIZARD_RESPONSE = TString{R"(
{
    "rules": {
        "AliceRequest": {
            "FstText": ""
        }
    }
})"};

constexpr TStringBuf SK_REQ_GALLERY_NATIVE = R"(
{
    "request": {
        "device_state": {
            "video": {
                "current_screen": "gallery",
                "screen_state": {
                    "visible_items": [
                        0
                    ],
                    "items": [
                        {
                            "type": "video"
                        },
                        {
                            "type": "video"
                        }
                    ]
                }
            }
        }
    }
}
)";

constexpr TStringBuf SK_REQ_GALLERY_NATIVE_WITH_INVALID_INDEX = R"(
{
    "request": {
        "device_state": {
            "video": {
                "current_screen": "gallery",
                "screen_state": {
                    "visible_items": [
                        1
                    ],
                    "items": [
                        {
                            "type": "video"
                        }
                    ]
                }
            }
        }
    }
}
)";

constexpr TStringBuf SK_REQ_GALLERY_WEBVIEW = R"(
{
    "request": {
        "device_state": {
            "video": {
                "current_screen": "mordovia_webview",
                "view_state": {
                    "currentScreen": "videoSearch",
                    "sections": [
                        {
                            "items": [
                                {
                                    "provider_info": [{
                                         "type": "video",
                                         "available": 1
                                    }],
                                    "active": 1,
                                    "number": 1
                                },
                                {
                                    "provider_info": [{
                                         "type": "video",
                                         "available": 1
                                    }],
                                    "active": 0,
                                    "number": 2
                                }
                            ]
                        }
                    ]
                }
            }
        }
    }
}
)";

constexpr TStringBuf SK_REQ_ACTIONS = R"(
{
    "request": {
        "device_state": {
            "actions": {
                "action": {}
            }
        }
    }
}
)";

const THashSet<TStringBuf> ACTIVE_ACTIONS_SEMANTIC_FRAMES = {
    "SF_NAME",
    "personal_assistant.scenarios.search",
    "alice.iot.voice_discovery.success",
    "quasar.mordovia.home_screen"
};

constexpr TStringBuf SK_REQ_WITH_ACTIVE_ACTIONS = R"(
{
    "request": {
        "device_state": {
            "active_actions": {
                "semantic_frames": {
                    "SF_NAME": {}
                },
                "screen_conditional_action": {
                    "screen1": {
                        "conditional_actions": [
                            {
                                "conditional_semantic_frame": {
                                    "search_semantic_frame": {}
                                },
                                "effect_frame_request_data": {
                                    "iot_broadcast_start": {}
                                }
                            },
                            {
                                "conditional_semantic_frame": {
                                    "iot_broadcast_success": {}
                                },
                                "effect_frame_request_data": {
                                    "iot_broadcast_failure": {}
                                }
                            }
                        ]
                    },
                    "screen2": {
                        "conditional_actions": [
                            {
                                "conditional_semantic_frame": {
                                    "mordovia_home_screen": {}
                                },
                                "effect_frame_request_data": {
                                    "news_semantic_frame": {}
                                }
                            }
                        ]
                    },
                },
            }
        }
    }
}
)";

constexpr TStringBuf SK_REQ_FOR_GET_WIZEXTRA = R"(
{
    "request": {
        "device_state": {
            "video": {
                "current_screen": "gallery"
            },
            "device_id": "some_device_id",
            "active_actions": {
                "semantic_frames": {
                    "SF_NAME": {}
                }
            },
            "external_entities_description": [
                {
                    "name": "nonsense_entity_name",
                    "items": [
                        {
                            "phrase": "war",
                            "video_meta": {
                                "genre": "some_genre"
                            }
                        }
                    },
                    "value": {
                        "string_value": "first_element"
                    }
                }
            ]
        },
        "experiments": {
            ";": "1",
            "bg_fresh_granet_form=f1": "",
            "bg_fresh_granet_form=f2": "",
            "wtf": "1"
        },
        "environment_state": {
            "devices": [
                {
                    "device_state": {
                        "external_entities_description": [
                            {
                                "name": "nonsense_entity_name_2",
                                "items": [
                                    {
                                        "phrase": "battle"
                                    }
                                },
                                "value": {
                                    "string_value": "second_element"
                                }
                            }
                        ]
                    }
                }
            ]
        }
    },
    "contacts": {
        "status": "ok",
        "data": {
            "contacts": [
                {
                    "account_name": "test@gmail.com",
                    "first_name": "Test",
                    "lookup_key": "123",
                    "account_type": "com.google",
                    "display_name": "Test"
                }
            ]
        }
    }
}
)";

constexpr TStringBuf GRANET_RULE_RESULT = "{\"Name\":\"alice.external_skill_activate_weak\",\"LogProbability\":-21.38629532,"
                                          "\"Tags\":[{\"Begin\":1,\"End\":3,\"Name\":\"activation_phrase\"}]}";

const TClientEntity BuildExpectedGallery(const TString& galleryName) {
    TNluHint item{};
    item.AddInstances();

    TVideoGalleryItemMeta &meta = *item.MutableVideo();
    meta.SetType("video");
    meta.SetPosition(1);

    TClientEntity entity{};
    entity.SetName(galleryName);
    entity.MutableItems()->insert({"1", item});
    return entity;
}

TClientFeatures MakeClientFeatures(TStringBuf appId = {}) {
    auto features = TClientFeatures(TClientInfoProto{}, /* flags= */ {});
    features.Name = TString{appId};
    return features;
}

TFrameAction MakeFrameAction(const TString& frameName,
                             const TVector<TString>& instances = {},
                             const TVector<TString>& negatives = {})
{
    TFrameAction action;
    auto& nluHint = *action.MutableNluHint();
    nluHint.SetFrameName(frameName);
    for (const auto& phrase : instances) {
        auto& instance = *nluHint.AddInstances();
        instance.SetLanguage(L_RUS);
        instance.SetPhrase(phrase);
    }
    for (const auto& phrase : negatives) {
        auto& negative = *nluHint.AddNegatives();
        negative.SetLanguage(L_RUS);
        negative.SetPhrase(phrase);
    }
    return action;
}

TFrameAction SetFrameEffect(TFrameAction action, const TString& frameName) {
    action.MutableFrame()->SetName(frameName);
    return action;
}

TFrameAction SetParsedUtteranceEffect(TFrameAction action, const TParsedUtterance& parsedUtterance) {
    *action.MutableParsedUtterance() = parsedUtterance;
    return action;
}

TParsedUtterance MakeParsedUtterance(const TString& utterance, const TString& frameName) {
    TParsedUtterance parsedUtterance;
    parsedUtterance.SetUtterance(utterance);
    parsedUtterance.MutableFrame()->SetName(frameName);
    return parsedUtterance;
}

google::protobuf::Map<TString, TFrameAction> MakeActions() {
    google::protobuf::Map<TString, TFrameAction> actions{};
    actions["action_with_no_effect"] = MakeFrameAction("hint_frame_0",
                                                       {"positive_0", "positive_1"},
                                                       {"negative_0", "negative_2"});
    actions["action_with_frame_effect"] = SetFrameEffect(MakeFrameAction("hint_frame_1"), "effect_frame");
    actions["action_with_parsed_utterance_effect"] = SetParsedUtteranceEffect(MakeFrameAction("effect_frame"),
                                                                              MakeParsedUtterance("text", "pu_frame"));
    return actions;
}

google::protobuf::RepeatedPtrField<TClientEntity> MakeEntities() {
    google::protobuf::RepeatedPtrField<TClientEntity> entities;
    TClientEntity& entity = *entities.Add();
    entity.SetName("separate_entity");
    TNluHint& item = (*entity.MutableItems())["first_item"];
    TNluPhrase& phrase = *item.AddInstances();
    phrase.SetLanguage(L_RUS);
    phrase.SetPhrase("some text");
    return entities;
}

struct TFixture : public NTesting::TAppHostFixture {
    StrictMock<TMockContext> Context{};
    TClientFeatures EmptyFeatures = MakeClientFeatures();
    TClientFeatures SmartSpeakerFeatures = MakeClientFeatures(TStringBuf("ru.yandex.quasar"));
    StrictMock<TMockSession> Session{};
    google::protobuf::Map<TString, TFrameAction> Actions = MakeActions();
    TTestSpeechKitRequest EmptySpeechKitRequest = TSpeechKitRequestBuilder{TStringBuf{"{}"}}.Build();
    TIoTUserInfo EmptyIoTUserInfo{};
};

} // namespace

#define EXPECT_AT_LEAST_ONE_CALL(obj, call, action) EXPECT_CALL(obj, call).Times(AtLeast(1)).WillRepeatedly(action)

Y_UNIT_TEST_SUITE_F(Begemot, TFixture) {
    Y_UNIT_TEST(JoinWizextra) {
        const THashMap<TString, TString> data = {{"b", "1"}, {"a", "2"}, {"c", ""}};

        const TStringBuf expected = "a=2;b=1;c";
        const auto actual = JoinWizextra(data);

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(IsEnabledAliceTagger_Enabled) {
        EXPECT_CALL(Context, HasExpFlag(EXP_DISABLE_TAGGER)).WillOnce(Return(false));

        EXPECT_CALL(Context, Language()).WillOnce(Return(ELanguage::LANG_RUS));

        UNIT_ASSERT(IsEnabledAliceTagger(Context, Context.Language()));
    }

    Y_UNIT_TEST(IsEnabledAliceTagger_DisabledIfLangIsNotRus) {
        EXPECT_CALL(Context, Language()).WillOnce(Return(ELanguage::LANG_UNK));

        UNIT_ASSERT(!IsEnabledAliceTagger(Context, Context.Language()));
    }

    Y_UNIT_TEST(IsEnabledAliceTagger_DisabledByFlag) {
        EXPECT_CALL(Context, HasExpFlag(EXP_DISABLE_TAGGER)).WillOnce(Return(true));

        EXPECT_CALL(Context, Language()).WillOnce(Return(ELanguage::LANG_RUS));

        UNIT_ASSERT(!IsEnabledAliceTagger(Context, Context.Language()));
    }

    Y_UNIT_TEST(EnabledBegemotRules) {
        EXPECT_CALL(Context, Language()).WillRepeatedly(Return(ELanguage::LANG_RUS));
        EXPECT_CALL(Context, HasExpFlag(EXP_DISABLE_TAGGER)).WillOnce(Return(false));
        EXPECT_CALL(Context, HasExpFlag(EXP_BEGEMOT_REWRITE_ELLIPSIS)).WillOnce(Return(true));
        EXPECT_CALL(Context, HasExpFlag(EXP_ENABLE_GC_MEMORY_LSTM)).WillOnce(Return(true));
        EXPECT_CALL(Context, HasExpFlag(EXP_ENABLE_PARTIAL_PRECLASSIFIER)).WillOnce(Return(true));
        EXPECT_CALL(Context, HasExpFlag(EXP_USE_SEARCH_QUERY_PREPARE_RULE)).WillOnce(Return(true));
        EXPECT_CALL(Context, HasExpFlag(EXP_ENABLE_GC_WIZ_DETECTION)).WillOnce(Return(true));
        const TString aliceMultiIntentClassifierRule = "AliceMultiIntentClassifier";
        const TString aliceNluFeaturesRule = "AliceNluFeatures";
        const THashMap<TString, TMaybe<TString>> expFlags{{EXP_ENABLE_BEGEMOT_RULES_PREFIX + aliceMultiIntentClassifierRule + ";" + aliceNluFeaturesRule, "1"}};
        EXPECT_CALL(Context, ExpFlags()).WillOnce(ReturnRef(expFlags));

        const auto target = EnabledBegemotRules(Context, Context.Language());
        for (const auto expectedRule : {
                 TStringBuf("AliceTagger"),
                 TStringBuf("AliceEllipsisRewriter"),
                 TStringBuf("AliceGcMemoryStateUpdater"),
                 TStringBuf("AlicePartialPreEmbedding"),
                 TStringBuf("AliceSearchQueryPreparer"),
                 TStringBuf("AliceWizDetection"),
                 TStringBuf(aliceMultiIntentClassifierRule),
                 TStringBuf(aliceNluFeaturesRule),
             }) {
            UNIT_ASSERT(FindPtr(target, expectedRule));
        }
    }

    Y_UNIT_TEST(IsEnabledResolveContextualAmbiguity_DisabledIfLangIsNotRus) {
        EXPECT_CALL(Context, Language()).WillOnce(Return(ELanguage::LANG_UNK));

        UNIT_ASSERT(!IsEnabledResolveContextualAmbiguity(Context, Context.Language()));
    }

    Y_UNIT_TEST(IsEnabledResolveContextualAmbiguity_Enabled) {
        EXPECT_AT_LEAST_ONE_CALL(Context, Language(), Return(ELanguage::LANG_RUS));

        EXPECT_CALL(Context, HasExpFlag(EXP_BEGEMOT_REWRITE_ANAPHORA)).WillOnce(Return(true));
        UNIT_ASSERT(IsEnabledResolveContextualAmbiguity(Context, Context.Language()));

        EXPECT_CALL(Context, HasExpFlag(EXP_BEGEMOT_REWRITE_ANAPHORA)).WillOnce(Return(false));
        EXPECT_CALL(Context, HasExpFlag(EXP_BEGEMOT_REWRITE_ELLIPSIS)).WillOnce(Return(true));
        UNIT_ASSERT(IsEnabledResolveContextualAmbiguity(Context, Context.Language()));
    }

    void TestStripPhrase(const TString& phrase, size_t maxTokenCount, size_t maxSymbolCount, const TString& expected) {
        const TString actual = StripPhrase(phrase, maxTokenCount, maxSymbolCount);
        UNIT_ASSERT_STRINGS_EQUAL_C(expected, actual, Endl
            << "Details:" << Endl
            << "  phrase: \"" << phrase << "\"" << Endl
            << "  maxTokenCount: " << maxTokenCount << Endl
            << "  maxSymbolCount: " << maxSymbolCount << Endl
            << "  expected: \"" << expected << "\"" << Endl
            << "  actual: \"" << actual << "\"" << Endl);
    }

    Y_UNIT_TEST(StripPhrase) {
        TestStripPhrase("мама мыла раму",    2, 100, "мама мыла");
        TestStripPhrase("мама:   мыла раму", 2, 100, "мама:   мыла");
        TestStripPhrase("мама-мыла раму",    2, 100, "мама-мыла");
        TestStripPhrase("мама-мыла раму",    1, 100, "мама");
        TestStripPhrase("мама:   мыла раму", 5, 100, "мама:   мыла раму");
        TestStripPhrase("мама:   мыла раму", 3, 100, "мама:   мыла раму");
        TestStripPhrase("мама:   мыла раму", 1, 100, "мама");
        TestStripPhrase("мама:   мыла раму", 0, 100, "");
        TestStripPhrase("мама:   мыла раму", 3,   0, "");
        TestStripPhrase("мама:   мыла раму", 3,   4, "мама");
        TestStripPhrase("мама:   мыла раму", 3,   6, "мама: ");
        TestStripPhrase("мама:   мыла раму", 3,   9, "мама:   м");
    }

    Y_UNIT_TEST(StripPhraseIsRobustToBrokenUtf8) {
        const TString phrase = "мама мыла";
        const TString broken = phrase.substr(0, phrase.length() - 1); // break last utf8 symbol
        const TString fixed = WideToUTF8(UTF8ToWide<true>(broken));
        TestStripPhrase(broken, 10, 100, broken);
        TestStripPhrase(broken + " раму", 10, 100, broken + " раму");
        TestStripPhrase(broken + " раму", 2, 100, fixed);
        TestStripPhrase(broken, 1, 100, "мама");
        TestStripPhrase(broken, 10, 7, "мама мы");
    }

    Y_UNIT_TEST(AddJsonDialogHistoryToWizextra_NoPhrases) {
        TDialogHistory dialogHistory;
        const THashMap<TString, TString> expected{};
        auto actual = expected;

        AddJsonDialogHistoryToWizextra(dialogHistory, actual);

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(AddJsonDialogHistoryToWizextra) {
        TDialogHistory dialogHistory;
        dialogHistory.PushDialogTurn({"abc,def;ghi", "abc,def;ghi", ";,", "ScenarioName", 0, 0});
        const THashMap<TString, TString> expected {
            {WIZEXTRA_KEY_PREVIOUS_PHRASES, R"(["abc,def,ghi",",,"])"},
            {WIZEXTRA_KEY_PREVIOUS_REWRITTEN_REQUESTS, R"(["abc,def,ghi"])"}
        };
        THashMap<TString, TString> actual;
        AddJsonDialogHistoryToWizextra(dialogHistory, actual);

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(AddJsonDialogHistoryToWizextra_LongPhrases) {
        NJson::TJsonValue expectedPhrasesJson;
        NJson::TJsonValue expectedRewrittenRequestsJson;
        TDialogHistory dialogHistory;

        for (size_t phraseIndex = 0; phraseIndex < 3; ++phraseIndex) {
            TString inputPhrase;
            TString expectedPhrase;
            BuildLongRequest(MAX_DIALOG_HISTORY_PHRASE_TOKEN_COUNT, inputPhrase, expectedPhrase);
            TString responsePhrase;
            TString expectedResponsePhrase;
            BuildLongRequest(MAX_DIALOG_HISTORY_PHRASE_TOKEN_COUNT, responsePhrase, expectedResponsePhrase);
            dialogHistory.PushDialogTurn({inputPhrase, inputPhrase, responsePhrase, "ScenarioName", 0, 0});

            expectedPhrasesJson.AppendValue(expectedPhrase);
            expectedPhrasesJson.AppendValue(expectedResponsePhrase);
            expectedRewrittenRequestsJson.AppendValue(expectedResponsePhrase);
        }

        const THashMap<TString, TString> expected {
            {WIZEXTRA_KEY_PREVIOUS_PHRASES, JsonToString(expectedPhrasesJson)},
            {WIZEXTRA_KEY_PREVIOUS_REWRITTEN_REQUESTS, JsonToString(expectedRewrittenRequestsJson)}
        };
        THashMap<TString, TString> actual;
        AddJsonDialogHistoryToWizextra(dialogHistory, actual);

        UNIT_ASSERT_VALUES_EQUAL(expected, actual);
    }

    Y_UNIT_TEST(GetScenarioAvailableFrameActionsProto) {
        EXPECT_CALL(Context, Session()).WillOnce(Return(&Session));
        EXPECT_CALL(Session, GetActions()).WillOnce(Return(Actions));

        const auto target = GetScenarioAvailableFrameActionsProto(Context).GetActionsByName();

        UNIT_ASSERT_VALUES_EQUAL(target.size(), Actions.size());
        bool hasFrameEffect = false;
        bool hasParsedUtteranceEffect = false;
        bool hasNoEffect = false;
        for (const auto& [actionId, action] : Actions) {
            if (action.HasFrame()) {
                UNIT_ASSERT(target.count(actionId));
                UNIT_ASSERT_MESSAGES_EQUAL(target.at(actionId).GetNluHint(), action.GetNluHint());
                hasNoEffect = true;
            } else if (action.HasParsedUtterance()) {
                UNIT_ASSERT(target.count(actionId));
                UNIT_ASSERT_MESSAGES_EQUAL(target.at(actionId).GetNluHint(), action.GetNluHint());
                hasParsedUtteranceEffect = true;
            } else {
                UNIT_ASSERT(target.count(actionId));
                UNIT_ASSERT_MESSAGES_EQUAL(target.at(actionId).GetNluHint(), action.GetNluHint());
                hasFrameEffect = true;
            }
        }
        UNIT_ASSERT(hasFrameEffect);
        UNIT_ASSERT(hasParsedUtteranceEffect);
        UNIT_ASSERT(hasNoEffect);
    }

    Y_UNIT_TEST(GetDeviceStateAvailableActionsProto) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_ACTIONS}.Build();
        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(skRequest));

        const auto target = GetDeviceStateAvailableActionsProto(Context).GetActionsByName();

        UNIT_ASSERT_VALUES_EQUAL(target.size(), 1);
        UNIT_ASSERT(target.count("action"));
    }

    Y_UNIT_TEST(GetAvailableActiveSpaceActionsProto) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_WITH_ACTIVE_ACTIONS}.Build();
        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(skRequest));

        const auto frames = GetAvailableActiveSpaceActionsProto(Context).GetSemanticFrames();

        UNIT_ASSERT_VALUES_EQUAL(frames.size(), ACTIVE_ACTIONS_SEMANTIC_FRAMES.size());
        for (int i = 0; i < frames.size(); ++i) {
            UNIT_ASSERT(ACTIVE_ACTIONS_SEMANTIC_FRAMES.contains(frames.Get(i)));
        }
    }

    Y_UNIT_TEST(GetVideoGallery_ReturnNothingOnNotGallery) {
        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(EmptySpeechKitRequest));

        UNIT_ASSERT(!GetVideoGallery(Context, /* galleryName= */ GALLERY).Defined());
    }

    Y_UNIT_TEST(GetVideoGallery_Native) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_GALLERY_NATIVE}.Build();
        const TClientEntity& expected = BuildExpectedGallery(GALLERY);

        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(skRequest));

        const auto actual = GetVideoGallery(Context, /* galleryName= */ GALLERY);
        UNIT_ASSERT(actual.Defined());
        UNIT_ASSERT(actual->GetItems().size() == 1);

        UNIT_ASSERT_MESSAGES_EQUAL(expected, *actual);
    }

    Y_UNIT_TEST(GetVideoGallery_Native_InvalidIndex) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_GALLERY_NATIVE_WITH_INVALID_INDEX}.Build();

        EXPECT_CALL(Context, Logger()).WillOnce(ReturnRef(TRTLogger::StderrLogger()));
        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(skRequest));

        const auto actual = GetVideoGallery(Context, /* galleryName= */ GALLERY);
        UNIT_ASSERT(actual.Defined());
        UNIT_ASSERT(actual->GetItems().size() == 0);
    }

    Y_UNIT_TEST(GetVideoGallery_WebView) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_GALLERY_WEBVIEW}.Build();
        const TClientEntity& expected = BuildExpectedGallery(GALLERY);

        EXPECT_CALL(Context, SpeechKitRequest()).WillOnce(Return(skRequest));

        const auto actual = GetVideoGallery(Context, /* galleryName= */ GALLERY);
        UNIT_ASSERT(actual.Defined());
        UNIT_ASSERT(actual->GetItems().size() == 1);

        UNIT_ASSERT_MESSAGES_EQUAL(expected, *actual);
    }

    Y_UNIT_TEST(GetWizextra) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_FOR_GET_WIZEXTRA}.Build();

        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(EXP_BEGEMOT_REWRITE_ANAPHORA), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(EXP_DISABLE_BEGEMOT_ITEM_SELECTOR), Return(false));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasIoTUserInfo(), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, IoTUserInfo(), ReturnRef(EmptyIoTUserInfo));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(TStringBuf("wtf")), Return(false));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(TStringBuf(";")), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(TStringBuf("bg_fresh_granet_form=f1")), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(TStringBuf("bg_fresh_granet_form=f2")), Return(true));

        EXPECT_AT_LEAST_ONE_CALL(Context, SpeechKitRequest(), Return(skRequest));
        EXPECT_AT_LEAST_ONE_CALL(Context, ExpFlags(), ReturnRef(skRequest.ExpFlags()));
        EXPECT_AT_LEAST_ONE_CALL(Context, Language(), Return(ELanguage::LANG_RUS));
        EXPECT_AT_LEAST_ONE_CALL(Context, Logger(), ReturnRef(TRTLogger::NullLogger()));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientInfo(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientFeatures(), ReturnRef(SmartSpeakerFeatures));

        EXPECT_AT_LEAST_ONE_CALL(Context, Session(), Return(nullptr));

        const TMap<TString, TString> expectedOnNullSession = {
            {"operation_threshold", "50"},
            {"device_actions", "e30,"},
            {"resolve_contextual_ambiguity", "true"},
            {"galleries", "eyJlbnRpdGllcyI6W3sibmFtZSI6InZpZGVvX2dhbGxlcnkifV19"},
            {"contacts_proto", "ChsKCGNvbnRhY3RzEg8KAzEyMxIIEgYSBFRlc3Q,"},
            {"iot_user_info", {}},
            {"bg_fresh_granet_form=f1", {}},
            {"bg_fresh_granet_form=f2", {}},
            {"available_frame_actions", "e30,"},
            {"user_entity_dicts", "W10,"},
            {"alice_preprocessing", "true"},
            {"granet_print_sample_mock", "true"},
            {"is_smart_speaker", "true"},
            {"device_id", "some_device_id"},
            {"active_space_actions", "eyJTZW1hbnRpY0ZyYW1lcyI6WyJTRl9OQU1FIl19"},
            {"bg_gc_classifier_context_length=3", {}},
        };

        auto asMap = [](const THashMap<TString, TString>& hm) { return TMap<TString, TString>(hm.begin(), hm.end()); };

        UNIT_ASSERT_VALUES_EQUAL(expectedOnNullSession, asMap(GetWizextra(Context, /* text= */ {}, Context.Language(), /* addContacts= */ true)));

        EXPECT_AT_LEAST_ONE_CALL(Context, Session(), Return(&Session));

        const TDialogHistory dialogHistory = {{{"Привет!", "Привет!", "Хэллоу", "SomeScenario", 0, 0},
                                              {"Как дела?", "Как дела?", "Норм", "SomeScenario", 1, 1}}};
        EXPECT_AT_LEAST_ONE_CALL(Session, GetActions(), Return(Actions));
        EXPECT_AT_LEAST_ONE_CALL(Session, GetDialogHistory(), Return(dialogHistory));
        EXPECT_AT_LEAST_ONE_CALL(Session, GetResponseFrame(), Return(TSemanticFrame{}));
        EXPECT_AT_LEAST_ONE_CALL(Session, GetResponseEntities(), Return(MakeEntities()));
        EXPECT_AT_LEAST_ONE_CALL(Session, GetGcMemoryState(), Return(TGcMemoryState{}));

        EXPECT_AT_LEAST_ONE_CALL(Context, ClientFeatures(), ReturnRef(EmptyFeatures));

        const TString resultEntities = "W3siaXRlbXMiOnsiIjp7IlZpZGVvIjp7ImdlbnJlIjoic29tZV9nZW5yZSJ9fX0sIm5hbWUiOiJub25zZW5zZV9lbnRpdHlfbmFtZSJ9LHsiaXRlbXMiOnsiIjp7fX0sIm5hbWUiOiJub25zZW5zZV9lbnRpdHlfbmFtZV8yIn0seyJpdGVtcyI6eyJmaXJzdF9pdGVtIjp7Imluc3RhbmNlcyI6W3sibGFuZ3VhZ2UiOiJMX1JVUyIsInBocmFzZSI6InNvbWUgdGV4dCJ9XX19LCJuYW1lIjoic2VwYXJhdGVfZW50aXR5In1d";
        {
            const TMap<TString, TString> expectedAddContactsTrue = {
                {"operation_threshold", "50"},
                {"device_actions", "e30,"},
                {"galleries", "eyJlbnRpdGllcyI6W3sibmFtZSI6InZpZGVvX2dhbGxlcnkifV19"},
                {"contacts_proto", "ChsKCGNvbnRhY3RzEg8KAzEyMxIIEgYSBFRlc3Q,"},
                {"bg_fresh_granet_form=f1", {}},
                {"bg_fresh_granet_form=f2", {}},
                {"iot_user_info", {}},
                {"resolve_contextual_ambiguity", "true"},
                {"semantic_frame", "e30,"},
                {"entities", resultEntities},
                {"available_frame_actions", "eyJBY3Rpb25zQnlOYW1lIjp7ImFjdGlvbl93aXRoX3BhcnNlZF91dHRlcmFuY2VfZWZmZWN0Ijp7Ik5sdUhpbnQiOnsiZnJhbWVfbmFtZSI6ImVmZmVjdF9mcmFtZSJ9fSwiYWN0aW9uX3dpdGhfbm9fZWZmZWN0Ijp7Ik5sdUhpbnQiOnsiZnJhbWVfbmFtZSI6ImhpbnRfZnJhbWVfMCIsImluc3RhbmNlcyI6W3sicGhyYXNlIjoicG9zaXRpdmVfMCIsImxhbmd1YWdlIjoiTF9SVVMifSx7InBocmFzZSI6InBvc2l0aXZlXzEiLCJsYW5ndWFnZSI6IkxfUlVTIn1dLCJuZWdhdGl2ZXMiOlt7InBocmFzZSI6Im5lZ2F0aXZlXzAiLCJsYW5ndWFnZSI6IkxfUlVTIn0seyJwaHJhc2UiOiJuZWdhdGl2ZV8yIiwibGFuZ3VhZ2UiOiJMX1JVUyJ9XX19LCJhY3Rpb25fd2l0aF9mcmFtZV9lZmZlY3QiOnsiTmx1SGludCI6eyJmcmFtZV9uYW1lIjoiaGludF9mcmFtZV8xIn19fX0,"},
                {"gc_memory_state", ""},
                {"user_entity_dicts", "W10,"},
                {"previous_phrases", R"(["Привет!","Хэллоу","Как дела?","Норм"])"},
                {"previous_rewritten_requests", R"(["Привет!","Как дела?"])"},
                {"alice_preprocessing", "true"},
                {"granet_print_sample_mock", "true"},
                {"is_smart_speaker", "true"},
                {"device_id", "some_device_id"},
                {"alice_enabled_conditional_forms", "effect_frame,hint_frame_0,hint_frame_1"},
                {"active_space_actions", "eyJTZW1hbnRpY0ZyYW1lcyI6WyJTRl9OQU1FIl19"},
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedAddContactsTrue, asMap(GetWizextra(Context, /* text= */ {}, Context.Language(), /* addContacts= */ true)));
        }

        {
            const TMap<TString, TString> expectedAddContactsFalse = {
                {"operation_threshold", "50"},
                {"device_actions", "e30,"},
                {"galleries", "eyJlbnRpdGllcyI6W3sibmFtZSI6InZpZGVvX2dhbGxlcnkifV19"},
                {"bg_fresh_granet_form=f1", {}},
                {"bg_fresh_granet_form=f2", {}},
                {"iot_user_info", {}},
                {"resolve_contextual_ambiguity", "true"},
                {"semantic_frame", "e30,"},
                {"entities", resultEntities},
                {"available_frame_actions", "eyJBY3Rpb25zQnlOYW1lIjp7ImFjdGlvbl93aXRoX3BhcnNlZF91dHRlcmFuY2VfZWZmZWN0Ijp7Ik5sdUhpbnQiOnsiZnJhbWVfbmFtZSI6ImVmZmVjdF9mcmFtZSJ9fSwiYWN0aW9uX3dpdGhfbm9fZWZmZWN0Ijp7Ik5sdUhpbnQiOnsiZnJhbWVfbmFtZSI6ImhpbnRfZnJhbWVfMCIsImluc3RhbmNlcyI6W3sicGhyYXNlIjoicG9zaXRpdmVfMCIsImxhbmd1YWdlIjoiTF9SVVMifSx7InBocmFzZSI6InBvc2l0aXZlXzEiLCJsYW5ndWFnZSI6IkxfUlVTIn1dLCJuZWdhdGl2ZXMiOlt7InBocmFzZSI6Im5lZ2F0aXZlXzAiLCJsYW5ndWFnZSI6IkxfUlVTIn0seyJwaHJhc2UiOiJuZWdhdGl2ZV8yIiwibGFuZ3VhZ2UiOiJMX1JVUyJ9XX19LCJhY3Rpb25fd2l0aF9mcmFtZV9lZmZlY3QiOnsiTmx1SGludCI6eyJmcmFtZV9uYW1lIjoiaGludF9mcmFtZV8xIn19fX0,"},
                {"gc_memory_state", ""},
                {"user_entity_dicts", "W10,"},
                {"previous_phrases", R"(["Привет!","Хэллоу","Как дела?","Норм"])"},
                {"previous_rewritten_requests", R"(["Привет!","Как дела?"])"},
                {"alice_preprocessing", "true"},
                {"granet_print_sample_mock", "true"},
                {"is_smart_speaker", "true"},
                {"device_id", "some_device_id"},
                {"alice_enabled_conditional_forms", "effect_frame,hint_frame_0,hint_frame_1"},
                {"active_space_actions", "eyJTZW1hbnRpY0ZyYW1lcyI6WyJTRl9OQU1FIl19"},
            };
            UNIT_ASSERT_VALUES_EQUAL(expectedAddContactsFalse, asMap(GetWizextra(Context, /* text= */ {}, Context.Language(), /* addContacts= */ false)));
        }

    }

    Y_UNIT_TEST(CreateBegemotRequest) {
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(_), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, SpeechKitRequest(), Return(EmptySpeechKitRequest));
        EXPECT_AT_LEAST_ONE_CALL(Context, ExpFlags(), ReturnRef(EmptySpeechKitRequest.ExpFlags()));
        EXPECT_AT_LEAST_ONE_CALL(Context, Language(), Return(ELanguage::LANG_RUS));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientInfo(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientFeatures(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, Session(), Return(nullptr));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasIoTUserInfo(), Return(false));

        NTestingHelpers::TFakeRequestBuilder requestBuilder{};
        UNIT_ASSERT(CreateBegemotRequest(/* text= */ "TEXT", Context.Language(), Context, requestBuilder).IsSuccess());

        const auto& actual = requestBuilder.Cgi;
        for (const auto& [k, v] : THashMap<TString, TString>{
                 {"format", "json"},
                 {"lr", "213"},
                 {"text", "TEXT"},
                 {"tld", "ru"},
                 {"uil", "ru"},
                 {"wizclient", "megamind"},
                 {"wizdbg", "2"},
             }) {
            UNIT_ASSERT_VALUES_EQUAL(v, actual.Get(k));
        }
        UNIT_ASSERT(!actual.Get("rwr").empty());
        UNIT_ASSERT(!actual.Get("wizextra").empty());
    }

    Y_UNIT_TEST(CreateBegemotRequest_FailsOnEmptyText) {
        NTestingHelpers::TFakeRequestBuilder requestBuilder{};
        UNIT_ASSERT(!CreateBegemotRequest(/* text= */ {}, ELanguage::LANG_RUS, Context, requestBuilder).IsSuccess());
    }

    Y_UNIT_TEST(CreateBegemotRequest_StripLongText) {
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(_), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, SpeechKitRequest(), Return(EmptySpeechKitRequest));
        EXPECT_AT_LEAST_ONE_CALL(Context, ExpFlags(), ReturnRef(EmptySpeechKitRequest.ExpFlags()));
        EXPECT_AT_LEAST_ONE_CALL(Context, Language(), Return(ELanguage::LANG_RUS));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientInfo(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientFeatures(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, Session(), Return(nullptr));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasIoTUserInfo(), Return(false));

        TString text;
        TString strippedText;
        BuildLongRequest(MAX_TEXT_TOKEN_COUNT, text, strippedText);

        NTestingHelpers::TFakeRequestBuilder requestBuilder{};
        UNIT_ASSERT(CreateBegemotRequest(text, Context.Language(), Context, requestBuilder).IsSuccess());
        UNIT_ASSERT_VALUES_EQUAL(requestBuilder.Cgi.Get("text"), strippedText);
    }

    Y_UNIT_TEST(CreateNativeBegemotRequest) {
        const auto skRequest = TSpeechKitRequestBuilder{SK_REQ_FOR_GET_WIZEXTRA}.Build();

        EXPECT_AT_LEAST_ONE_CALL(Context, SpeechKitRequest(), Return(skRequest));
        EXPECT_AT_LEAST_ONE_CALL(Context, ExpFlags(), ReturnRef(skRequest.ExpFlags()));
        EXPECT_AT_LEAST_ONE_CALL(Context, Language(), Return(ELanguage::LANG_RUS));
        EXPECT_AT_LEAST_ONE_CALL(Context, Logger(), ReturnRef(TRTLogger::NullLogger()));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientInfo(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, ClientFeatures(), ReturnRef(SmartSpeakerFeatures));
        EXPECT_AT_LEAST_ONE_CALL(Context, Session(), Return(nullptr));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasExpFlag(_), Return(true));
        EXPECT_AT_LEAST_ONE_CALL(Context, HasIoTUserInfo(), Return(false));

        NJson::TJsonValue request;
        UNIT_ASSERT(CreateNativeBegemotRequest(/* text= */ "TEXT", Context.Language(), Context, request).IsSuccess());
        const NJson::TJsonValue& params = request["params"];
        UNIT_ASSERT(params["text"] == NJson::TJsonArray({"TEXT"}));
        UNIT_ASSERT(params["lr"] == NJson::TJsonArray({"213"}));
        UNIT_ASSERT(params["uil"] == NJson::TJsonArray({"ru"}));
        // check that wizextra are stored separately
        UNIT_ASSERT(params["wizextra"].GetArray().size() > 1);

        // check that rwr rules are passed in config
        UNIT_ASSERT(params["rwr"].GetArray().size() == 0);
        auto nativeRequest = SetRequiredRules(request, Context, Context.Language());
        UNIT_ASSERT(nativeRequest["params"]["rwr"].GetArray().size() != 0);
    }

    Y_UNIT_TEST(ParseAliceBegemotResponse) {
        GlobalCtx.GenericInit();
        auto ahCtx = CreateAppHostContext();

        const TString userRequest = "test toplevel field";
        const TString translatedNormalizedRequest = "translated normalized request";

        {
            NJson::TJsonValue granetJsonForms;
            NJson::ReadJsonFastTree(GRANET_RULE_RESULT, &granetJsonForms, true);

            NBg::NProto::TAlicePolyglotMergeResponseResult responseProto;
            NBg::NProto::TGranetResult& result = *responseProto.MutableAliceResponse()->MutableGranet();
            NBg::NProto::TGranetForm* granetForm = result.AddForms();
            granetForm->SetName(granetJsonForms["Name"].GetString());
            granetForm->SetLogProbability(granetJsonForms["LogProbability"].GetDouble());

            NBg::NProto::TGranetTag* granetTag = granetForm->AddTags();
            NJson::TJsonValue granetJsonTag = granetJsonForms["Tags"][0];
            granetTag->SetName(granetJsonTag["Name"].GetString());
            granetTag->SetBegin(granetJsonTag["Begin"].GetInteger());
            granetTag->SetEnd(granetJsonTag["End"].GetInteger());

            *result.MutableWizForms() = result.GetForms();

            NBg::NProto::TTextResult& textResult = *responseProto.MutableAliceResponse()->MutableText();
            textResult.SetUserRequest(userRequest);

            *responseProto.MutableTranslatedResponse()->MutableAliceNormalizer()->MutableNormalizedRequest() = translatedNormalizedRequest;

            ahCtx.TestCtx().AddProtobufItem(responseProto, AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME, NAppHost::EContextItemKind::Input);
        }

        TWizardResponse wizardResponse;
        NBg::NProto::TAlicePolyglotMergeResponseResult begemotResponse;
        Y_ENSURE(!GetFirstProtoItem(ahCtx.ItemProxyAdapter(), AH_ITEM_BEGEMOT_ALICE_POLYGLOT_MERGER_RESPONSE_NAME, begemotResponse));

        const auto aliceResponse = begemotResponse.GetAliceResponse(); // need copy as we move original proto
        Y_ENSURE(!ParseBegemotAliceResponse(std::move(begemotResponse), /* needGranetLog= */ false).MoveTo(wizardResponse));

        UNIT_ASSERT(google::protobuf::util::MessageDifferencer::Equals(wizardResponse.GetProtoResponse(), aliceResponse));

        // check correct parsing begemot proto flags (inline_json), (always_array)
        const NJson::TJsonValue& granetResult = wizardResponse.RawResponse()["rules"]["Granet"];

        UNIT_ASSERT_STRINGS_EQUAL(granetResult["WizForms"].GetString(), GRANET_RULE_RESULT);

        NJson::TJsonValue granetJsonForms;
        NJson::ReadJsonFastTree(granetResult["WizForms"].GetString(), &granetJsonForms, true);
        UNIT_ASSERT(granetResult["Forms"][0] == granetJsonForms);

        UNIT_ASSERT_STRINGS_EQUAL(wizardResponse.RawResponse()["original_request"].GetString(), userRequest);

        UNIT_ASSERT(wizardResponse.GetNormalizedTranslatedUtterance());
        UNIT_ASSERT_STRINGS_EQUAL(wizardResponse.GetNormalizedTranslatedUtterance().GetRef(), translatedNormalizedRequest);
    }

}

} // namespace NAlice::NMegamind
