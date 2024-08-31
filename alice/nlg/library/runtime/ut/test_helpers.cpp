#include <alice/nlg/library/runtime/helpers.h>
#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;
using namespace NAlice::NNlg::NPrivate;

namespace {

using TRenderNlgToOutputStream = std::function<void(IOutputStream&)>;

THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder> ConvertPlaceholdersMap(IOutputStream& out, const THashMap<TStringBuf, TRenderNlgToOutputStream>& placeholders) {
    auto result = THashMap<TStringBuf, TRenderLocalizedNlgPlaceholder>(placeholders.size());
    for (const auto& [key, value] : placeholders) {
        const auto& placeholderCallback = value;
        result.emplace(key, [&out, &placeholderCallback]() { placeholderCallback(out); });
    }
    return result;
}

TString RunFormatLocalizedTemplate(
    const TStringBuf localizedTemplate,
    const THashMap<TStringBuf, TRenderNlgToOutputStream>& placeholders)
{
    TStringStream out;
    FormatLocalizedTemplate(localizedTemplate, ConvertPlaceholdersMap(out, placeholders), out);
    return out.Str();
}

} // namespace


Y_UNIT_TEST_SUITE(NlgHelpers) {
    Y_UNIT_TEST(TestFormatLocalizedTemplate) {
        const auto placeholders = THashMap<TStringBuf, TRenderNlgToOutputStream> {
            {"x_placeholder", [](IOutputStream& out) { out << "placeholder x value";}},
            {"y_placeholder()", [](IOutputStream& out) { out << "placeholder y() value";}},
        };

        UNIT_ASSERT_EQUAL("template without placeholders",
            RunFormatLocalizedTemplate("template without placeholders", {}));

        UNIT_ASSERT_EQUAL("template with placeholder x value two placeholder y() value placeholders",
            RunFormatLocalizedTemplate("template with {x_placeholder} two {y_placeholder()} placeholders", placeholders));

        UNIT_ASSERT_EQUAL("template with placeholder y() value two placeholder x value placeholders",
            RunFormatLocalizedTemplate("template with {y_placeholder()} two {x_placeholder} placeholders", placeholders));

        UNIT_ASSERT_EXCEPTION(RunFormatLocalizedTemplate(
            "template {x_placeholder} with {y_placeholder()} unknown {z_placeholder} placeholder", placeholders), TTranslationError);
    }
}
