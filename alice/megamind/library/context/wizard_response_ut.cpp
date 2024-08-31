#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

#include <alice/megamind/library/context/wizard_response.h>
#include <alice/library/video_common/defs.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

TWizardResponse WizardFromString(TStringBuf wizardData) {
    NJson::TJsonValue begemotResponseJson;
    NJson::ReadJsonTree(wizardData, &begemotResponseJson);
    auto begemotResponse = NAlice::JsonToProto<NBg::NProto::TAlicePolyglotMergeResponseResult>(begemotResponseJson);
    return TWizardResponse(std::move(begemotResponse));
}

Y_UNIT_TEST_SUITE(WizardResponseToVideoSlots) {

Y_UNIT_TEST(Empty) {
    auto wizard = WizardFromString("{}");
    UNIT_ASSERT(!wizard.GetRequestFrame(NVideoCommon::SEARCH_VIDEO));
}

}

Y_UNIT_TEST_SUITE(GetSearchQuery) {
    Y_UNIT_TEST(Anaphora) {
        auto wizard = WizardFromString(R"({
            "AliceResponse": {
                "AliceAnaphoraSubstitutor": {
                    "Substitution": [
                        {
                            "IsRewritten": true,
                            "Score": 0.9969587326,
                            "RewrittenRequest": "а сколько джонни деппу лет"
                        }
                    ]
                }
            }
        })");
        const THashMap<TString, TMaybe<TString>> expFlags;

        const auto query = wizard.GetSearchQuery(expFlags);
        UNIT_ASSERT(query.Defined());
        UNIT_ASSERT_EQUAL(*query, TStringBuf("а сколько джонни деппу лет"));
    }
    Y_UNIT_TEST(AnaphoraLowScore) {
        auto wizard = WizardFromString(R"({
            "AliceResponse": {
                "AliceAnaphoraSubstitutor": {
                    "Substitution": [
                        {
                            "IsRewritten": true,
                            "Score": 0.8969587326,
                            "RewrittenRequest": "а сколько джонни деппу лет"
                        }
                    ]
                }
            }
        })");
        const THashMap<TString, TMaybe<TString>> expFlags;
        const auto query = wizard.GetSearchQuery(expFlags);
        UNIT_ASSERT(!query.Defined());
    }
    Y_UNIT_TEST(QueryFromFrame) {
        auto wizard = WizardFromString(R"({
            "AliceResponse": {
                "AliceParsedFrames": {
                    "Frames": [
                        {
                            "Slots": [
                                {
                                    "AcceptedTypes": [
                                        "string"
                                    ],
                                    "Value": "кто такой джонни депп",
                                    "Type": "string",
                                    "Name": "query"
                                }
                            ],
                            "Name": "personal_assistant.scenarios.search"
                        }
                    ],
                    "Sources": ["AliceTagger"],
                    "Confidences": [1]
                }
            }
        })");
        const THashMap<TString, TMaybe<TString>> expFlags;
        const auto query = wizard.GetSearchQuery(expFlags);
        UNIT_ASSERT(query.Defined());
        UNIT_ASSERT_EQUAL(*query, TStringBuf("кто такой джонни депп"));
    }
    Y_UNIT_TEST(QueryFromRule) {
        auto wizard = WizardFromString(R"({
            "AliceResponse": {
                "AliceSearchQueryPreparer": {
                    "Frame": {
                        "Slots": [
                            {
                                "TypedValue": {
                                    "String": "кто такой джонни депп",
                                    "Type": "string"
                                },
                                "Name": "query"
                            }
                        ],
                        "Name": "alice.search_query"
                    }
                }
            }
        })");
        const THashMap<TString, TMaybe<TString>> expFlags{{"mm_use_search_query_prepare_rule", "1"}};
        const auto query = wizard.GetSearchQuery(expFlags);
        UNIT_ASSERT(query.Defined());
        UNIT_ASSERT_EQUAL(*query, TStringBuf("кто такой джонни депп"));
    }
    Y_UNIT_TEST(TranslatedResponse) {
        {
            auto wizard = WizardFromString(R"({
                "AliceResponse": {
                    "AliceNormalizer": {
                        "NormalizedRequest": "native request"
                    }
                }
            })");

            UNIT_ASSERT(wizard.GetNormalizedUtterance());
            UNIT_ASSERT_EQUAL(wizard.GetNormalizedUtterance().GetRef(), "native request");
            UNIT_ASSERT(!wizard.GetNormalizedTranslatedUtterance());
        }
        {
            auto wizard = WizardFromString(R"({
                "AliceResponse": {
                    "AliceNormalizer": {
                        "NormalizedRequest": "native request"
                    }
                },
                "TranslatedResponse": {
                    "AliceNormalizer": {
                        "NormalizedRequest": "translated request"
                    }
                }
            })");

            UNIT_ASSERT(wizard.GetNormalizedUtterance());
            UNIT_ASSERT_EQUAL(wizard.GetNormalizedUtterance().GetRef(), "native request");
            UNIT_ASSERT(wizard.GetNormalizedTranslatedUtterance());
            UNIT_ASSERT_EQUAL(wizard.GetNormalizedTranslatedUtterance().GetRef(), "translated request");
        }
    }
}

} // namespace
