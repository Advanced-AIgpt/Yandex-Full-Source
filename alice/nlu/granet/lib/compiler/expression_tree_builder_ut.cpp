#include "expression_tree_builder.h"
#include "preprocessor.h"
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NGranet::NCompiler {

Y_UNIT_TEST_SUITE(TExpressionTreeBuilder) {

    struct TTestData {
        bool Optimize = true;
        TString Source;
        TString Expected;
    };

    void Test(const TTestData& data) {
        const TString source = NAlice::NUtUtils::NormalizeText(data.Source);
        TSrcLine::TRef lines = TLinesTreeBuilder(TSourceText::Create(source, "", false, LANG_ENG)).Build();
        TStringPool stringPool;
        TSourcesLoader dummyLoader;
        TExpressionTreeBuilder builder(TTextView(), LANG_RUS, dummyLoader, &stringPool);
        if (!data.Optimize) {
            builder.DisableOptimization();
        }
        builder.ReadLines(lines->Children);
        builder.Finalize();
        TStringStream actual;
        builder.GetTree()->DumpInline(&actual);
        NAlice::NUtUtils::TestEqual(source, data.Expected, actual.Str());
    }

    void Test(const TVector<TTestData>& dataList) {
        for (const TTestData& data : dataList) {
            Test(data);
        }
    }

    Y_UNIT_TEST(Simple) {
        Test({
            {true, "aaa", "{(aaa)}"},
            {true, "aaa\nbbb", "{(aaa)|(bbb)}"}
        });
    }

    Y_UNIT_TEST(OrInsideOr) {
        Test({
            .Optimize = true,
            .Source = "aaa | bbb ccc | (ddd | eee) | (fff)",
            .Expected = "{"
                "(aaa)|"
                "(bbb ccc)|"
                "(ddd)<w:0.5>|"
                "(eee)<w:0.5>|"
                "(fff)"
            "}"
        });
    }

    Y_UNIT_TEST(Question) {
        Test({
            .Optimize = true,
            .Source = R"(
                aa
                bb cc? (dd ee)? ff
                gg ee
            )",
            .Expected = "{"
                "(aa)|"
                "(bb ff)<w:0.25>|"
                "(bb dd ee ff)<w:0.25>|"
                "(bb cc ff)<w:0.25>|"
                "(bb cc dd ee ff)<w:0.25>|"
                "(gg ee)"
            "}"
        });
    }

    Y_UNIT_TEST(Bag) {
        Test({
            .Optimize = true,
            .Source = "aa | BB cc | (dd | ee) | (ff) | [gg hh* ii? jj+] | kk [ll mm]",
            .Expected = "{"
                "(aa)|"
                "(bb cc)|"
                "(dd)<w:0.5>|"
                "(ee)<w:0.5>|"
                "(ff)|"
                "([(gg) (hh)* (ii)? (jj)+])|"
                "(kk ll mm)<w:0.5>|"
                "(kk mm ll)<w:0.5>"
            "}"
        });
    }

    Y_UNIT_TEST(MotherBag) {
        Test({
            .Optimize = true,
            .Source = "[мама мыла раму]",
            .Expected = "{([(мама) (мыла) (раму)])}"
        });
    }

    Y_UNIT_TEST(TopLevelBag) {
        Test({
            .Optimize = true,
            .Source = "[(aa bb) (cc | dd) (ee ff)* (gg hh)? (ii | jj kk)+]",
            .Expected = "{(["
                "(aa bb) "
                "({(cc)|(dd)}) "
                "(ee ff)* "
                "(gg hh)? "
                "({(ii)|(jj kk)})+"
            "])}"
        });
    }

    Y_UNIT_TEST(Lemma) {
        Test({
            .Optimize = true,
            .Source = R"(
                Тексты
                %lemma
                действие (распространяется | до | конца | списка) правил
                или до директивы exact .
                сорока-воровка
                %exact
                сорока-воровка
                всё
            )",
            .Expected = "{"
                "(тексты)|"
                "(~действие ~распространяться ~правило)<w:0.25>|"
                "(~действие ~до ~правило)<w:0.25>|"
                "(~действие ~конец ~правило)<w:0.25>|"
                "(~действие ~список ~правило)<w:0.25>|"
                "(~или ~до ~директива ~exact .)|"
                "(~сорок ~воровка)<w:0.5>|"
                "(~сорока ~воровка)<w:0.5>|"
                "(сорока воровка)|"
                "(все)"
            "}"
        });
    }

    Y_UNIT_TEST(NegativeSimple) {
        Test({
            .Optimize = true,
            .Source = R"(
                %negative
                aa
            )",
            .Expected = "{(aa)<negative>}"
        });
    }

    Y_UNIT_TEST(NegativeCompound) {
        Test({
            .Optimize = true,
            .Source = R"(
                aa
                %negative
                bb (cc | dd)
                ee
            )",
            .Expected = "{"
                "(aa)|"
                "(bb cc)<negative><w:0.5>|"
                "(bb dd)<negative><w:0.5>|"
                "(ee)<negative>"
            "}"
        });
    }

    Y_UNIT_TEST(Weight) {
        Test({
            .Optimize = true,
            .Source = R"(
                aa
                %weight 1.5
                bb
                $B | cc
                (dd | ee)
                %weight 2
                ff
            )",
            .Expected = "{"
                "(aa)|"
                "(bb)<w:1.5>|"
                "($B)<w:1.5>|"
                "(cc)<w:1.5>|"
                "(dd)<w:0.75>|"
                "(ee)<w:0.75>|"
                "(ff)<w:2>"
            "}"
        });
    }

    Y_UNIT_TEST(SimpleRange) {
        Test({
            .Optimize = true,
            .Source = "$B<1,3>",
            .Expected = "{({($B)}<1,3>)}"
        });
    }

    Y_UNIT_TEST(OperatorRange) {
        Test({
            .Optimize = true,
            .Source = "$B<,3> $C<2,> привет* пока+",
            .Expected = "{("
                "{($B)}<0,3> "
                "{($C)}<2,255> "
                "{(привет)}* "
                "{(пока)}+"
            ")}"
        });
    }

    Y_UNIT_TEST(SimpleSlot) {
        Test({
            .Optimize = true,
            .Source = "a 'b'(slot)",
            .Expected = "{(a TAG_BEGIN b TAG_END<tag:slot>)}"
        });
    }

    Y_UNIT_TEST(ComplexSlot) {
        Test({
            .Optimize = true,
            .Source = R"(
                prefix 'slot body'(slot name/type:value) suffix
                prefix slot
                before (variant one | variant 'with slot'(slot_inside))? suffix
            )",
            .Expected = "{"
                "(prefix TAG_BEGIN slot body TAG_END<tag:slot name/type:value> suffix)|"
                "(prefix slot)|"
                "(before suffix)<w:0.5>|"
                "(before variant one suffix)<w:0.25>|"
                "(before variant TAG_BEGIN with slot TAG_END<tag:slot_inside> suffix)<w:0.25>"
            "}"
        });
    }

    Y_UNIT_TEST(Optimization) {
        Test({
            // Basic
            {true,  "a",                    "{(a)}"},
            {true,  "a b c",                "{(a b c)}"},
            {true,  "a | b c",              "{(a)|(b c)}"},

            // Chain inside chain
            {true,  "a (b (c d) e)",        "{(a b c d e)}"},

            // List inside list
            {false, "(a | b) | c",          "{({(a)|(b)})|(c)}"},
            {true,  "(a | b) | c",          "{(a)<w:0.5>|(b)<w:0.5>|(c)}"},
            {false, "(a | b)\nc",           "{({(a)|(b)})|(c)}"},
            {true,  "(a | b)\nc",           "{(a)<w:0.5>|(b)<w:0.5>|(c)}"},
            {false, "(a | b)\n%negative\n(c | d)", "{({(a)|(b)})|({(c)|(d)})<negative>}"},
            {true,  "(a | b)\n%negative\n(c | d)", "{(a)<w:0.5>|(b)<w:0.5>|(c)<negative><w:0.5>|(d)<negative><w:0.5>}"},

            // List inside chain
            {false, "(a | b c) d | e",      "{({(a)|(b c)} d)|(e)}"},
            {true,  "(a | b c) d | e",      "{(a d)<w:0.5>|(b c d)<w:0.5>|(e)}"},
            {true,  "a (b | c) d (e | f)",  "{(a b d e)|(a b d f)|(a c d e)|(a c d f)}"},
            {true,  "(a | b) (c | d | e)",  "{(a c)|(a d)|(a e)|(b c)|(b d)|(b e)}"},

            // Bag
            {true,  "[a]",                  "{(a)}"},
            {false, "[a b]",                "{([(a) (b)])}"},
            {true,  "[a b]",                "{(a b)|(b a)}"},
            {false, "[a b] c",              "{([(a) (b)] c)}"},
            {true,  "[a b] c",              "{(a b c)|(b a c)}"},
            {true,  "[a (b c)]",            "{(a b c)|(b c a)}"},
            {true,  "[a (b | c)]",          "{(a b)<w:0.5>|(a c)<w:0.5>|(b a)<w:0.5>|(c a)<w:0.5>}"},
            {true,  "[a b] | c",            "{(a b)<w:0.5>|(b a)<w:0.5>|(c)}"},
            {true,  "[a b] c | d e",        "{(a b c)<w:0.5>|(b a c)<w:0.5>|(d e)}"},

            // Not optimized bag
            {true,  "[a b c]",              "{([(a) (b) (c)])}"},
            {true,  "[a b c d]",            "{([(a) (b) (c) (d)])}"},
            {true,  "[a b?]",               "{([(a) (b)?])}"},

            // Question
            {false, "a?",                   "{({(a)}?)}"},
            {true,  "a?",                   "{()|(a)}"},
            {true,  "a? b",                 "{(b)|(a b)}"},
            {true,  "a (b c)? d",           "{(a d)|(a b c d)}"},
            {true,  "a b? c?",              "{(a)|(a c)|(a b)|(a b c)}"},
            {true,  "(a | b)? c",           "{(c)|(a c)<w:0.5>|(b c)<w:0.5>}"}
        });
    }
}

} // namespace NGranet::NCompiler
