#include <alice/hollywood/library/scenarios/news/nlg/register.h>

#include <alice/library/util/rng.h>
#include <alice/nlg/library/testing/testing_helpers.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice;
using namespace NAlice::NNlg::NTesting;

namespace {

const auto REG = &NAlice::NHollywood::NLibrary::NScenarios::NNews::NNlg::RegisterAll;

constexpr TStringBuf NLG_NAME = "get_news";
constexpr TStringBuf NONEWS = "К сожалению, я не смогла найти новостей по данному запросу.";
// NLG always expect context.attentions in request.
const auto BASS_CONTEXT = NJson::TJsonMap({{"attentions", NJson::TJsonMap()}});
const auto REQ_INFO = NJson::TJsonMap({{"experiments", NJson::TJsonMap()}});

} // namespace

Y_UNIT_TEST_SUITE(GetNews) {
    Y_UNIT_TEST(NoNews) {
        TestPhrase(REG, NLG_NAME, "render_error__nonews", NONEWS, BASS_CONTEXT, NJson::TJsonMap(), REQ_INFO);
    }

    Y_UNIT_TEST(NewsEnded) {
        const auto context = NJson::TJsonMap({
            {"attentions", NJson::TJsonMap({
                {"news_ended", NJson::TJsonValue(true)}
            })}
        });

        const TString reply1 = "Вот и все новости из этого источника.";
        const TString reply2 = "Это все новости из этого источника.";

        CheckChoiceFreqs(REG, NLG_NAME, "render_error__nonews",
                         {{reply1, 0.5}, {reply2, 0.5}}, context, NJson::TJsonMap(), REQ_INFO);
    }

    Y_UNIT_TEST(ErrorSystem) {
        TestPhrase(REG, NLG_NAME, "render_error__system", NONEWS, BASS_CONTEXT, NJson::TJsonMap(), REQ_INFO);
    }
}
