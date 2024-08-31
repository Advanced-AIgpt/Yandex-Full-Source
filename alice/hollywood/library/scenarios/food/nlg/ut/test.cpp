#include <alice/library/util/rng.h>
#include <alice/hollywood/library/scenarios/food/nlg/register.h>
#include <alice/nlg/library/nlg_renderer/nlg_renderer.h>
#include <alice/nlg/library/testing/testing_helpers.h>
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;

namespace {

NNlg::TRenderPhraseResult GetNlgOutput(TStringBuf templateId, TStringBuf phrase, TStringBuf context) {
    const auto contextValue = NJson::ReadJsonFastTree(context);
    const auto nlgRenderer = NNlg::NTesting::CreateTestingNlgRenderer(&NAlice::NHollywood::NLibrary::NScenarios::NFood::NNlg::RegisterAll);
    return NNlg::NTesting::GetRenderPhraseResult(*nlgRenderer, templateId, phrase, nullptr, contextValue);
}

void TestNlg(TStringBuf templateId, TStringBuf phrase, TStringBuf context,
    TStringBuf expectedText, TStringBuf expectedVoice)
{
    const auto [actualText, actualVoice] = GetNlgOutput(templateId, phrase, context);
    TStringBuilder comment;
    comment << "phrase: " << phrase << Endl;
    NUtUtils::TestEqualStr(comment + "test text", expectedText, actualText);
    NUtUtils::TestEqualStr(comment + "test voice", expectedVoice, actualVoice);
}

void TestNlg(TStringBuf templateId, TStringBuf phrase, TStringBuf context, TStringBuf expected) {
    TestNlg(templateId, phrase, context, expected, expected);
}

} // namespace

Y_UNIT_TEST_SUITE(TestFoodRu) {

    Y_UNIT_TEST(HowToOrder) {
        TestNlg(
            "food",
            "fallback_common",
            "{}",
            "Извините, я вас не поняла."
        );
    }

    Y_UNIT_TEST(NewItems) {
        TestNlg(
            "food",
            "nlg_cart_show_ok",
            R"(
                {
                  "cart": {
                    "items": [
                      {
                        "quantity": 2,
                        "name": "Картофель Фри",
                        "item_option_names": [
                          "Стандартный",
                          "Без Соуса"
                        ],
                        "price": 128
                      },
                      {
                        "quantity": 1,
                        "name": "Биг Мак МакКомбо Большой",
                        "item_option_names": [
                          "Картофель фри",
                          "Фанта"
                        ],
                        "price": 225
                      }
                    ]
                  },
                  "subtotal": 353
                }
            )",
            R"(
                В корзине:
                * 2 x Картофель Фри (Стандартный, Без Соуса) – 128 р.
                * Биг Мак МакКомбо Большой (Картофель фри, Фанта) – 225 р.
                Сумма вашего заказа 353 р.
                Заказываем?
            )",
            R"(
                В корзине:
                Картофель Фри. две штуки. Опции: Стандартный, Без Соуса.
                Биг Мак МакКомбо Большой. Опции: Картофель фри, Фанта.
                Сумма вашего заказа 353 р.
                Заказываем?
            )"
        );
    }
}
