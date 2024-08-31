#include "handle.h"

#include <alice/hollywood/library/scenarios/suggesters/common/state_updater.h>
#include <alice/hollywood/library/scenarios/suggesters/games/proto/game_suggest_state.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/frame/builder.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/stream/file.h>
#include <util/string/builder.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "alice/hollywood/library/scenarios/suggesters/games/ut/data";

const TString FRAME = "alice.game_suggest";
const TString CONFIRM_ACTION = "confirm";
const TString DECLINE_BY_FRAME_ACTION = "decline_by_frame";

const TString CONFIRM_GRANET = "alice.game_suggest.confirm";
const TString DECLINE_GRANET = "alice.game_suggest.decline";

const TString CANNOT_RECOMMEND_ANYMORE = "Извините, я предложила вам всё, что могу.";
const TString IS_IRRELEVANT = "Что-то пошло не так.";

constexpr TStringBuf SAMPLE_RECOMMENDER_DATA = TStringBuf(R"([
  {
    "item_id": "cd052c55-d16b-470b-bbf1-d4e78cb4f1f8",
    "name": "Кто будет миллионером?",
    "responses": [
      {
        "text": "Сыграем в Игру кто будет миллионером?"
      }
    ]
  },
  {
    "item_id": "f80f9b78-18cf-4a91-9d1b-96e32dfc52e0",
    "name": "Угадай персонажа",
    "responses": [
      {
        "text": "Попробуем сыграть в Угадай персонажа?"
      }
    ]
  },
])");

NScenarios::TScenarioRunRequest BuildRequestProto(const TMaybe<TGameSuggestState>& state = Nothing(),
                                                  const TMaybe<TSemanticFrame>& frame = Nothing())
{
    NScenarios::TScenarioRunRequest requestProto;

    if (frame) {
        *requestProto.MutableInput()->AddSemanticFrames() = *frame;
    } else {
        requestProto.MutableInput()->AddSemanticFrames()->SetName(FRAME);
    }

    if (state) {
        requestProto.MutableBaseRequest()->MutableState()->PackFrom(*state);
    }

    return requestProto;
}

using MultistepTestRunner = TStateUpdater<TGameRecommender, TGameSuggestState>;

TMaybe<TString> GetText(const NScenarios::TScenarioRunResponse* response) {
    if (!response) {
        return Nothing();
    }
    return response->GetResponseBody().GetLayout().GetCards()[0].GetText();
}

TSemanticFrame GetExpectedConfirmFrame(const TString& expectedSuggestionId) {
    const auto expectedSlot = MakeSlot(/* name= */ "fixed_skill_id",
                                       /* acceptedTypes= */ {},
                                       /* type= */ "ActivationPhraseExternalSkillId",
                                       /* value */ expectedSuggestionId);
    return MakeFrame(/* name= */ "alice.external_skill_fixed_activate", /* slots= */ {expectedSlot});
}

TSemanticFrame GetExpectedDeclineFrame() {
    const auto expectedSlot = MakeSlot(/* name= */ "decline", /* acceptedTypes= */ {},
                                       /* type= */ Nothing(), /* value */ Nothing());
    return MakeFrame(/* name= */ "alice.game_suggest", /* slots= */ {expectedSlot});
}

TMaybe<TString> CheckAction(const TString& expectedGranet, const TSemanticFrame& expectedFrame,
                            const NScenarios::TFrameAction& action)
{
    if (action.GetNluHint().GetFrameName() != expectedGranet) {
        return TStringBuilder{} << "Invalid action granet. Expected: " << expectedGranet
            << ", Found" << action.GetNluHint().GetFrameName();
    }

    TSemanticFrame frame = action.GetFrame();
    if (const auto callbackFrame = GetCallbackFrame(action.HasCallback() ? &action.GetCallback() : nullptr)) {
        frame = callbackFrame->ToProto();
    }

    NAlice::TMessageDiff diff(expectedFrame, frame);
    if (!diff.AreEqual) {
        return TStringBuilder{} << "Invalid action effect. Diff: " << diff.Diff;
    }

    return Nothing();
}

TMaybe<TString> CheckNavigationActions(const TString& expectedSuggestionId,
                                       const NScenarios::TScenarioRunResponse* response)
{
    if (!response) {
        return "Response was not provided";
    }

    const auto& frameActions = response->GetResponseBody().GetFrameActions();

    const auto& confirmAction = frameActions.at(CONFIRM_ACTION);
    const auto& declineAction = frameActions.at(DECLINE_BY_FRAME_ACTION);

    const auto expectedConfirmFrame = GetExpectedConfirmFrame(expectedSuggestionId);
    const auto expectedDeclineFrame = GetExpectedDeclineFrame();

    if (const auto error = CheckAction(CONFIRM_GRANET, expectedConfirmFrame, confirmAction)) {
        return error;
    } else if (const auto error = CheckAction(DECLINE_GRANET, expectedDeclineFrame, declineAction)) {
        return error;
    }

    return Nothing();
}

void CheckTurn(const TString& expectedText, const TMaybe<TString>& expectedId, MultistepTestRunner& runner) {
    const auto requestProto = BuildRequestProto(runner.GetState(), runner.GetFrame());
    NAppHost::NService::TTestContext serviceCtx;
    const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
    const auto response = runner.ProcessRequest(request);

    const TString errorText = TStringBuilder{} << "Failed on " << expectedText;

    UNIT_ASSERT_C(response, errorText);
    UNIT_ASSERT_VALUES_EQUAL_C(response->GetResponseBody().GetSemanticFrame().GetName(), FRAME, errorText);
    UNIT_ASSERT_VALUES_EQUAL_C(GetText(response.get()), expectedText, errorText);

    if (expectedId) {
        const auto maybeError = CheckNavigationActions(*expectedId, response.get());
        UNIT_ASSERT_C(!maybeError, "Invalid actions: " << *maybeError << ". " << errorText);
    } else {
        UNIT_ASSERT(!response->GetResponseBody().GetFrameActions().count(CONFIRM_ACTION));
        UNIT_ASSERT(!response->GetResponseBody().GetFrameActions().count(DECLINE_BY_FRAME_ACTION));
    }
}

} // namespace

Y_UNIT_TEST_SUITE(TGamesSuggestSuite) {
    Y_UNIT_TEST(TestSuggestions) {
        const auto jsonData = JsonFromString(SAMPLE_RECOMMENDER_DATA);

        TGameRecommender recommender;
        recommender.LoadFromJson(jsonData);

        MultistepTestRunner runner(recommender, BuildGamesSuggestConfig());

        for (const auto& suggestion : jsonData.GetArray()) {
            CheckTurn(suggestion["responses"][0]["text"].GetString(), suggestion["item_id"].GetString(), runner);
        }
        CheckTurn(CANNOT_RECOMMEND_ANYMORE, /* expectedId= */ Nothing(), runner);
    }

    Y_UNIT_TEST(TestMultistepRecommendations) {
        TFsPath dataDirPath(BinaryPath(DATA_PATH));
        TFileInput inputStream(dataDirPath / "game_suggestions.json");
        const NJson::TJsonValue suggestions = JsonFromString(inputStream.ReadAll());

        TGameRecommender recommender;
        recommender.LoadFromJson(suggestions);

        MultistepTestRunner runner(recommender, BuildGamesSuggestConfig());

        for (const auto& suggestion : suggestions.GetArray()) {
            CheckTurn(suggestion["responses"][0]["text"].GetString(), suggestion["item_id"].GetString(), runner);
        }
        CheckTurn(CANNOT_RECOMMEND_ANYMORE, /* expectedId= */ Nothing(), runner);
    }

    Y_UNIT_TEST(TestIrrelevantResponse) {
        TGameRecommender recommender;
        recommender.LoadFromJson(JsonFromString(SAMPLE_RECOMMENDER_DATA));

        MultistepTestRunner runner(recommender, BuildGamesSuggestConfig());

        const TSemanticFrame emptyFrame;
        const auto requestWithoutFrameProto = BuildRequestProto(/* state= */ Nothing(), /* frame= */ emptyFrame);
        NAppHost::NService::TTestContext serviceCtx;
        const TScenarioRunRequestWrapper request{requestWithoutFrameProto, serviceCtx};
        const auto response = runner.ProcessRequest(request);

        UNIT_ASSERT(response->GetFeatures().GetIsIrrelevant());
        UNIT_ASSERT_VALUES_EQUAL(GetText(response.get()), IS_IRRELEVANT);
    }
}

} // namespace NAlice::NHollywood
