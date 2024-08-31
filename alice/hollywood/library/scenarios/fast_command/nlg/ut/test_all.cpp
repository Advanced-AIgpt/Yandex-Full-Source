#include <alice/hollywood/library/scenarios/fast_command/nlg/register.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NNlg::NTesting;

namespace {

class TFastCommandNlgFixture : public NUnitTest::TBaseFixture {
public:
    explicit TFastCommandNlgFixture() {
        NlgRenderer = CreateTestingLocalizedNlgRenderer(
            &NAlice::NHollywood::NLibrary::NScenarios::NFastCommand::NNlg::RegisterAll,
            TFsPath(ArcadiaSourceRoot()) / "alice/hollywood/shards/all/prod/common_resources/nlg_translations.json");
    }
public:
    NNlg::INlgRendererPtr NlgRenderer;
};

} // namespace

Y_UNIT_TEST_SUITE_F(Stop, TFastCommandNlgFixture) {
    Y_UNIT_TEST(RenderIsNotSmartSpeaker) {
        TestPhrase(*NlgRenderer, "pause_command", "render_is_not_smart_speaker", "Поняла.");
    }

    Y_UNIT_TEST(RenderNavigatorCancel) {
        TestPhrase(*NlgRenderer, "pause_command", "render_navigator_cancel_confirmation", "Хорошо.");
    }
}

Y_UNIT_TEST_SUITE_F(Sound, TFastCommandNlgFixture) {
    Y_UNIT_TEST(RenderGetSound) {
        const auto context = NJson::TJsonMap({
            {"form", NJson::TJsonMap({
                {"level", NJson::TJsonValue(8)},
            })},
        });

        TestPhrase(*NlgRenderer, "sound_get_level", "render_result", "8", context);
    }

    Y_UNIT_TEST(RenderSetSound) {
        const TStringBuf result =
            TStringBuf("Для установки громкости, скажите, к примеру, \"поставь громкость на 9\".");
        TestPhrase(*NlgRenderer, "sound_set_level", "ask__level", result);
    }

    Y_UNIT_TEST(RenderSetSoundArabic) {
        const TStringBuf expectedText = TStringBuf("لضبط مستوى الصوت، قل، على سبيل المثال، \"اضبطي مستوى الصوت على 8\".");
        const TStringBuf expectedVoice = TStringBuf("<speaker voice=\"arabic.gpu\" lang=\"ar\">لضبط مستوى الصوت، قل، على سبيل المثال، \"اضبطي مستوى الصوت على 8\".");
        TestPhraseTextVoice(*NlgRenderer, "sound_set_level", "ask__level", expectedText, expectedVoice,
            NJson::TJsonMap(), NJson::TJsonMap(), NJson::TJsonMap(), ELanguage::LANG_ARA);
    }

    Y_UNIT_TEST(RenderSoundErrors) {
        const auto context1 = NJson::TJsonMap({
            {"error", NJson::TJsonMap({
                {"data", NJson::TJsonMap({
                    {"code", NJson::TJsonValue("already_max")},
                })},
            })},
        });
        const TStringBuf result1 = "Соседи говорят что и так всё хорошо слышат.";
        TestPhrase(*NlgRenderer, "sound_common", "render_error__sounderror", result1, context1);

        const auto context2 = NJson::TJsonMap({
            {"error", NJson::TJsonMap({
                {"data", NJson::TJsonMap({
                    {"code", NJson::TJsonValue("already_min")},
                })},
            })},
        });
        const TStringBuf result2 = "Тише уже нельзя.";
        TestPhrase(*NlgRenderer, "sound_common", "render_error__sounderror", result2, context2);

        const auto context3 = NJson::TJsonMap({
            {"error", NJson::TJsonMap({
                {"data", NJson::TJsonMap({
                    {"code", NJson::TJsonValue("already_set")},
                })},
            })},
        });
        const TStringBuf result3 = "Ничего не изменилось.";
        TestPhrase(*NlgRenderer, "sound_common", "render_error__sounderror", result3, context3);

        const auto context4 = NJson::TJsonMap({
            {"error", NJson::TJsonMap({
                {"data", NJson::TJsonMap({
                    {"code", NJson::TJsonValue("level_out_of_range")},
                })},
            })},
            {"sound_max_level", NJson::TJsonValue(10)},
        });
        const TStringBuf result4 = "Шкала громкости - от 1 до 10. Но вы можете управлять ею в процентах, если вам так удобнее.";
        TestPhrase(*NlgRenderer, "sound_common", "render_error__sounderror", result4, context4);

        const TStringBuf result5 = "Произошла какая-то ошибка. Спросите ещё раз попозже, пожалуйста.";
        TestPhrase(*NlgRenderer, "sound_common", "render_error__sounderror", result5);
    }
}
