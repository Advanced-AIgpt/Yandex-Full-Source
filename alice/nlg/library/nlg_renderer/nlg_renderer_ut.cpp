#include "create_nlg_renderer_from_register_function.h"
#include "nlg_renderer.h"

#include <alice/nlg/library/nlg_renderer/ut/nlg/register.h>

#include <alice/library/util/rng.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;

namespace {

const TString TEMPLATE_NAME = "test_intent";
const TString ABSENT_TEMPLATE_NAME = "absent_template";

TRenderContextData CreateContext(const TStringBuf x) {
    return TRenderContextData {
        .Context = NJson::TJsonMap({
            {"x", NJson::TJsonValue(x)},
        }),
    };
}

} // namespace

class TNlgRendererFixture : public NUnitTest::TBaseFixture {
public:
    TNlgRendererFixture()
        : Rng_(4)
        , NlgRenderer_(CreateNlgRendererFromRegisterFunction(::NAlice::NNlg::NLibrary::NNlgRenderer::NUt::NNlg::RegisterAll, Rng_))
    {
    }

    NAlice::IRng& Rng() {
        return Rng_;
    }

    const INlgRenderer& NlgRenderer() const {
        return *NlgRenderer_;
    }
private:
    NAlice::TRng Rng_;
    INlgRendererPtr NlgRenderer_;
};

Y_UNIT_TEST_SUITE_F(NlgRendererTest, TNlgRendererFixture) {
    Y_UNIT_TEST(HasPhrase) {
        UNIT_ASSERT(NlgRenderer().HasPhrase(TEMPLATE_NAME, "happy_phrase", ELanguage::LANG_RUS));
        UNIT_ASSERT(!NlgRenderer().HasPhrase(TEMPLATE_NAME, "absent_phrase", ELanguage::LANG_RUS));
        UNIT_ASSERT(!NlgRenderer().HasPhrase(TEMPLATE_NAME, "happy_phrase", ELanguage::LANG_FRE));
        UNIT_ASSERT(!NlgRenderer().HasPhrase(ABSENT_TEMPLATE_NAME, "happy_phrase", ELanguage::LANG_RUS));
    }
    Y_UNIT_TEST(HasCard) {
        UNIT_ASSERT(NlgRenderer().HasCard(TEMPLATE_NAME, "happy_card", ELanguage::LANG_RUS));
        UNIT_ASSERT(!NlgRenderer().HasCard(TEMPLATE_NAME, "absent_card", ELanguage::LANG_RUS));
        UNIT_ASSERT(!NlgRenderer().HasCard(TEMPLATE_NAME, "happy_card", ELanguage::LANG_FRE));
        UNIT_ASSERT(!NlgRenderer().HasCard(ABSENT_TEMPLATE_NAME, "happy_card", ELanguage::LANG_RUS));
    }

    Y_UNIT_TEST(RenderPhrasePositive) {
        const auto phrase = NlgRenderer().RenderPhrase(
            TEMPLATE_NAME, "happy_phrase", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));

        UNIT_ASSERT_VALUES_EQUAL(phrase.Text, "Happy foo text");
        UNIT_ASSERT_VALUES_EQUAL(phrase.Voice, "Happy foo voice");
    }

    Y_UNIT_TEST(RenderPhraseNegative) {
        const auto render = [&]() {
            return NlgRenderer().RenderPhrase(
                TEMPLATE_NAME, "sad_phrase", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));
        };
        UNIT_ASSERT_EXCEPTION(render(), TRuntimeError);
    }

    Y_UNIT_TEST(RenderPhraseNotFound) {
        const auto render = [&]() {
            return NlgRenderer().RenderPhrase(
                TEMPLATE_NAME, "absent_phrase", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));
        };
        UNIT_ASSERT_EXCEPTION(render(), TPhraseNotFoundError);
    }

    Y_UNIT_TEST(RenderCardPositive) {
        const auto card = NlgRenderer().RenderCard(
            TEMPLATE_NAME, "happy_card", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));

        const auto expected = NJson::ReadJsonFastTree(R"({
            "background": [
                {"type": "Happy foo card"}
            ],
            "states": []
        })");

        UNIT_ASSERT_VALUES_EQUAL(card.Card, expected);
    }

    Y_UNIT_TEST(RenderCardDiv2) {
        const auto card = NlgRenderer().RenderCard(
            TEMPLATE_NAME, "div2_card", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));

        const auto expected = NJson::ReadJsonFastTree(R"({
            "templates": {
                "foo": {}
            },
            "card": {
                "log_id": "Div2 foo card"
            }
        })");

        UNIT_ASSERT_VALUES_EQUAL(card.Card, expected);
    }

    Y_UNIT_TEST(RenderCardNegative) {
        const auto render = [&]() {
            return NlgRenderer().RenderCard(
                TEMPLATE_NAME, "sad_card", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));
        };
        UNIT_ASSERT_EXCEPTION(render(), TRuntimeError);
    }

    Y_UNIT_TEST(RenderCardNotFound) {
        const auto render = [&]() {
            return NlgRenderer().RenderCard(
                TEMPLATE_NAME, "absent_card", ELanguage::LANG_RUS, Rng(), CreateContext("foo"));
        };
        UNIT_ASSERT_EXCEPTION(render(), TCardNotFoundError);
    }

}
