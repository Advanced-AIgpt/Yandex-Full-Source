#include <alice/nlg/example/localized_example/register.h>
#include <alice/nlg/library/nlg_renderer/create_nlg_renderer_from_register_function.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/library/runtime_api/translations.h>
#include <alice/library/util/rng.h>
#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NNlg;

namespace {

class TLocalizedExampleFixture : public NUnitTest::TBaseFixture {
public:
    TRenderPhraseResult RenderPhrase(const TStringBuf phraseId, const ELanguage language) const {
        auto nlgRenderer = CreateNlgRenderer();
        auto rng = TRng(RenderNlgRngSeed);

        auto renderContextData = TRenderContextData();
        renderContextData.Context = Context;

        return nlgRenderer->RenderPhrase(
            "simple_phrase", phraseId, language, rng, std::move(renderContextData));
    }

private:
    INlgRendererPtr CreateNlgRenderer() const {
        auto rng = TRng(GlobalNlgRngSeed);
        auto translationsContainer = CreateTranslationsContainerFromFile(
            TFsPath(ArcadiaSourceRoot()) / "alice/nlg/example/localized_example/translations.json"
        );
        return CreateLocalizedNlgRendererFromRegisterFunction(
            ::NAlice::NNlg::NExample::NLocalizedExample::RegisterAll,
            std::move(translationsContainer),
            rng);
    }
public:
    ui64 GlobalNlgRngSeed = 0;
    ui64 RenderNlgRngSeed = 1;
    NJson::TJsonValue Context = NJson::TJsonMap();
};

} // namespace

Y_UNIT_TEST_SUITE_F(LocalizedExample, TLocalizedExampleFixture) {
    Y_UNIT_TEST(TestSimplePhrase) {
        UNIT_ASSERT_EQUAL("Привет!", RenderPhrase("say_hello", ELanguage::LANG_RUS).Text);
        // UNIT_ASSERT_EQUAL("Hello!", RenderPhrase("say_hello", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithPlaceholder) {
        Context = NJson::TJsonMap({{"name", "Алиса"}});
        UNIT_ASSERT_EQUAL("Привет, меня зовут Алиса, приятно познакомиться!",
            RenderPhrase("say_hello_with_name", ELanguage::LANG_RUS).Text);

        // Context = NJson::TJsonMap({{"name", "Alice"}});
        // UNIT_ASSERT_EQUAL("Hello, my name is Alice, nice to meet you!",
        //     RenderPhrase("say_hello_with_name", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithCall) {
        Context = NJson::TJsonMap({{"x", 5}, {"y", 3}});
        UNIT_ASSERT_EQUAL("Привет! Я умею вычитать числа. 5 минус 3 равняется 2. Я очень умная!",
            RenderPhrase("say_hello_with_call", ELanguage::LANG_RUS).Text);
        // UNIT_ASSERT_EQUAL("Hello! I can subtract numbers. 5 minus 3 equals 2. I'm very clever!",
        //     RenderPhrase("say_hello_with_call", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithVoiceText) {
        const auto ruResult = RenderPhrase("say_hello_with_voice_text", ELanguage::LANG_RUS);
        UNIT_ASSERT_EQUAL("Привет. Это голосовой ответ.", ruResult.Voice);
        UNIT_ASSERT_EQUAL("Привет. Это текстовый ответ.", ruResult.Text);

        // const auto enResult = RenderPhrase("say_hello_with_voice_text", ELanguage::LANG_ENG);
        // UNIT_ASSERT_EQUAL("Hello. It's voice response.", ruResult.Voice);
        // UNIT_ASSERT_EQUAL("Hello. It's text response.", ruResult.Text);
    }

    Y_UNIT_TEST(TestPhraseWithChooseline) {
        RenderNlgRngSeed = 0;
        UNIT_ASSERT_EQUAL("Доброе утро, загадочный незнакомец.",
            RenderPhrase("say_hello_with_chooseline", ELanguage::LANG_RUS).Text);
        RenderNlgRngSeed = 3;
        UNIT_ASSERT_EQUAL("Добрый день, загадочный незнакомец.",
            RenderPhrase("say_hello_with_chooseline", ELanguage::LANG_RUS).Text);

        // RenderNlgRngSeed = 0;
        // UNIT_ASSERT_EQUAL("good morning, mysterious stranger.",
        //     RenderPhrase("say_hello_with_chooseline", ELanguage::LANG_ENG).Text);
        // RenderNlgRngSeed = 3;
        // UNIT_ASSERT_EQUAL("Good afternoon, mysterious stranger.",
        //     RenderPhrase("say_hello_with_chooseline", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithIf) {
        Context = NJson::TJsonMap({{"is_mysterious", false}});
        UNIT_ASSERT_EQUAL("Привет, незнакомец.", RenderPhrase("say_hello_with_if", ELanguage::LANG_RUS).Text);
        Context = NJson::TJsonMap({{"is_mysterious", true}});
        UNIT_ASSERT_EQUAL("Привет, загадочный незнакомец.", RenderPhrase("say_hello_with_if", ELanguage::LANG_RUS).Text);

        // Context = NJson::TJsonMap({{"is_mysterious", false}});
        // UNIT_ASSERT_EQUAL("Hello, stranger.", RenderPhrase("say_hello_with_if", ELanguage::LANG_ENG).Text);
        // Context = NJson::TJsonMap({{"is_mysterious", true}});
        // UNIT_ASSERT_EQUAL("Hello, mysterious stranger.", RenderPhrase("say_hello_with_if", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithCondexpr) {
        Context = NJson::TJsonMap({{"is_mysterious", false}});
        UNIT_ASSERT_EQUAL("Привет, обычный незнакомец.", RenderPhrase("say_hello_with_condexpr", ELanguage::LANG_RUS).Text);
        Context = NJson::TJsonMap({{"is_mysterious", true}});
        UNIT_ASSERT_EQUAL("Привет, загадочный незнакомец.", RenderPhrase("say_hello_with_condexpr", ELanguage::LANG_RUS).Text);

        // Context = NJson::TJsonMap({{"is_mysterious", false}});
        // UNIT_ASSERT_EQUAL("Hello, stranger.", RenderPhrase("say_hello_with_condexpr", ELanguage::LANG_ENG).Text);
        // Context = NJson::TJsonMap({{"is_mysterious", true}});
        // UNIT_ASSERT_EQUAL("Hello, mysterious stranger.", RenderPhrase("say_hello_with_condexpr", ELanguage::LANG_ENG).Text);
    }

    Y_UNIT_TEST(TestPhraseWithJinjaWhitespaceStrip) {
        UNIT_ASSERT_EQUAL("Здесь имитируется DivCard разметка: \"text\": \"<font color=\\\"#0A4B8C\\\">цветной текст<\\/font>\". Нам важно не добавить пробелы рядом с вызовом макроса.",
            RenderPhrase("phrase_with_jinja_whitespace_strip", ELanguage::LANG_RUS).Text);
        // UNIT_ASSERT_EQUAL("DivCard markup is simulated here: \"text\": \"<font color=\\\"#0A4B8C\\\">цветной текст<\\/font>\". It's important not to add spaces around macro call.",
        //     RenderPhrase("phrase_with_jinja_whitespace_strip", ELanguage::LANG_ENG).Text);
    }
}
