#include "preprocessor.h"
#include "compiler_error.h"
#include <alice/nlu/libs/ut_utils/ut_utils.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/string/join.h>

namespace NGranet::NCompiler {

Y_UNIT_TEST_SUITE(TPreprocessor) {

    void PrintTokenTree(const TString& indent, bool shouldUnescape, const TSrcLine* node, TStringBuilder* out) {
        Y_ENSURE(node);
        Y_ENSURE(out);
        *out << indent << (shouldUnescape ? Unquote(node->Str()) : TString{node->Str()}) << '\n';
        for (const TSrcLine::TRef& child : node->Children) {
            PrintTokenTree(indent + "    ", shouldUnescape, child.Get(), out);
        }
    }

    TString PrintTokenTree(const TSrcLine* root, bool shouldUnescape) {
        Y_ENSURE(root);
        TStringBuilder out;
        PrintTokenTree("--->", shouldUnescape, root, &out);
        return out;
    }

    void TestParsing(TStringBuf srcRaw, bool shouldUnescape, TStringBuf expected) {
        const TString src = NAlice::NUtUtils::NormalizeText(srcRaw);
        TSrcLine::TConstRef root = TLinesTreeBuilder(TSourceText::Create(src, "", false, LANG_ENG)).Build();
        const TString actual = PrintTokenTree(root.Get(), shouldUnescape);
        NAlice::NUtUtils::TestEqual(src, expected, actual);
    }

    TSrcLine::TRef Preprocess(TStringBuf srcRaw) {
        const TString srcText = NAlice::NUtUtils::NormalizeText(srcRaw);
        return TLinesTreeBuilder(TSourceText::Create(srcText, "", false, LANG_ENG)).Build();
    }

    void CheckException(TStringBuf src) {
        UNIT_CHECK_GENERATED_EXCEPTION(Preprocess(src), TCompilerError);
    }

    void CheckNoException(TStringBuf src) {
        UNIT_CHECK_GENERATED_NO_EXCEPTION(Preprocess(src), TCompilerError);
    }

    Y_UNIT_TEST(Parsing) {
        const TStringBuf source = R"(
            # comment
            value
            key1: value1
            # comment
            key2: value2 # comment
                key21: value21
                # comment
                value22
                value23; value24; value25

                key26:
                    value27
                    # comment
                    value28 | "special : \"symbols | more\"" | value 30
                    value31 | value32
            key 3: value31 | value32
                value33 | (value34 | value35) | value36 |
                value37

            "key:4":
                "{special: \"symbols\", )( #in: \"quotes\"}": value41 | value42
                "\"\\\n\"": value43 | value44 | value 45 # comment
            multiline: (
                    text 1 2 3 | word1
                        word2
                            word3 ( "string"
                        word4) [] word5
            word6)
                normal
            key5: $Object<g:pl,nom>
        )";


        TestParsing(source, false, R"(
            --->
            --->    value
            --->    key1
            --->        value1
            --->    key2
            --->        value2
            --->        key21
            --->            value21
            --->        value22
            --->        value23
            --->        value24
            --->        value25
            --->        key26
            --->            value27
            --->            value28
            --->            "special : \"symbols | more\""
            --->            value 30
            --->            value31
            --->            value32
            --->    key 3
            --->        value31
            --->        value32
            --->        value33
            --->        (value34 | value35)
            --->        value36
            --->        value37
            --->    "key:4"
            --->        "{special: \"symbols\", )( #in: \"quotes\"}"
            --->            value41
            --->            value42
            --->        "\"\\\n\""
            --->            value43
            --->            value44
            --->            value 45
            --->    multiline
            --->        (
                    text 1 2 3 | word1
                        word2
                            word3 ( "string"
                        word4) [] word5
            word6)
            --->        normal
            --->    key5
            --->        $Object<g:pl,nom>
        )");

        // Unqouted
        TestParsing(source, true, R"(
            --->
            --->    value
            --->    key1
            --->        value1
            --->    key2
            --->        value2
            --->        key21
            --->            value21
            --->        value22
            --->        value23
            --->        value24
            --->        value25
            --->        key26
            --->            value27
            --->            value28
            --->            special : "symbols | more"
            --->            value 30
            --->            value31
            --->            value32
            --->    key 3
            --->        value31
            --->        value32
            --->        value33
            --->        (value34 | value35)
            --->        value36
            --->        value37
            --->    key:4
            --->        {special: "symbols", )( #in: "quotes"}
            --->            value41
            --->            value42
            --->        "\
            "
            --->            value43
            --->            value44
            --->            value 45
            --->    multiline
            --->        (
                    text 1 2 3 | word1
                        word2
                            word3 ( "string"
                        word4) [] word5
            word6)
            --->        normal
            --->    key5
            --->        $Object<g:pl,nom>
        )");
    }

    Y_UNIT_TEST(CheckIndentWithoutColon) {
        CheckException(R"(
            key,
                value
        )");
    }

    Y_UNIT_TEST(CheckNoException) {
        CheckNoException(R"(
            value
        )");
    }

    Y_UNIT_TEST(CheckCompleteBraces) {
        CheckException(R"(
            (value # not completed brace )
        )");
    }

    Y_UNIT_TEST(CheckCompleteQuotes) {
        CheckException(R"(
            "value
        )");
    }

    Y_UNIT_TEST(CheckCommentInsideBracesNotSupported) {
        CheckException(R"(
            multiline: (
                word1 # comment
                word2
            )
        )");
    }
}

} // namespace NGranet::NCompiler
