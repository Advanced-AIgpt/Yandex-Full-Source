#include <alice/nlg/library/runtime_api/env.h>

#include <library/cpp/testing/unittest/registar.h>

using namespace NAlice::NNlg;

namespace {

void Phrase(const TCallCtx&, const TCaller*, const TGlobalsChain*, IOutputStream&) {
    // nothing
}

void Card(const TCallCtx&, const TCaller*, const TGlobalsChain*, IOutputStream&) {
    // nothing
}

TValue Value(const TStringBuf json) {
    return TValue::ParseJson(json);
}

} // namespace

Y_UNIT_TEST_SUITE(NlgEnv) {
    Y_UNIT_TEST(Templates) {
        TEnvironment env;
        env.RegisterPhrase("template1", "phrase", &Phrase);
        env.RegisterCard("template2", "card", &Card);

        UNIT_ASSERT(env.HasPhrase("template1", "phrase", ELanguage::LANG_RUS));
        UNIT_ASSERT(!env.HasPhrase("template1", "card", ELanguage::LANG_RUS));
        UNIT_ASSERT(!env.HasPhrase("template2", "card", ELanguage::LANG_RUS));

        UNIT_ASSERT(env.HasCard("template2", "card", ELanguage::LANG_RUS));
        UNIT_ASSERT(!env.HasPhrase("template2", "card", ELanguage::LANG_RUS));
        UNIT_ASSERT(!env.HasCard("template1", "card", ELanguage::LANG_RUS));
    }

    Y_UNIT_TEST(TransformForm) {
        // positive cases
        {
            auto target = TValue::Dict();
            auto expected = TValue::Dict();
            GetAttrStore(expected, "raw_form") = TValue::Dict(target.GetDict());
            GetAttrStore(GetAttrStore(expected, "raw_form"), "slots_by_name") = TValue::Dict();
            UNIT_ASSERT_VALUES_EQUAL(expected, TransformForm(std::move(target)));
        }
        {
            auto target = Value(R"({"slots":[{"slot":"foo","value":123},{"name":"bar","value":"str"}]})");
            auto expected = Value(R"({"foo":123,"bar":"str"})");
            GetAttrStore(expected, "raw_form") = TValue::Dict(target.GetDict());
            GetAttrStore(GetAttrStore(expected, "raw_form"), "slots_by_name") =
                Value(R"({"foo":{"slot":"foo","value":123},"bar":{"name":"bar","value":"str"}})");
            UNIT_ASSERT_VALUES_EQUAL(expected, TransformForm(std::move(target)));
        }

        // negative cases
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"(123)")), TTypeError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":123})")), TTypeError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":[123]})")), TTypeError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":[{"slot1":"foo","value":1}]})")), TValueError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":[{"slot":"raw_form","value":1}]})")), TValueError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":[{"slot":123,"value":1}]})")), TTypeError);
        UNIT_ASSERT_EXCEPTION(TransformForm(Value(R"({"slots":[{"slot":"foo"}]})")), TValueError);
    }
}
