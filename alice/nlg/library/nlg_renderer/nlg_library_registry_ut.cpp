#include "create_nlg_renderer_from_nlg_library_path.h"
#include "nlg_renderer.h"

#include <alice/library/util/rng.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;

namespace {

const TString NLG_LIBRARY_PATH = "alice/nlg/library/nlg_renderer/ut/nlg";
const TString TEMPLATE_NAME = "test_intent";
const TString PHRASE_NAME = "happy_phrase";

} // namespace


class TNlgLibraryRegistryFixture : public NUnitTest::TBaseFixture {
public:
    TNlgLibraryRegistryFixture()
        : Rng_(4)
    {
        RenderContextData_.Context["x"] = "foo";
    }

    NAlice::IRng& Rng() {
        return Rng_;
    }

    const TRenderContextData& RenderContextData() {
        return RenderContextData_;
    }
private:
    NAlice::TRng Rng_;
    TRenderContextData RenderContextData_;
};

Y_UNIT_TEST_SUITE_F(NlgLibraryRegistryTest, TNlgLibraryRegistryFixture) {
    Y_UNIT_TEST(InvalidLibraryPath) {
        UNIT_ASSERT_EXCEPTION(
            CreateNlgRendererFromNlgLibraryPath("alice/non_existent/nlg", Rng()),
            yexception);
    }

    Y_UNIT_TEST(ProperLibraryPath) {
        const auto nlgRenderer = CreateNlgRendererFromNlgLibraryPath(NLG_LIBRARY_PATH, Rng());
        const auto phrase = nlgRenderer->RenderPhrase(
            TEMPLATE_NAME, PHRASE_NAME, ELanguage::LANG_RUS, Rng(), RenderContextData());

        UNIT_ASSERT_VALUES_EQUAL(phrase.Text, "Happy foo text");
        UNIT_ASSERT_VALUES_EQUAL(phrase.Voice, "Happy foo voice");
    }

}
