#include <alice/nlu/granet/lib/ut/granet_tester.h>
#include <alice/nlu/granet/lib/compiler/compiler_error.h>

using namespace NGranet;
using namespace NGranet::NCompiler;

Y_UNIT_TEST_SUITE(GranetPart14_CompilerError) {

    void CheckInvalid(TStringBuf src) {
        UNIT_CHECK_GENERATED_EXCEPTION(TGranetTester(src), NCompiler::TCompilerError);
    }

    void CheckInvalid(EMessageId messageId, TStringBuf src, const TGranetDomain& domain = {}) {
        try {
            TGranetTester tester(src, domain);
        } catch (const NCompiler::TCompilerError& e) {
            if (e.MessageId == messageId) {
                return;
            }
            TStringStream out;
            out << Endl;
            out << "Wrong exception:" << Endl;
            out << "  Expected MessageId: " << messageId << Endl;
            out << "  Actual MessageId:   " << e.MessageId << Endl;
            UNIT_FAIL(out.Str());
        }
        TStringStream out;
        out << Endl;
        out << "No exception:" << Endl;
        out << "  Expected MessageId: " << messageId << Endl;
        UNIT_FAIL(out.Str());
    }

    void CheckInvalid(EMessageId messageId, size_t lineIndex, size_t columnIndex, size_t charCount, TStringBuf src) {
        try {
            TGranetTester tester(src);
        } catch (const NCompiler::TCompilerError& e) {
            if (e.MessageId == messageId
                && e.LineIndex == lineIndex
                && e.ColumnIndex == columnIndex
                && e.CharCount == charCount)
            {
                return;
            }
            TStringStream out;
            out << Endl;
            out << "Wrong exception:" << Endl;
            out << "  Expected MessageId:   " << messageId << Endl;
            out << "  Expected LineIndex:   " << lineIndex << Endl;
            out << "  Expected ColumnIndex: " << columnIndex << Endl;
            out << "  Expected CharCount:   " << charCount << Endl;
            out << "  Actual MessageId:     " << e.MessageId << Endl;
            out << "  Actual LineIndex:     " << e.LineIndex << Endl;
            out << "  Actual ColumnIndex:   " << e.ColumnIndex << Endl;
            out << "  Actual CharCount:     " << e.CharCount << Endl;
            UNIT_FAIL(out.Str());
        }
        TStringStream out;
        out << Endl;
        out << "No exception:" << Endl;
        out << "  Expected MessageId: " << messageId << Endl;
        UNIT_FAIL(out.Str());
    }

    void CheckValid(TStringBuf src, const TGranetDomain& domain = {}) {
        UNIT_CHECK_GENERATED_NO_EXCEPTION(TGranetTester(src, domain), NCompiler::TCompilerError);
    }

    Y_UNIT_TEST(Test_MSG_UNDEFINED_PATH) {
        CheckInvalid(MSG_UNDEFINED_PATH, 0, 8, 2, R"(
            import: ""
        )");
    }

    Y_UNIT_TEST(Test_MSG_DUPLICATED_KEY) {
        CheckInvalid(MSG_DUPLICATED_KEY, R"(
            form f:
                keep_variants: true
                keep_variants: false
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_NAME_IS_EMPTY) {
        // Can't be
    }

    Y_UNIT_TEST(EmptyTaskName) {
        CheckInvalid(MSG_UNEXPECTED_KEYWORD, R"(
            form:
                root:
                    aaa
        )");
        CheckInvalid(MSG_UNEXPECTED_KEYWORD, R"(
            entity:
                root:
                    aaa
        )");

        CheckValid(R"(
            form alice.my_form:
                root:
                    aaa
        )");
        CheckValid(R"(
            entity custom.my_entity:
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_NAME) {
        CheckInvalid(MSG_INVALID_NAME, R"(
            form $alice.my_form:
                root:
                    aaa
        )");
        CheckInvalid(MSG_INVALID_NAME, R"(
            entity $custom.my_entity:
                root:
                    aaa
        )");
        CheckInvalid(MSG_INVALID_NAME, R"(
            form 4eee7a6c-9e0f-4456-8083-6c946f920d4b.restart:
                root:
                    aaa
        )");

        CheckValid(R"(
            form alice.my_form:
                root:
                    aaa
        )");
        CheckValid(R"(
            entity custom.my_entity:
                root:
                    aaa
        )");
        CheckValid(R"(
            form 4eee7a6c-9e0f-4456-8083-6c946f920d4b.restart:
                root:
                    aaa
        )", {.IsPASkills = true});
    }

    Y_UNIT_TEST(Test_MSG_DUPLICATED_NAME) {
        CheckInvalid(MSG_DUPLICATED_NAME, R"(
            form f:
                root:
                    aaa
            form f:
                root:
                    bbb
        )");
        CheckInvalid(MSG_DUPLICATED_NAME, R"(
            entity custom.my:
                root:
                    aaa
            entity custom.my:
                root:
                    bbb
        )");

        CheckValid(R"(
            form f1:
                root:
                    aaa
            form f2:
                root:
                    bbb
        )");
        CheckValid(R"(
            entity custom.my1:
                root:
                    aaa
            entity custom.my2:
                root:
                    bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_DUPLICATED_ELEMENT_NAME) {
        CheckInvalid(MSG_DUPLICATED_ELEMENT_NAME, R"(
            $A:
                aaa
            $A:
                bbb
        )");
        CheckInvalid(MSG_DUPLICATED_ELEMENT_NAME, R"(
            form f:
                root:
                    aaa
                root:
                    bbb
        )");
        // Different scopes
        CheckValid(R"(
            form f:
                root:
                    $A
                $A:
                    aaa
            root:
                $A
            $A:
                bbb
        )");
        // List of entity values treated as root
        CheckInvalid(MSG_DUPLICATED_ELEMENT_NAME, R"(
            entity custom.my:
                root:
                    aaa
                values:
                    v1:
                        aaa
                    v2:
                        bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNEXPECTED_KEYWORD) {
        CheckInvalid(MSG_UNEXPECTED_KEYWORD, R"(
            form f:
                slot:
                    my_slot:
                        type: string
                root:
                    aaa
        )");
        CheckValid(R"(
            form f:
                slots:
                    my_slot:
                        type: string
                root:
                    aaa
        )");
        CheckInvalid(MSG_UNEXPECTED_KEYWORD, R"(
            importS:  # should be import
                aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_EMPTY_SLOT_NAME) {
        // Can't be
    }

    Y_UNIT_TEST(Test_MSG_DUPLICATED_SLOT_NAME) {
        CheckInvalid(MSG_DUPLICATED_SLOT_NAME, R"(
            form f:
                slots:
                    my_slot:
                        type: string
                    my_slot:
                        type: string
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_TYPE_NAME) {
        CheckInvalid(MSG_INVALID_TYPE_NAME, R"(
            form f:
                slots:
                    my_slot:
                        type: $sys.num
                root:
                    aaa
        )");
        CheckValid(R"(
            form f:
                slots:
                    my_slot:
                        type: sys.num
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_MATCHING_TYPE) {
        CheckInvalid(MSG_INVALID_MATCHING_TYPE, R"(
            form f:
                slots:
                    my_slot:
                        type: string
                        matching_type: overlapED
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_CONCATENATE_STRINGS) {
        CheckInvalid(MSG_INVALID_CONCATENATE_STRINGS, R"(
            form f:
                enable_alice_tagger: true
                enable_granet_parser: false
                slots:
                    my_slot:
                        type: string
                        concatenate_strings: x
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_KEEP_VARIANTS) {
        CheckInvalid(MSG_INVALID_KEEP_VARIANTS, R"(
            form f:
                slots:
                    my_slot:
                        type: string
                        keep_variants: x
        )");
    }

    Y_UNIT_TEST(Test_MSG_NO_TYPE_SECTION) {
        CheckInvalid(MSG_NO_TYPE_SECTION, R"(
            form f:
                slots:
                    my_slot:
                        source: $sys.num
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_CONCATENATE_STRINGS_SUPPORTED_ONLY_FOR_TAGGER) {
        CheckInvalid(MSG_CONCATENATE_STRINGS_SUPPORTED_ONLY_FOR_TAGGER, R"(
            form f:
                enable_granet_parser: false
                slots:
                    my_slot:
                        type: string
                        concatenate_strings: true
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_ELEMENT_NAME) {
        CheckInvalid(MSG_INVALID_ELEMENT_NAME, R"(
            $A:
                aaa
                bbb
                Embedded:
                   ccc
        )");
        CheckValid(R"(
            $A:
                aaa
                bbb
                $Embedded:
                   ccc
        )");

        CheckInvalid(MSG_INVALID_ELEMENT_NAME, R"(
            $A:
                aaa
                root:
                    bbb
        )");
        CheckInvalid(MSG_INVALID_ELEMENT_NAME, R"(
            $A:
                aaa
                some text: with colon
        )");
        CheckValid(R"(
            $A:
                aaa
                $some_text: with colon
        )");

        CheckInvalid(MSG_INVALID_ELEMENT_NAME, R"(
            form f:
                slots:
                    my_slot:
                        type: string
                        source: sys.num
                root:
                    aaa
        )");
        CheckValid(R"(
            form f:
                slots:
                    my_slot:
                        type: string
                        source: $sys.num
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_CAN_NOT_FIND_ELEMENT) {
        CheckInvalid(MSG_CAN_NOT_FIND_ELEMENT, R"(
            form f:
                root:
                    $A
        )");

        CheckInvalid(MSG_CAN_NOT_FIND_ELEMENT, R"(
            form form_without_root:
                slots:
                    my_slot:
                        type: string
        )");
        CheckValid(R"(
            form form_without_root:
                slots:
                    my_slot:
                        type: string
                root: привет
        )");
        CheckValid(R"(
            form form_without_root:
                slots:
                    my_slot:
                        type: string
                enable_granet_parser: false
        )");
    }

    Y_UNIT_TEST(Test_MSG_AMBIGUOUS_ELEMENT_NAME) {
        // Reproduced only with imported files
    }

    Y_UNIT_TEST(Test_MSG_UNKNOWN_TYPE_OF_MODIFICATION) {
        CheckInvalid(MSG_UNKNOWN_TYPE_OF_MODIFICATION, R"(
            form f:
                root:
                    $B<unknown_modification>
            $B:
                aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_GRAMMEME_PARSER_ERROR) {
        CheckInvalid(MSG_GRAMMEME_PARSER_ERROR, R"(
            form f:
                root:
                    $B<g:unknown_grammeme>
            $B:
                слово
        )");
    }

    Y_UNIT_TEST(Test_MSG_STRING_LITERAL_PARSER_ERROR) {
        // Can't be
    }

    Y_UNIT_TEST(Test_MSG_NOT_QUOTED_STRING_PARSING_ERROR) {
        CheckInvalid(MSG_NOT_QUOTED_STRING_PARSING_ERROR, R"(
            entity e:
                values:
                    {"base": 2}: двоичный
                    {"base": 10}: десятичный
        )");
        CheckValid(R"(
            entity e:
                values:
                    "{\"base\": 2}": двоичный
                    "{\"base\": 10}": десятичный
        )");
        CheckValid(R"(
            entity e:
                values:
                    2: двоичный
                    10: десятичный
        )");
    }

    Y_UNIT_TEST(Test_MSG_RECURSION_DETECTED) {
        CheckInvalid(MSG_RECURSION_DETECTED, R"(
            form f:
                root:
                    $A
            $A:
                $B
            $B:
                $A
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNKNOWN_DIRECTIVE) {
        CheckInvalid(MSG_UNKNOWN_DIRECTIVE, R"(
            form f:
                root:
                    %unknown_directive
        )");
    }

    Y_UNIT_TEST(Test_MSG_ALLOWED_ONLY_BEFORE_ALL_RULES) {
        CheckInvalid(MSG_ALLOWED_ONLY_BEFORE_ALL_RULES, R"(
            form f:
                root:
                    $A
            $A:
                aaa
                %anchor_to_begin
                bbb
        )");
        CheckValid(R"(
            form f:
                root:
                    $A
            $A:
                %anchor_to_begin
                aaa
                bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_NOT_ALLOWED_IN_FILLER) {
        CheckInvalid(MSG_NOT_ALLOWED_IN_FILLER, R"(
            form f:
                root:
                    a
            filler:
                %cover_fillers
        )");
    }

    Y_UNIT_TEST(Test_MSG_NOT_ALLOWED_IN_ROOT_OF_FORM) {
        CheckInvalid(MSG_NOT_ALLOWED_IN_ROOT_OF_FORM, R"(
            form f:
                root:
                    %anchor_to_begin
                    aaa
        )");
        CheckInvalid(MSG_NOT_ALLOWED_IN_ROOT_OF_FORM, R"(
            form f:
                root:
                    %cover_fillers
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_NOT_ALLOWED_IN_LIST_OF_VALUES) {
        CheckInvalid(MSG_NOT_ALLOWED_IN_LIST_OF_VALUES, R"(
            entity custom.my:
                values:
                    v1:
                        %anchor_to_begin
                        aaa
                    v2:
                        bbb
        )");
        CheckValid(R"(
            entity custom.my:
                values:
                    v1:
                        aaa
                    v2:
                        bbb
        )");
        CheckValid(R"(
            entity custom.my:
                root:
                    %anchor_to_begin
                    %value v1
                    aaa
                    %value v2
                    bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_NOT_ALLOWED_HERE) {
        // Can't be
    }

    Y_UNIT_TEST(Test_MSG_INVALID_ARGUMENT) {
        CheckInvalid(MSG_INVALID_ARGUMENT, R"(
            form f:
                root:
                    %fillers ne_nado
                    aaa
        )");
        CheckValid(R"(
            form f:
                root:
                    %fillers off
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_WEIGHT) {
        CheckInvalid(MSG_INVALID_WEIGHT, R"(
            form f:
                root:
                    aaa
                    %weight very_big
                    bbb
        )");
        CheckValid(R"(
            form f:
                root:
                    aaa
                    %weight 1e+10
                    bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNEXPECTED_PARAM_OF_DIRECTIVE) {
        CheckInvalid(MSG_UNEXPECTED_PARAM_OF_DIRECTIVE, R"(
            form f:
                root:
                    aaa
                    %negative on
                    bbb
        )");
        CheckValid(R"(
            form f:
                root:
                    aaa
                    %negative
                    bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_EMPTY_EXPRESSION) {
        CheckInvalid(MSG_EMPTY_EXPRESSION, R"(
            form f:
                root:
                    []
        )");
    }

    Y_UNIT_TEST(Test_MSG_MODIFIER_ONLY_FOR_ELEMENTS) {
        CheckInvalid(MSG_MODIFIER_ONLY_FOR_ELEMENTS, R"(
            form f:
                root:
                    слово<g:nom>
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_QUANTIFIER) {
        CheckInvalid(MSG_INVALID_QUANTIFIER, R"(
            form f:
                root:
                    $B<5,1>
            $B:
                bbb
        )");
        CheckValid(R"(
            form f:
                root:
                    $B<1,5>
            $B:
                bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_NO_WORDS_AFTER_NORMALIZATION) {
        // TODO(samoylovboris)
    }

    Y_UNIT_TEST(Test_MSG_TOO_MANY_OPERANDS_IN_BAG) {
        CheckInvalid(MSG_TOO_MANY_OPERANDS_IN_BAG, R"(
            form f:
                root:
                    [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33]
        )");
        CheckValid(R"(
            form f:
                root:
                    [1 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32]
        )");
    }

    Y_UNIT_TEST(Test_MSG_COMMENTS_INSIDE_BRACKETS) {
        CheckInvalid(MSG_COMMENTS_INSIDE_BRACKETS, R"(
            form f:
                root: [
                    aaa
                    # comment
                    bbb
                ]
        )");
        CheckValid(R"(
            form f:
                root: [
                    aaa
                    bbb
                ]
        )");
    }

    Y_UNIT_TEST(Test_MSG_OPENING_BRACKET_NOT_FOUND) {
        CheckInvalid(MSG_OPENING_BRACKET_NOT_FOUND, R"(
            form f:
                root: ]
        )");
    }

    Y_UNIT_TEST(Test_MSG_CLOSING_QUOTE_NOT_FOUND) {
        CheckInvalid(MSG_CLOSING_QUOTE_NOT_FOUND, R"(
            form f:
                root: "
        )");
    }

    Y_UNIT_TEST(Test_MSG_CLOSING_BRACKET_NOT_FOUND) {
        CheckInvalid(MSG_CLOSING_BRACKET_NOT_FOUND, R"(
            form f:
                root: (
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNEXPECTED_INDENT) {
        CheckInvalid(MSG_UNEXPECTED_INDENT, R"(
            form f:
                root:
                    $A
            $A
                aaa
        )");
        CheckValid(R"(
            form f:
                root:
                    $A
            $A:
                aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_TAB_SYMBOL_IN_INDENT) {
        CheckInvalid(MSG_TAB_SYMBOL_IN_INDENT, "$A:\n\taaa");
        CheckValid("$A:\n    aaa");
    }

    Y_UNIT_TEST(Test_MSG_EMPTY_KEY) {
        CheckInvalid(MSG_EMPTY_KEY, R"(
            :
                aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_IMPLICIT_EMPTY_TOKEN_IS_FORBIDEN) {
        CheckInvalid(MSG_IMPLICIT_EMPTY_TOKEN_IS_FORBIDEN, R"(
            form f:
                root: | aaa | bbb
        )");
        CheckValid(R"(
            form f:
                root: (aaa | bbb)?
        )");
        CheckValid(R"(
            form f:
                root: $sys.void | aaa | bbb
        )");
    }

    Y_UNIT_TEST(Test_MSG_INVALID_DIRECTIVE) {
        CheckInvalid(MSG_INVALID_DIRECTIVE, R"(
            %include: path  # should be "%include path"
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNEXPECTED_TOKEN) {
        CheckInvalid(MSG_UNEXPECTED_TOKEN, R"(
            form f:
                root:
                    *
        )");
        CheckValid(R"(
            form f:
                root:
                    a*
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNEXPECTED_SYMBOL) {
        CheckInvalid(MSG_UNEXPECTED_SYMBOL, R"(
            form f:
                root:
                    @
        )");
    }

    Y_UNIT_TEST(Test_MSG_NO_VALUE) {
        CheckInvalid(MSG_NO_VALUE, R"(
            form f:
                keep_variants:
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_NOT_A_VALUE) {
        CheckInvalid(MSG_NOT_A_VALUE, R"(
            form f:
                keep_variants:
                    true:
                        true
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_BOOLEAN_EXPECTED) {
        CheckInvalid(MSG_BOOLEAN_EXPECTED, R"(
            form f:
                keep_variants: давай
                root:
                    aaa
        )");
    }

    Y_UNIT_TEST(Test_MSG_UNSIGNED_INT_EXPECTED) {
        CheckInvalid(MSG_UNSIGNED_INT_EXPECTED, R"(
            form f:
                freshness: true
                root:
                    aaa
        )");
    }
}
