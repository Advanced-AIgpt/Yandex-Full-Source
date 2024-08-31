#include "handle.h"

#include <alice/hollywood/library/scenarios/suggesters/movies/proto/movie_suggest_state.pb.h>

#include <alice/hollywood/library/scenarios/suggesters/common/state_updater.h>
#include <alice/hollywood/library/scenarios/suggesters/nlg/register.h>

#include <alice/hollywood/library/context/context.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <alice/library/proto/protobuf.h>
#include <alice/library/unittest/message_diff.h>
#include <alice/library/video_common/defs.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/resource/resource.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/folder/path.h>
#include <util/stream/file.h>

namespace NAlice::NHollywood {

namespace {

constexpr TStringBuf DATA_PATH = "alice/hollywood/library/scenarios/suggesters/movies/ut/data/ut";

const TString FRAME = "alice.movie_suggest";

const TString GUMP = "Форрест Гамп";
const TString GUMP_SUGGESTION = "Хотите посмотреть «Форреста Гампа»? Говорят, это один из тех случаев,"
                                " когда фильм оказался в разы лучше книги. Ну что, включаю?";

const TString WOLF = "Волк с Уолл-стрит";
const TString WOLF_SUGGESTION = "Хотите посмотреть фильм «Волк с Уолл-стрит»?"
                                " Один из эпизодов с Леонардо ДиКаприо помог фильму"
                                " выиграть премию за самый безумный эпизод.";

const TString UP = "Вверх";
const TString UP_SUGGESTION = "Хотите посмотреть мультфильм «Вверх»? Чудесная история о дружбе, любви и мечтах.";
const TString UP_PERSUASION = "Но я так хотела посмотреть его, это же мой любимый мультик!";

const TString CANNOT_RECOMMEND_ANYMORE = "Извините, я показала вам всё, что могу предложить.";

TMovieRecommender LoadMovieRecommender() {
    TMovieRecommender recommender;
    recommender.LoadFromPath(BinaryPath(DATA_PATH));

    return recommender;
}

NScenarios::TScenarioRunRequest BuildRequestProto(bool isTvPluggedIn, EContentSettings contentSettings,
                                                  const TMaybe<TMovieSuggestState>& state = Nothing(),
                                                  const TMaybe<TSemanticFrame>& frame = Nothing())
{
    NScenarios::TScenarioRunRequest requestProto;

    if (frame) {
        *requestProto.MutableInput()->AddSemanticFrames() = *frame;
    } else {
        requestProto.MutableInput()->AddSemanticFrames()->SetName(FRAME);
    }

    auto& baseRequest = *requestProto.MutableBaseRequest();
    baseRequest.MutableClientInfo()->SetAppId("ru.yandex.quasar");
    baseRequest.MutableInterfaces()->SetIsTvPlugged(isTvPluggedIn);
    baseRequest.MutableDeviceState()->MutableDeviceConfig()->SetContentSettings(contentSettings);
    baseRequest.MutableOptions()->SetFiltrationLevel(1);

    if (state) {
        baseRequest.MutableState()->PackFrom(*state);
    }

    return requestProto;
}

struct TTestCase {
    EContentSettings ContentSettings = EContentSettings::without;
    TMaybe<TString> ExpectedItemName;
    bool IsTvPluggedIn = false;
    bool HasChildrenContent = false;
    bool ExpectedIsIrrelevant = false;
};

// Only the first two fields are filled, other values are generated in GenerateTestCases()
TVector<TTestCase> RAW_TEST_CASES = {
    {EContentSettings::children, UP},
    {EContentSettings::medium, GUMP},
    {EContentSettings::without, WOLF},
};

TVector<TTestCase> GenerateTestCases() {
    TVector<TTestCase> testCases;

    for (bool isTvPluggedIn : {false, true}) {
        for (bool hasChildrenContent : {false, true}) {
            for (auto testCase : RAW_TEST_CASES) {
                const bool cannotShowToChildren =
                    !hasChildrenContent && testCase.ContentSettings == EContentSettings::children;

                const bool isIrrelevant = !isTvPluggedIn || cannotShowToChildren;

                testCase.IsTvPluggedIn = isTvPluggedIn;
                testCase.HasChildrenContent = hasChildrenContent;
                testCase.ExpectedIsIrrelevant = isIrrelevant;

                if (isIrrelevant) {
                    testCase.ExpectedItemName = Nothing();
                }

                testCases.push_back(std::move(testCase));
            }
        }
    }

    return testCases;
}

TMaybe<NScenarios::TShowVideoDescriptionDirective> GetShowDescriptionDirective(
    const NScenarios::TScenarioRunResponse* response)
{
    if (!response) {
        return Nothing();
    }

    for (const auto& directive : response->GetResponseBody().GetLayout().GetDirectives()) {
        if (directive.HasShowVideoDescriptionDirective()) {
            return directive.GetShowVideoDescriptionDirective();
        }
    }

    return Nothing();
}

TMaybe<TString> GetItemName(const NScenarios::TScenarioRunResponse* response) {
    if (const auto maybeDescriptionDirective = GetShowDescriptionDirective(response)) {
        const auto& item = maybeDescriptionDirective->GetItem();
        return item.GetName();
    }

    return Nothing();
}

TMaybe<TString> GetVoice(const NScenarios::TScenarioRunResponse* response) {
    if (!response) {
        return Nothing();
    }

    return response->GetResponseBody().GetLayout().GetOutputSpeech();
}

using MultistepTestRunner = TStateUpdater<TMovieRecommender, TMovieSuggestState>;

} // namespace

Y_UNIT_TEST_SUITE(TMovieSuggestSuite) {
    Y_UNIT_TEST(TestMovieSuggest) {
        NAlice::TFakeRng rng;
        TCompiledNlgComponent nlg(rng, nullptr, &Register);

        const auto scenarioConfig = BuildMovieSuggestConfig();

        for (const auto& testCase : GenerateTestCases()) {
            TFsPath dataDirPath(BinaryPath(DATA_PATH));
            TFileInput inputStream(dataDirPath / "suggestions.json");
            NJson::TJsonValue recommendationsJson = JsonFromString(inputStream.ReadAll());

            if (!testCase.HasChildrenContent) {
                recommendationsJson.EraseValue(2); // Remove "Вверх"
            }

            TMovieRecommender recommender;
            recommender.LoadFromJson(recommendationsJson);

            const auto requestProto = BuildRequestProto(testCase.IsTvPluggedIn, testCase.ContentSettings);
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            TNlgWrapper nlgWrapper = TNlgWrapper::Create(nlg, request, rng, ELanguage::LANG_RUS);

            const auto response = BuildResponse(request, recommender, scenarioConfig, rng,
                                                TRTLogger::NullLogger(), nlgWrapper);

            UNIT_ASSERT_VALUES_EQUAL(response->GetFeatures().GetIsIrrelevant(), testCase.ExpectedIsIrrelevant);
            UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), testCase.ExpectedItemName);
        }
    }

    Y_UNIT_TEST(TestUniqueMultistepRecommendations) {
        TMovieRecommender recommender = LoadMovieRecommender();

        MultistepTestRunner runner(recommender, BuildMovieSuggestConfig());

        const EContentSettings contentSettings = EContentSettings::without;

        const TVector<TMaybe<TString>> expectedRecommendations = {WOLF, GUMP, UP, Nothing()};
        const TVector<TString> expectedVoices =
            {WOLF_SUGGESTION, GUMP_SUGGESTION, UP_SUGGESTION, CANNOT_RECOMMEND_ANYMORE};

        for (size_t i = 0; i < expectedRecommendations.size(); ++i) {
            const auto requestProto = BuildRequestProto(/* isTvPluggedIn= */ true, contentSettings,
                                                        runner.GetState(), runner.GetFrame());
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            const auto response = runner.ProcessRequest(request);

            UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), expectedRecommendations[i]);
            UNIT_ASSERT_VALUES_EQUAL(GetVoice(response.get()), expectedVoices[i]);
        }
    }

    Y_UNIT_TEST(TestPersuasions) {
        TMovieRecommender recommender = LoadMovieRecommender();

        MultistepTestRunner runner(recommender, BuildMovieSuggestConfig());

        const EContentSettings contentSettings = EContentSettings::children;
        const auto experiments = TProtoStructBuilder{}.SetNull("max_persuasion_count=1").Build();

        const TVector<TString> expectedVoices = {UP_SUGGESTION, UP_PERSUASION};
        for (size_t stepIndex = 0; stepIndex < expectedVoices.size(); ++stepIndex) {
            const auto& expectedVoice = expectedVoices[stepIndex];
            auto requestProto = BuildRequestProto(/* isTvPluggedIn= */ true, contentSettings,
                                                  runner.GetState(), runner.GetFrame());
            *requestProto.MutableBaseRequest()->MutableExperiments() = experiments;
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            const auto response = runner.ProcessRequest(request);

            UNIT_ASSERT_VALUES_EQUAL(GetVoice(response.get()), expectedVoice);
            if (stepIndex == 0) {
                UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), UP);
            } else {
                UNIT_ASSERT(!GetShowDescriptionDirective(response.get()));
            }
        }
    }

    Y_UNIT_TEST(TestCopySlots) {
        const EContentSettings contentSettings = EContentSettings::without;

        TSemanticFrame frame;
        frame.SetName(FRAME);

        TSemanticFrame::TSlot& contentTypeSlot = *frame.AddSlots();
        contentTypeSlot.SetName(TString{NVideoCommon::SLOT_CONTENT_TYPE});
        contentTypeSlot.SetType(TString{NVideoCommon::SLOT_CONTENT_TYPE_TYPE});
        contentTypeSlot.AddAcceptedTypes(TString{NVideoCommon::SLOT_CONTENT_TYPE_TYPE});
        contentTypeSlot.SetValue("movie");
        contentTypeSlot.MutableTypedValue()->SetType(TString{NVideoCommon::SLOT_CONTENT_TYPE_TYPE});
        contentTypeSlot.MutableTypedValue()->SetString("movie");

        TMovieRecommender recommender = LoadMovieRecommender();
        MultistepTestRunner runner(recommender, BuildMovieSuggestConfig(), frame);

        const TVector<TMaybe<TString>> expectedRecommendations = {WOLF, GUMP, Nothing()};
        const TVector<TString> expectedVoices = {WOLF_SUGGESTION, GUMP_SUGGESTION, CANNOT_RECOMMEND_ANYMORE};

        for (size_t i = 0; i < expectedRecommendations.size(); ++i) {
            const auto requestProto = BuildRequestProto(/* isTvPluggedIn= */ true, contentSettings,
                                                        runner.GetState(), runner.GetFrame());
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            const auto response = runner.ProcessRequest(request);

            UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), expectedRecommendations[i]);
            UNIT_ASSERT_VALUES_EQUAL(GetVoice(response.get()), expectedVoices[i]);

            if (!runner.GetFrame()) {
                UNIT_ASSERT(!expectedRecommendations[i]);
            } else {
                UNIT_ASSERT_VALUES_EQUAL(runner.GetFrame()->GetName(), FRAME);
                for (const auto& slot : runner.GetFrame()->GetSlots()) {
                    if (slot.GetName() == NVideoCommon::SLOT_CONTENT_TYPE) {
                        UNIT_ASSERT_MESSAGES_EQUAL(slot, contentTypeSlot);
                    }
                }
            }
        }
    }

    Y_UNIT_TEST(TestContentTypeByGenre) {
        TMovieRecommender recommender = LoadMovieRecommender();

        {
            TSemanticFrame implicitMovieFrame;
            implicitMovieFrame.SetName(FRAME);

            TSemanticFrame::TSlot& comedySlot = *implicitMovieFrame.AddSlots();
            comedySlot.SetName(TString{NVideoCommon::SLOT_GENRE});
            comedySlot.SetType(TString{NVideoCommon::SLOT_GENRE_TYPE});
            comedySlot.SetValue(ToString(NVideoCommon::EVideoGenre::Comedy));

            MultistepTestRunner runner(recommender, BuildMovieSuggestConfig());
            const EContentSettings contentSettings = EContentSettings::without;

            auto requestProto = BuildRequestProto(/* isTvPluggedIn= */ true, contentSettings,
                                                  runner.GetState(), implicitMovieFrame);
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            const auto response = runner.ProcessRequest(request);

            UNIT_ASSERT_VALUES_EQUAL(GetVoice(response.get()), WOLF_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), WOLF);
        }

        {
            TSemanticFrame implicitCartoonFrame;
            implicitCartoonFrame.SetName(FRAME);

            TSemanticFrame::TSlot& comedySlot = *implicitCartoonFrame.AddSlots();
            comedySlot.SetName(TString{NVideoCommon::SLOT_GENRE});
            comedySlot.SetType(TString{NVideoCommon::SLOT_GENRE_TYPE});
            comedySlot.SetValue(ToString(NVideoCommon::EVideoGenre::Childrens));

            MultistepTestRunner runner(recommender, BuildMovieSuggestConfig());
            const EContentSettings contentSettings = EContentSettings::without;

            auto requestProto = BuildRequestProto(/* isTvPluggedIn= */ true, contentSettings,
                                                  runner.GetState(), implicitCartoonFrame);
            NAppHost::NService::TTestContext serviceCtx;
            const TScenarioRunRequestWrapper request{requestProto, serviceCtx};
            const auto response = runner.ProcessRequest(request);

            UNIT_ASSERT_VALUES_EQUAL(GetVoice(response.get()), UP_SUGGESTION);
            UNIT_ASSERT_VALUES_EQUAL(GetItemName(response.get()), UP);
        }
    }
}

} // namespace NAlice::NHollywood
