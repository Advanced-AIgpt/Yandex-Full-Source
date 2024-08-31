#include <alice/nlg/library/runtime/builtins.h>
#include <library/cpp/testing/unittest/registar.h>

namespace {

NAlice::NNlg::TEnvironment ENV;
NAlice::TFakeRng RNG;

TString EmojizeStr(const TString& str) {
    using namespace NAlice::NNlg;

    TCallStack callStack;
    TCallCtx ctx{ENV, RNG, callStack, TStringBuf()};

    auto value = TValue::String(str);
    auto result = NBuiltins::Emojize(ctx, /* globals = */ nullptr, value);
    return result.GetString().GetStr();
}

const TString CROSS = "❌";

} // namespace

Y_UNIT_TEST_SUITE(TestEmoji) {
    Y_UNIT_TEST(Emtpy) {
        UNIT_ASSERT_VALUES_EQUAL("", EmojizeStr(""));
    }

    Y_UNIT_TEST(Single) {
        UNIT_ASSERT_VALUES_EQUAL(CROSS, EmojizeStr(":x:"));
    }

    Y_UNIT_TEST(Multiple) {
        UNIT_ASSERT_VALUES_EQUAL("No emoji", EmojizeStr("No emoji"));
        UNIT_ASSERT_VALUES_EQUAL(":notarealemoji:", EmojizeStr(":notarealemoji:"));
        UNIT_ASSERT_VALUES_EQUAL(":::::::", EmojizeStr(":::::::"));
        UNIT_ASSERT_VALUES_EQUAL("::::::::", EmojizeStr("::::::::"));
        UNIT_ASSERT_VALUES_EQUAL(CROSS + CROSS, EmojizeStr(":x::x:"));
        UNIT_ASSERT_VALUES_EQUAL(CROSS + "foo" + CROSS, EmojizeStr(":x:foo:x:"));
        UNIT_ASSERT_VALUES_EQUAL(CROSS + ":" + CROSS, EmojizeStr(":x:::x:"));
        UNIT_ASSERT_VALUES_EQUAL(CROSS + "x:", EmojizeStr(":x:x:"));
        UNIT_ASSERT_VALUES_EQUAL("Alice: hello", EmojizeStr("Alice: hello"));
        UNIT_ASSERT_VALUES_EQUAL("Alice: hello " + CROSS + "!", EmojizeStr("Alice: hello :x:!"));
        UNIT_ASSERT_VALUES_EQUAL("Alice: hello " + CROSS + "!:", EmojizeStr("Alice: hello :x:!:"));
        UNIT_ASSERT_VALUES_EQUAL(":Alice: hello " + CROSS + "!", EmojizeStr(":Alice: hello :x:!"));
        UNIT_ASSERT_VALUES_EQUAL("Hello :Alice: hello " + CROSS + "!", EmojizeStr("Hello :Alice: hello :x:!"));
    }
}
