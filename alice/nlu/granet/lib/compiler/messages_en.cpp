#include "messages_en.h"

namespace NGranet::NCompiler {

const TMessageTable MESSAGE_TABLE_EN = {
    {MSG_UNDEFINED,                         ""},

    // compiler.cpp
    {MSG_UNDEFINED_PATH,                    "Undefined path"},
    {MSG_DUPLICATED_KEY,                    "Duplicated key"},
    {MSG_NAME_IS_EMPTY,                     "Name is empty"},
    {MSG_INVALID_NAME,                      "Invalid name: \"{0}\""},
    {MSG_DUPLICATED_NAME,                   "Duplicated name: \"{0}\""},
    {MSG_DUPLICATED_ELEMENT_NAME,           "Duplicated element name: \"{0}\""},
    {MSG_UNEXPECTED_KEYWORD,                "Unexpected keyword"},
    {MSG_INVALID_SLOT_NAME,                 "Invalid slot name: \"{0}\""},
    {MSG_DUPLICATED_SLOT_NAME,              "Duplicated slot name: \"{0}\""},
    {MSG_INVALID_TYPE_NAME,                 "Invalid type name: \"{0}\""},
    {MSG_INVALID_MATCHING_TYPE,             "matching_type value can be 'exact', 'inside' or 'overlap'. You passed \"{0}\"."},
    {MSG_INVALID_CONCATENATE_STRINGS,       "concatenate_strings value is expected to have boolean value. You passed \"{0}\"."},
    {MSG_INVALID_KEEP_VARIANTS,             "keep_variants value is expected to have boolean value. You passed \"{0}\"."},
    {MSG_NO_TYPE_SECTION,                   "No type section provided"},
    {MSG_CONCATENATE_STRINGS_SUPPORTED_ONLY_FOR_TAGGER,
                                            "concatenate_strings parameter is supported only for tagger (enable_alice_tagger: true)."},
    {MSG_INVALID_ELEMENT_NAME,              "Invalid element name: \"{0}\""},
    {MSG_CAN_NOT_FIND_ELEMENT,              "Can't find element \"{0}\""},
    {MSG_AMBIGUOUS_ELEMENT_NAME,            "Ambiguous element name: \"{0}\""},
    {MSG_UNEXPECTED_PARAM_OF_MODIFIER,      "Unexpected parameter of modifier"},
    {MSG_UNKNOWN_TYPE_OF_MODIFICATION,      "Unknown type of modification: \"{0}\""},
    {MSG_GRAMMEME_PARSER_ERROR,             "Grammeme parser error: {0}"},
    {MSG_STRING_LITERAL_PARSER_ERROR,       "String literal parser error: {0}"},
    {MSG_NOT_QUOTED_STRING_PARSING_ERROR,   "Not quoted string parsing error"},
    {MSG_RECURSION_DETECTED,                "Recursion detected: {0}"},

    // directives.cpp
    {MSG_UNKNOWN_DIRECTIVE,                 "Unknown directive \"{0}\""},
    {MSG_UNKNOWN_SYNONYM_TYPE,              "Unknown synonym type \"{0}\""},
    {MSG_ALLOWED_ONLY_BEFORE_ALL_RULES,     "Directive \"{0}\" allowed only before all rules"},
    {MSG_NOT_ALLOWED_IN_FILLER,             "Directive \"{0}\" not allowed in filler"},
    {MSG_NOT_ALLOWED_IN_ROOT_OF_FORM,       "Directive \"{0}\" not allowed in root of form"},
    {MSG_NOT_ALLOWED_IN_LIST_OF_VALUES,     "Directive \"{0}\" not allowed in list of entity values"},
    {MSG_NOT_ALLOWED_HERE,                  "Directive \"{0}\" not allowed here"},
    {MSG_INVALID_ARGUMENT,                  "Invalid argument"},
    {MSG_INVALID_WEIGHT,                    "Invalid weight"},
    {MSG_UNEXPECTED_PARAM_OF_DIRECTIVE,     "Unexpected param of directive \"{0}\""},
    {MSG_SYNONYM_TYPES_ARE_EMPTY,           "Empty list of synonym types"},

    // expression_tree_builder.cpp
    {MSG_EMPTY_EXPRESSION,                  "Empty expression"},
    {MSG_MODIFIER_ONLY_FOR_ELEMENTS,        "Modifier allowed only for elements"},
    {MSG_INVALID_QUANTIFIER,                "Invalid quantifier"},
    {MSG_NO_WORDS_AFTER_NORMALIZATION,      "No words after normalization"},
    {MSG_TOO_MANY_OPERANDS_IN_BAG,          "Too many operands in bag expression (limit is 32)."},

    // preprocessor.cpp
    {MSG_ABSOLUTE_PATH,                     "Absolute path is not allowed: \"{0}\""},
    {MSG_FILE_NOT_FOUND,                    "File not found: \"{0}\""},
    {MSG_COMMENTS_INSIDE_BRACKETS,          "Comments inside brackets are not allowed"},
    {MSG_OPENING_BRACKET_NOT_FOUND,         "Opening bracket not found"},
    {MSG_CLOSING_QUOTE_NOT_FOUND,           "Closing quote not found"},
    {MSG_CLOSING_BRACKET_NOT_FOUND,         "Closing bracket not found"},
    {MSG_UNEXPECTED_INDENT,                 "Unexpected indent"},
    {MSG_TAB_SYMBOL_IN_INDENT,              "Tab symbol in indent"},
    {MSG_EMPTY_KEY,                         "Empty key"},
    {MSG_IMPLICIT_EMPTY_TOKEN_IS_FORBIDEN,  "Implicit empty token is forbiden, use $sys.void instead"},
    {MSG_INVALID_DIRECTIVE,                 "Invalid directive"},

    // rule_parser.cpp
    {MSG_UNEXPECTED_TOKEN,                  "Unexpected token"},
    {MSG_UNEXPECTED_SYMBOL,                 "Unexpected symbol"},

    // src_line.cpp
    {MSG_NO_VALUE,                          "No value"},
    {MSG_NOT_A_VALUE,                       "Not a value"},
    {MSG_BOOLEAN_EXPECTED,                  "Boolean value is expected"},
    {MSG_UNSIGNED_INT_EXPECTED,             "Unsigned int value is expected"},
};

} // namespace NGranet::NCompiler
