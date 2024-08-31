#include <alice/library/util/rng.h>
#include <alice/hollywood/library/scenarios/music/nlg/register.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NNlg::NTesting;

namespace {

const auto REG = &NAlice::NHollywood::NLibrary::NScenarios::NMusic::NNlg::RegisterAll;

} // namespace

Y_UNIT_TEST_SUITE(MusicPlay) {
    Y_UNIT_TEST(Unauthorized) {
        TestPhrase(REG, "music_play", "render_unauthorized", "Вы не авторизовались.");
    }

    Y_UNIT_TEST(CommonError) {
        TestPhrase(REG, "music_play", "external_skill_deactivated",
                   "Прошу прощения, но в данный момент этот диалог выключен.");
    }

    Y_UNIT_TEST(MusicCommon) {
        TestPhrase(REG, "music_play", "render_suggest_caption__authorize", "Авторизоваться");
    }

    Y_UNIT_TEST(MusicCommonContext) {
        const TString explicit1 =
            "А такую музыку я знаю, но поставить в детском режиме не могу.";
        const TString explicit2 =
            "А такую музыку я поставить бы и рада, но у вас включён детский режим поиска.";

        const auto context = NJson::TJsonMap({
            {"is_alarm_set_with_sound_intent", NJson::TJsonValue(true)},
            {"error", NJson::TJsonMap({
                {"data", NJson::TJsonMap({
                    {"code", NJson::TJsonValue("forbidden-content")},
                })},
            })},
        });

        CheckChoiceFreqs(REG, "music_play", "render_error__musicerror",
                         {{explicit1, 0.5}, {explicit2, 0.5}},
                         context);
    }
}

Y_UNIT_TEST_SUITE(WhatIsPlaying) {
    Y_UNIT_TEST(FairyTale) {
        NJson::TJsonValue context;
        context["is_smart_speaker"] = true;
        context["answer"]["title"] = "О попе и о работнике его балде";
        context["answer"]["type"] = "track";
        context["answer"]["genre"] = "fairytales";
        context["attentions"].SetType(NJson::JSON_MAP);

        TestPhraseTextVoice(REG, "player_what_is_playing", "render_result",
                            /* expectedText = */ "Сейчас играет сказка \"О попе и о работнике его балде\"..",
                            /* expectedVoice = */ "<[domain music]> Сейчас играет сказка \"О попе и о работнике его балде\".. <[/domain]>",
                            std::move(context));
    }
}
