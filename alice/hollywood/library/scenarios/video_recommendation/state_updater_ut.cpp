#include "state_updater.h"

#include <alice/hollywood/library/scenarios/video_recommendation/proto/video_recommendation_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/request/slot.h>
#include <alice/library/unittest/message_diff.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/vector.h>

namespace NAlice::NHollywood {

namespace {

const TString RESOURCES_DIR = "alice/hollywood/library/scenarios/video_recommendation/ut/data/";

constexpr TStringBuf DATABASE_PATH = "sample_video_base.txt";
constexpr TStringBuf EMBEDDER_CONFIG_PATH = "bigrams_v_20200101_config.json";
constexpr TStringBuf EMBEDDER_DATA_PATH = "bigrams_v_20200101.dssm";

TString GetResourcePath(const TStringBuf resourceName) {
    return BinaryPath(RESOURCES_DIR + resourceName);
}

const TString FRAME_NAME = "personal_assistant.scenarios.video_recommendation";

const TStateUpdater::TFormDescription FORM_DESCRIPTION = {
    {"film_genre", {"custom.video_film_genre"}},
    {"country", {"custom.video_recommendation_country"}},
    {"release_date", {"custom.year_adjective", "custom.year"}}
};

struct TTestTurn {
    TVector<TSlot> UpdatedSlots;
    TVector<TString> ExpectedAttentions;
    TMaybe<TString> ExpectedRequestedSlot;
    TVector<TString> ExpectedGallery;
};

using TTestCase = TVector<TTestTurn>;

const TVector<TTestCase> TESTS = {
    {
        {
            {},
            {"recommendation_ask_film_genre"},
            "film_genre",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"film_genre", "custom.video_film_genre", "fantasy"}},
            {"recommendation_ask_country"},
            "country",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"country", "custom.video_recommendation_country", "США"}},
            {"recommendation_ask_release_date"},
            "release_date",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"release_date", "custom.year", "1999"}},
            {},
            Nothing(),
            {
                "История игрушек 2"
            }
        }
    },
    {
        {
            {{"film_genre", "custom.video_film_genre", "fantasy"}},
            {"recommendation_ask_country"},
            "country",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"country", "custom.video_recommendation_country", "США"}},
            {"recommendation_ask_release_date"},
            "release_date",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"release_date", "custom.year", "1999"}},
            {},
            Nothing(),
            {
                "История игрушек 2"
            }
        }
    },
    {
        {
            {{"film_genre", "custom.video_film_genre", "fantasy"}},
            {"recommendation_ask_country"},
            "country",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"country", "custom.video_recommendation_country", "не знаю"}},
            {"recommendation_ask_release_date"},
            "release_date",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"release_date", "custom.year", "1999"}},
            {},
            Nothing(),
            {
                "История игрушек 2"
            }
        }
    },
    {
        {
            {{"film_genre", "custom.video_film_genre", "fantasy"}},
            {"recommendation_ask_country"},
            "country",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {},
            {"did_not_understand", "recommendation_ask_country"},
            "country",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"country", "custom.video_recommendation_country", "США"}},
            {"recommendation_ask_release_date"},
            "release_date",
            {
                "Красавица и чудовище",
                "Звёздные войны: Эпизод 6 – Возвращение Джедая",
                "Звёздные войны: Эпизод 5 – Империя наносит ответный удар"
            }
        },
        {
            {{"release_date", "custom.year", "1999"}},
            {},
            Nothing(),
            {
                "История игрушек 2"
            }
        }
    },
    {
        {
            {{"film_genre", "custom.video_film_genre", "comedy"}, {"release_date", "custom.year", "2000"}},
            {},
            Nothing(),
            {
                "Мерцающие огни"
            }
        }
    },
    {
        {
            {{"film_genre", "custom.video_film_genre", "comedy"}, {"release_date", "custom.year", "2020"}},
            {"empty_search_gallery"},
            Nothing(),
            {
            }
        }
    }
};

NScenarios::TScenarioRunRequest CreateRequestProto(bool isTvPluggedIn) {
    NScenarios::TScenarioRunRequest requestProto;

    requestProto.MutableInput()->AddSemanticFrames()->SetName(FRAME_NAME);
    requestProto.MutableBaseRequest()->MutableClientInfo()->SetAppId("ru.yandex.quasar");
    requestProto.MutableBaseRequest()->MutableInterfaces()->SetIsTvPlugged(isTvPluggedIn);

    return requestProto;
}

TStateUpdater CreateUpdater(const TVideoDatabase& database, const NScenarios::TScenarioRunRequest& requestProto) {
    return {FRAME_NAME, FORM_DESCRIPTION, database, requestProto, /* gallerySize= */ 3};
}

void FillUpdatedSlots(const TVector<TSlot>& updatedSlots, NScenarios::TScenarioRunRequest& requestProto,
                      THashSet<TString>& filledSlots) {
    TSemanticFrame& frame = *requestProto.MutableInput()->MutableSemanticFrames(0);
    for (const TSlot& updatedSlot : updatedSlots) {
        bool wasUpdated = false;
        for (auto& oldSlot : *frame.MutableSlots()) {
            if (oldSlot.GetName() == updatedSlot.Name) {
                oldSlot.MutableTypedValue()->SetType(updatedSlot.Type);
                oldSlot.MutableTypedValue()->SetString(updatedSlot.Value.AsString());
                oldSlot.SetIsFilled(true);
                wasUpdated = true;
                break;
            }
        }
        if (!wasUpdated) {
            auto& slot = *frame.AddSlots();
            slot.SetName(updatedSlot.Name);
            slot.MutableTypedValue()->SetType(updatedSlot.Type);
            slot.MutableTypedValue()->SetString(updatedSlot.Value.AsString());
        }
        filledSlots.insert(updatedSlot.Name);
    }
}

void CheckSlots(const TMaybe<TString>& expectedRequestedSlot, const TSlots& slots,
                const THashSet<TString>& filledSlots) {
    if (expectedRequestedSlot) {
        UNIT_ASSERT_VALUES_EQUAL(filledSlots.size() + 1, slots.size());
    } else {
        UNIT_ASSERT_VALUES_EQUAL(filledSlots.size(), slots.size());
    }

    for (const auto& slot : slots) {
        if (!filledSlots.contains(slot.GetName())) {
            UNIT_ASSERT(expectedRequestedSlot);
            UNIT_ASSERT_VALUES_EQUAL(slot.GetName(), *expectedRequestedSlot);
        }
    }
}

void CheckGallery(const TVector<TString>& expectedItems,
                  const TMaybe<NScenarios::TShowGalleryDirective>& galleryDirective) {
    UNIT_ASSERT_VALUES_EQUAL(galleryDirective.Empty(), expectedItems.empty());

    if (galleryDirective.Empty()) {
        return;
    }

    const auto& items = galleryDirective->GetItems();

    UNIT_ASSERT_VALUES_EQUAL(items.size(), expectedItems.size());
    for (size_t i = 0; i < expectedItems.size(); ++i) {
        UNIT_ASSERT_VALUES_EQUAL(items[i].GetName(), expectedItems[i]);
    }
}

void CheckAttentions(const TVector<TString>& expectedAttentions, const NJson::TJsonValue& attentions) {
    const auto& attentionsMap = attentions.GetMap();
    UNIT_ASSERT_VALUES_EQUAL(expectedAttentions.size(), attentionsMap.size());

    for (const auto& attention : expectedAttentions) {
        UNIT_ASSERT(attentionsMap.contains(attention));
    }
}

void UpdateRequestProto(const TSemanticFrame& semanticFrame, const TVideoRecommendationState& state,
                        NScenarios::TScenarioRunRequest& requestProto)
{
    auto* requestInput = requestProto.MutableInput();
    if (requestInput->SemanticFramesSize() == 0) {
        *requestInput->AddSemanticFrames() = semanticFrame;
    } else {
        *requestInput->MutableSemanticFrames(0) = semanticFrame;
    }
    requestProto.MutableBaseRequest()->MutableState()->PackFrom(state);
}

} // namespace

Y_UNIT_TEST_SUITE(TDialogTestSuite) {
    Y_UNIT_TEST(TestDialog) {
        TVideoDatabase database;
        database.LoadFromPaths(GetResourcePath(DATABASE_PATH), GetResourcePath(EMBEDDER_DATA_PATH),
                               GetResourcePath(EMBEDDER_CONFIG_PATH));

        for (const auto& test : TESTS) {
            NScenarios::TScenarioRunRequest requestProto = CreateRequestProto(/* isTvPluggedIn= */ true);

            THashSet<TString> filledSlots;

            for (size_t turnIndex = 0; turnIndex < test.size(); ++turnIndex) {
                const TTestTurn& turn = test[turnIndex];
                const bool hasFrame = !turn.UpdatedSlots.empty();
                if (turnIndex > 0 && !hasFrame) {
                    requestProto.MutableInput()->ClearSemanticFrames();
                } else {
                    FillUpdatedSlots(turn.UpdatedSlots, requestProto, filledSlots);
                }

                TStateUpdater updater = CreateUpdater(database, requestProto);
                updater.Update();

                const auto& resultSemanticFrame = updater.GetSemanticFrame();
                const auto& resultState = updater.GetState();

                const auto& lastFrame =
                    resultState.GetStateHistory(resultState.StateHistorySize() - 1).GetSemanticFrame();
                UNIT_ASSERT_MESSAGES_EQUAL(lastFrame, resultSemanticFrame);

                CheckSlots(turn.ExpectedRequestedSlot, resultSemanticFrame.GetSlots(), filledSlots);
                CheckGallery(turn.ExpectedGallery, updater.GetShowGalleryDirective());
                CheckAttentions(turn.ExpectedAttentions, updater.GetNlgContext()["attentions"]);

                UpdateRequestProto(resultSemanticFrame, resultState, requestProto);
            }
        }
    }

    Y_UNIT_TEST(TestCannotWorkWithoutSemanticFrame) {
        TVideoDatabase database;
        database.LoadFromPaths(GetResourcePath(DATABASE_PATH), GetResourcePath(EMBEDDER_DATA_PATH),
                               GetResourcePath(EMBEDDER_CONFIG_PATH));

        UNIT_ASSERT_EXCEPTION_CONTAINS(CreateUpdater(database, NScenarios::TScenarioRunRequest()),
            yexception, "SemanticFrame wasn't found neither in request nor in state");
    }
}

} // namespace NAlice::NHollywood
