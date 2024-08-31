#include "collected_entities.h"
#include "tagger_slot_matching.h"
#include <alice/library/unittest/message_diff.h>
#include <alice/begemot/lib/utils/form_to_frame.h>
#include <alice/begemot/lib/utils/frame_description.h>

#include <search/begemot/rules/alice/tagger/proto/alice_tagger.pb.h>
#include <search/begemot/rules/granet_config/proto/granet_config.pb.h>
#include <search/begemot/rules/occurrences/custom_entities/rule/proto/custom_entities.pb.h>

#include <alice/library/frame/builder.h>
#include <alice/library/json/json.h>
#include <alice/library/proto/proto.h>

#include <library/cpp/iterator/zip.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/file.h>

namespace NAlice {

namespace {

using namespace NGranet;
using namespace NBg;

TEntity MakeEntity(const ::NNlu::TInterval& interval, TStringBuf type, TStringBuf value) {
    TEntity entity;
    entity.Interval = interval;
    entity.Type = type;
    entity.Value = value;
    return entity;
}

#define CHECK_FORM_EQUALS_FRAME(form, frame)                                        \
    do {                                                                            \
        UNIT_ASSERT_VALUES_EQUAL(form.GetTags().size(), frame.GetSlots().size());   \
        for (const auto& [tag, slot] : Zip(form.GetTags(), frame.GetSlots())) {     \
            UNIT_ASSERT_VALUES_EQUAL(tag.GetName(), slot.GetName());                \
            const TRecognizedSlot formSlot = NBg::GetRecognizedSlot(tag);           \
            UNIT_ASSERT_VALUES_EQUAL(formSlot.Name, slot.GetName());                \
            UNIT_ASSERT_VALUES_EQUAL(formSlot.Variants.size() > 0, true);           \
            UNIT_ASSERT_VALUES_EQUAL(formSlot.Variants[0].Type, slot.GetType());    \
            UNIT_ASSERT_VALUES_EQUAL(formSlot.Variants[0].Value, slot.GetValue());  \
        }                                                                           \
    } while(false)

#define CHECK_TAGGER(taggerResult, granetEntities, collectedEntities, expectedFrame)                                    \
    do {                                                                                                                \
        NBg::NProto::TGranetForm gotForm;                                                                               \
        TSemanticFrame gotFrame;                                                                                        \
        UNIT_ASSERT(!taggerResult.GetPredictions().at(expectedFrame.GetName()).GetPrediction().empty());                \
        FillFormAndFrame(                                                                                               \
            taggerResult.GetPredictions().at(expectedFrame.GetName()),                                                  \
            granetEntities,                                                                                             \
            collectedEntities,                                                                                          \
            expectedFrame.GetName(),                                                                                    \
            FrameDescriptionMap.FindPtr(expectedFrame.GetName()),                                                       \
            /* intentsToForceRegularProcessing= */ {},                                                                  \
            &gotForm,                                                                                                   \
            &gotFrame                                                                                                   \
        );                                                                                                              \
        UNIT_ASSERT_MESSAGES_EQUAL(expectedFrame, gotFrame);                                                            \
        CHECK_FORM_EQUALS_FRAME(gotForm, gotFrame);                                                                     \
    } while (false)

NProto::TGranetConfig ReadGranetConfig() {
    TFileInput input(BinaryPath("alice/begemot/lib/tagger/ut/data/granet_config.ru.pb.txt"));
    return NAlice::ParseProtoText<NProto::TGranetConfig>(input.ReadAll());
}

struct TFixture : public NUnitTest::TBaseFixture {
    TFixture() {
        for (const auto& [name, description] : ReadFrameDescriptionMap(ReadGranetConfig(), /* taggerOnly= */ true)) {
            FrameDescriptionMap[name] = description;
        }
    }

    NAlice::TFrameDescriptionMap FrameDescriptionMap;
};

Y_UNIT_TEST_SUITE_F(TTaggerPredictionTestSuite, TFixture) {

Y_UNIT_TEST(PlayMusic) {
    const auto customEntitiesResult = JsonToProto<NBg::NProto::TCustomEntitiesResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Occurrences": {
                "Tokens": [ "включи", "коррозию", "металла" ],
                "Ranges": [ { "Begin": 0, "End": 1 }, { "Begin": 2, "End": 3 } ]
            },
            "Values": [
                {
                    "CustomEntityValues": [
                        { "Type": "video_selection_action", "Value": "play" },
                        { "Type": "launch_command", "Value": "launch_command" },
                        { "Type": "video_action", "Value": "play" },
                        { "Type": "action_request", "Value": "autoplay" }
                    ]
                },
                {
                    "CustomEntityValues": [
                        { "Type": "fm_radio_station", "Value": "Metal" },
                        { "Type": "genre", "Value": "metal" }
                    ]
                }
            ]
        }
    )"));
    const auto entities = CollectEntities(customEntitiesResult, {});
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NMusic::MUSIC_PLAY << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                { "Text": "включи", "Tag": "B-action_request" },
                                { "Text": "коррозию", "Tag": "B-search_text" },
                                { "Text": "металла", "Tag": "I-search_text" }
                            ],
                            "Probability": 0.8699440906
                        }
                    ]
                }
            }
        }
    )"));
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NMusic::MUSIC_PLAY}}
        .AddSlot("action_request", "action_request", "autoplay")
        .AddSlot("search_text", "string", "коррозию металла")
        .Build();
    CHECK_TAGGER(taggerResult, TVector<TEntity>{}, entities, expectedFrame);
}

Y_UNIT_TEST(PlayMusicWithConcatenation) {
        const auto customEntitiesResult = JsonToProto<NBg::NProto::TCustomEntitiesResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Occurrences": {
                "Tokens": [ "алишера", "моргенштерна", "включи", "дуло" ],
                "Ranges": [ {"Begin": 2, "End": 3} ]
            },
            "Values": [
                {
                    "CustomEntityValues": [
                        { "Type": "action_request", "Value": "autoplay" },
                        { "Type": "launch_command", "Value": "launch_command" },
                        { "Type": "video_action", "Value": "play" },
                        { "Type": "video_selection_action", "Value": "play" }
                    ]
                }
            ]
        }
    )"));
    const auto entities = CollectEntities(customEntitiesResult, {});
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NMusic::MUSIC_PLAY << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                { "Text": "алишера", "Tag": "B-search_text" },
                                { "Text": "моргенштерна", "Tag": "I-search_text" },
                                { "Text": "включи", "Tag": "B-action_request" },
                                { "Text": "дуло", "Tag": "I-search_text" }
                            ],
                            "Probability": 0.9988438123
                        }
                    ]
                }
            }
        }
    )"));
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NMusic::MUSIC_PLAY}}
            .AddSlot("action_request", "action_request", "autoplay")
            .AddSlot("search_text", "string", "алишера моргенштерна дуло")
            .Build();
    CHECK_TAGGER(taggerResult, TVector<TEntity>{}, entities, expectedFrame);
}

Y_UNIT_TEST(TagsFromDifferentPartsOfUtterance) {
    const auto customEntitiesResult = JsonToProto<NBg::NProto::TCustomEntitiesResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Occurrences": {
                "Tokens": ["включи","песню","детские","алиса","включи","детские","песни"],
                "Ranges": [{"Begin":0,"End":1},{"Begin":1,"End":2},{"Begin":2,"End":3},{"Begin":3,"End":4},{"Begin":4,"End":5},{"Begin":5,"End":6},{"Begin":6,"End":7}]
            },
            "Values": [
                {"CustomEntityValues":[{"Type":"action_request","Value":"autoplay"}]},
                {"CustomEntityValues":[{"Type":"catalog_section","Value":"track"},{"Type":"player_type","Value":"music"}]},
                {"CustomEntityValues":[{"Type":"genre","Value":"forchildren"}]},
                {"CustomEntityValues":[{"Type":"fairy_tale","Value":"album/3540304"}]},
                {"CustomEntityValues":[{"Type":"action_request","Value":"autoplay"}]},
                {"CustomEntityValues":[{"Type":"video_film_genre","Value":"childrens"},{"Type":"genre","Value":"forchildren"}]},
                {"CustomEntityValues":[{"Type":"player_type","Value":"music"},{"Type":"catalog_section","Value":"track"}]}
            ]
        }
    )"));
    const auto entities = CollectEntities(customEntitiesResult, {});
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NMusic::MUSIC_PLAY << R"(": {
                    "Prediction": [
                        {
                            "Token":[
                                {"Text":"включи", "Tag":"B-action_request"},
                                {"Text":"песню", "Tag":"O"},
                                {"Text":"детские", "Tag":"B-genre"},
                                {"Text":"алиса", "Tag":"O"},
                                {"Text":"включи", "Tag":"B-action_request"},
                                {"Text":"детские", "Tag":"B-genre"},
                                {"Text":"песни", "Tag":"O"}
                            ],
                            "Probability":0.9510204074
                        }
                    ]
                }
            }
        }
    )"));
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NMusic::MUSIC_PLAY}}
        .AddSlot("genre", "genre", "forchildren")
        .AddSlot("action_request", "action_request", "autoplay")
        .Build();
    CHECK_TAGGER(taggerResult, TVector<TEntity>{}, entities, expectedFrame);
}

Y_UNIT_TEST(ConsecutiveTagsOfSameType) {
    const auto customEntitiesResult = JsonToProto<NBg::NProto::TCustomEntitiesResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Occurrences": {
                "Tokens":["детские","любознательные","песни"],
                "Ranges":[{"Begin":0,"End":1},{"Begin":2,"End":3}]
            },
            "Values": [
                {
                    "CustomEntityValues":[
                        {"Type":"video_film_genre","Value":"childrens"},
                        {"Type":"genre","Value":"forchildren"},
                        {"Type":"tv_genre","Value":"children"}
                    ]
                },
                {
                    "CustomEntityValues":[
                        {"Type":"player_type","Value":"music"},
                        {"Type":"catalog_section","Value":"track"}
                    ]
                }
            ]
        }
    )"));
    const auto entities = CollectEntities(customEntitiesResult, {});
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NMusic::MUSIC_PLAY << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text":"детские","Tag":"B-genre"},
                                {"Text":"любознательные","Tag":"I-genre"},
                                {"Text":"песни","Tag":"O"}
                            ],
                            "Probability": 0.9596296076
                        }
                    ]
                }
            }
        }
    )"));
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NMusic::MUSIC_PLAY}}
        .AddSlot("genre", "genre", "forchildren")
        .Build();
    CHECK_TAGGER(taggerResult, TVector<TEntity>{}, entities, expectedFrame);
}

// ------------------------------ ALTERNATIVE MATCHING LOGIC ------------------------------

Y_UNIT_TEST(PlayItemName) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text": "найди", "Tag": "B-action"},
                                {"Text": "игру", "Tag": "B-search_text"},
                                {"Text": "престолов", "Tag": "I-search_text"}
                            ],
                            "Probability": 0.9985912269
                        },
                        {
                            "Token": [
                                {"Text": "найди", "Tag": "O"},
                                {"Text": "игру", "Tag": "B-search_text"},
                                {"Text": "престолов","Tag": "I-search_text"}
                            ],
                            "Probability": 0.0005612611546
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({0, 1}, "custom.list_todo_reminders_action", "list_todo_reminders_action"),
        MakeEntity({0, 1}, "custom.video_action", "find"),
        MakeEntity({1, 2}, "custom.popular_goods", "popular"),
        MakeEntity({1, 2}, "custom.news_topic", "games"),
        MakeEntity({1, 2}, "custom.fairy_tale", "track/24057267"),
        MakeEntity({1, 2}, "custom.external_skill_type", "skill_type"),
        MakeEntity({1, 2}, "custom.site_or_app", "site_or_app"),
        MakeEntity({1, 3}, "custom.popular_goods", "popular")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO}}
        .AddSlot("action", "video_action", "find")
        .AddSlot("search_text", "string", "игру престолов")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(VideoPlaySecondSeason) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"2","Tag":"B-season"},
                                {"Text":"сезон","Tag":"O"}
                            ],
                            "Probability": 0.9982254534
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({0, 1}, "custom.video_selection_action", "play"),
        MakeEntity({0, 1}, "custom.launch_command", "launch_command"),
        MakeEntity({0, 1}, "custom.video_action", "play"),
        MakeEntity({0, 1}, "custom.action_request", "autoplay"),
        MakeEntity({1, 2}, "custom.fuel_type", "a92"),
        MakeEntity({1, 2}, "fst.num", "2")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO}}
        .AddSlot("season", "num", "2")
        .AddSlot("action", "video_action", "play")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(VideoPlayLastSeason) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"последний","Tag":"B-season"},
                                {"Text":"сезон","Tag":"O"}
                            ],
                            "Probability": 0.998553535
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({0, 1}, "custom.video_selection_action", "play"),
        MakeEntity({0, 1}, "custom.launch_command", "launch_command"),
        MakeEntity({0, 1}, "custom.video_action", "play"),
        MakeEntity({0, 1}, "custom.action_request", "autoplay"),
        MakeEntity({1, 2}, "custom.video_new", "new_video"),
        MakeEntity({1, 2}, "custom.video_season", "last"),
        MakeEntity({1, 2}, "custom.novelty", "new"),
        MakeEntity({1, 2}, "custom.video_episode", "last")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO}}
        .AddSlot("season", "video_season", "last")
        .AddSlot("action", "video_action", "play")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(SkipTaggerPredictionsWithMismatchedSlotTypes) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-search_text"},
                                {"Text":"видео","Tag":"B-content_type"}
                            ],
                            "Probability": 0.1920554021
                        },
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-film_genre"},
                                {"Text":"видео","Tag":"B-content_type"}
                            ],
                            "Probability": 0.7954814579
                        },
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-content_type"},
                                {"Text":"видео","Tag":"I-content_type"}
                            ],
                            "Probability": 0.003740568417
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({0, 1}, "custom.video_selection_action", "play"),
        MakeEntity({0, 1}, "custom.launch_command", "launch_command"),
        MakeEntity({0, 1}, "custom.video_action", "play"),
        MakeEntity({0, 1}, "custom.action_request", "autoplay"),
        MakeEntity({2, 3}, "custom.video_content_type", "video"),
        MakeEntity({2, 3}, "custom.popular_goods", "popular"),
        MakeEntity({2, 3}, "custom.default_app", "gallery"),
        MakeEntity({2, 3}, "custom.player_type", "video")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO}}
        .AddSlot("content_type", "video_content_type", "video")
        .AddSlot("action", "video_action", "play")
        .AddSlot("search_text", "string", "развивающее")
        .Build();

    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(AllTextFormDescrpition) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO_TEXT << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-film_genre"},
                                {"Text":"видео","Tag":"B-content_type"}
                            ],
                            "Probability": 0.7954814579
                        },
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-search_text"},
                                {"Text":"видео","Tag":"B-content_type"}
                            ],
                            "Probability": 0.1920554021
                        },
                        {
                            "Token": [
                                {"Text":"включи","Tag":"B-action"},
                                {"Text":"развивающее","Tag":"B-content_type"},
                                {"Text":"видео","Tag":"I-content_type"}
                            ],
                            "Probability": 0.003740568417
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({0, 1}, "custom.video_selection_action", "play"),
        MakeEntity({0, 1}, "custom.launch_command", "launch_command"),
        MakeEntity({0, 1}, "custom.video_action", "play"),
        MakeEntity({0, 1}, "custom.action_request", "autoplay"),
        MakeEntity({2, 3}, "custom.video_content_type", "video"),
        MakeEntity({2, 3}, "custom.popular_goods", "popular"),
        MakeEntity({2, 3}, "custom.default_app", "gallery"),
        MakeEntity({2, 3}, "custom.player_type", "video")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO_TEXT}}
        .AddSlot("content_type", "string", "видео")
        .AddSlot("film_genre", "string", "развивающее")
        .AddSlot("action", "string", "включи")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(ReleaseDateSlot) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::SEARCH_VIDEO << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                { "Tag": "B-action", "Text": "покажи" },
                                { "Tag": "B-content_type", "Text": "фильмы" },
                                { "Tag": "B-release_date", "Text": "2016" },
                                { "Tag": "O", "Text": "года" }
                            ]
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({1, 2}, "custom.video_content_type", "movie"),
        MakeEntity({2, 3}, "fst.date", "{\"years\":2016}"),
        MakeEntity({0, 1}, "custom.video_action", "play")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::SEARCH_VIDEO}}
        .AddSlot("content_type", "video_content_type", "movie")
        .AddSlot("action", "video_action", "play")
        .AddSlot("release_date", "date", "{\"years\":2016}")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(VideoSelectionActionSlot) {
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(TStringBuilder{} << R"(
        {
            "Predictions": {
                ")" << NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT << R"(": {
                    "Prediction": [
                        {
                            "Token": [
                                { "Tag": "B-action", "Text": "покажи" },
                                { "Tag": "I-action", "Text": "описание" },
                                { "Tag": "B-video_text", "Text": "3" },
                                { "Tag": "O", "Text": "фильма" }
                            ]
                        }
                    ]
                }
            }
        }
    )"));
    const TVector<TEntity> entities = {
        MakeEntity({1, 2}, "custom.video_selection_action", "description")
    };
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NVideoCommon::QUASAR_SELECT_VIDEO_FROM_GALLERY_BY_TEXT}}
        .AddSlot("action", "video_selection_action", "description")
        .AddSlot("video_text", "string", "3")
        .Build();
    CHECK_TAGGER(taggerResult, entities, TCollectedEntities{}, expectedFrame);
}

Y_UNIT_TEST(LocationGetsFilledFromAliceEntitiesCollector) {
    const auto customEntitiesResult = JsonToProto<NBg::NProto::TCustomEntitiesResult>(JsonFromString(R"(
        {
            "Occurrences": {
                "Tokens":["включи","музыку","на","кухне"],
                "Ranges":[{"Begin":0,"End":1},{"Begin":1,"End":2},{"Begin":3,"End":4}]
            },
            "Values": [
                {
                    "CustomEntityValues":[
                        {"Type":"video_selection_action","Value":"play"},
                        {"Type":"video_action","Value":"play"},
                        {"Type":"launch_command","Value":"launch_command"},
                        {"Type":"action_request","Value":"autoplay"}
                    ]
                },
                {
                    "CustomEntityValues":[
                        {"Type":"news_topic","Value":"music"},
                        {"Type":"player_type","Value":"music"}
                    ]
                },
                {
                    "CustomEntityValues":[
                        {"Type":"popular_goods","Value":"popular"}
                    ]
                }
            ]
        }
    )"));
    const auto aliceEntitiesCollectorResult = JsonToProto<NBg::NProto::TAliceEntitiesCollectorResult>(JsonFromString(R"(
        {
            "Entities": [
                {"Begin":0,"End":1,"Type":"custom.video_selection_action","Value":"play"},
                {"Begin":0,"End":1,"Type":"custom.video_action","Value":"play"},
                {"Begin":0,"End":1,"Type":"custom.launch_command","Value":"launch_command"},
                {"Begin":0,"End":1,"Type":"custom.action_request","Value":"autoplay"},
                {"Begin":1,"End":2,"Type":"custom.news_topic","Value":"music"},
                {"Begin":1,"End":2,"Type":"custom.player_type","Value":"music"},
                {"Begin":3,"End":4,"Type":"custom.popular_goods","Value":"popular"},
                {"Begin":0,"End":1,"Type":"user.iot.bow_action","Value":"включать"},
                {"Begin":2,"End":3,"Type":"user.iot.bow_action","Value":"на"},
                {"Begin":2,"End":3,"Type":"user.iot.preposition","Value":"на"},
                {"Begin":3,"End":4,"Type":"user.iot.room","Value":"d4c620e7-4ac6-48ca-9ba4-4bfad3c61ef8"},
                {"Begin":2,"End":4,"Type":"sys.album","Value":"\"album\""},
                {"Begin":1,"End":2,"Type":"sys.films_100_750","Value":"\"movie\""},
                {"Begin":3,"End":4,"Type":"sys.films_100_750","Value":"\"movie\""},
                {"Begin":3,"End":4,"Type":"sys.films_50_filtered","Value":"\"movie\""},
                {"Begin":1,"End":2,"Type":"sys.soft","Value":"\"apple music\""},
                {"Begin":2,"End":4,"Type":"sys.track","Value":"\"track\""}
            ]
        }
    )"));
    const auto entities = CollectEntities(customEntitiesResult, aliceEntitiesCollectorResult);
    const auto taggerResult = JsonToProto<NBg::NProto::TAliceTaggerResult>(JsonFromString(R"(
        {
            "Predictions": {
                "personal_assistant.scenarios.music_play": {
                    "Prediction":[
                        {
                            "Token":[
                                {"Text":"включи","Tag":"B-action_request"},
                                {"Text":"музыку","Tag":"O"},
                                {"Text":"на","Tag":"O"},
                                {"Text":"кухне","Tag":"B-location"}
                            ],
                            "Probability":0.9979205002
                        },
                        {
                            "Token":[
                                {"Text":"включи","Tag":"B-action_request"},
                                {"Text":"музыку","Tag":"B-search_text"},
                                {"Text":"на","Tag":"O"},
                                {"Text":"кухне","Tag":"B-location"}
                            ],
                            "Probability":0.0005891172344
                        },
                        {
                            "Token":[
                                {"Text":"включи","Tag":"B-action_request"},
                                {"Text":"музыку","Tag":"B-search_text"},
                                {"Text":"на","Tag":"I-search_text"},
                                {"Text":"кухне","Tag":"I-search_text"}
                            ],
                            "Probability":0.0005371126158
                        }
                    ]
                }
            }
        }
    )"));
    const TSemanticFrame expectedFrame = TSemanticFrameBuilder{TString{NMusic::MUSIC_PLAY}}
            .AddSlot("location", "user.iot.room", "d4c620e7-4ac6-48ca-9ba4-4bfad3c61ef8")
            .AddSlot("action_request", "action_request", "autoplay")
            .Build();
    CHECK_TAGGER(taggerResult, TVector<TEntity>{}, entities, expectedFrame);
}

}

} // namespace

} // namespace NAlice
