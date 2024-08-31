#include "messages_ru.h"

namespace NGranet::NCompiler {

const TMessageTable MESSAGE_TABLE_RU = {
    {MSG_UNDEFINED,                         ""},

    // compiler.cpp
    {MSG_UNDEFINED_PATH,                    "Неизвестный путь"},
    {MSG_DUPLICATED_KEY,                    "Дублирующийся ключ"},
    {MSG_NAME_IS_EMPTY,                     "Пустой ID формы"},
    {MSG_INVALID_NAME,                      "Некорректный ID формы: \"{0}\""},
    {MSG_DUPLICATED_NAME,                   "Дублирующийся ID формы: \"{0}\""},
    {MSG_DUPLICATED_ELEMENT_NAME,           "Дублирующееся название элемента: \"{0}\""},
    {MSG_UNEXPECTED_KEYWORD,                "Некорректное ключевое слово"},
    {MSG_INVALID_SLOT_NAME,                 "Некорректное название слота: \"{0}\""},
    {MSG_DUPLICATED_SLOT_NAME,              "Дублирующееся название слота: \"{0}\""},
    {MSG_INVALID_TYPE_NAME,                 "Некорректное название типа: \"{0}\""},
    {MSG_INVALID_MATCHING_TYPE,             "Параметр matching_type может принимать только булево значение 'exact', 'inside' или 'overlap'. Передано \"{0}\"."},
    {MSG_INVALID_CONCATENATE_STRINGS,       "Параметр concatenate_strings может принимать только булево значение. Передано \"{0}\"."},
    {MSG_INVALID_KEEP_VARIANTS,             "Параметр keep_variants может принимать только булево значение. Передано \"{0}\"."},
    {MSG_NO_TYPE_SECTION,                   "Отсутствует секция type"},
    {MSG_CONCATENATE_STRINGS_SUPPORTED_ONLY_FOR_TAGGER,
                                            "Параметр concatenate_strings поддерживается только для теггера (enable_alice_tagger: true)."},
    {MSG_INVALID_ELEMENT_NAME,              "Некорректное название элемента \"{0}\""},
    {MSG_CAN_NOT_FIND_ELEMENT,              "Неизвестный элемент \"{0}\""},
    {MSG_AMBIGUOUS_ELEMENT_NAME,            "<TRANSLATE> Ambiguous element name: \"{0}\""},
    {MSG_UNEXPECTED_PARAM_OF_MODIFIER,      "Некорректный параметр модификации"},
    {MSG_UNKNOWN_TYPE_OF_MODIFICATION,      "Неизвестный тип модификации: \"{0}\""},
    {MSG_GRAMMEME_PARSER_ERROR,             "Некорректная граммема: {0}"},
    {MSG_STRING_LITERAL_PARSER_ERROR,       "Ошибка разбора строки: {0}"},
    {MSG_NOT_QUOTED_STRING_PARSING_ERROR,   "Значение должно быть заключено в кавычки"},
    {MSG_RECURSION_DETECTED,                "Рекурсия: {0}"},

    // directives.cpp
    {MSG_UNKNOWN_DIRECTIVE,                 "Неизвестная директива \"{0}\""},
    {MSG_UNKNOWN_SYNONYM_TYPE,              "Неизвестный тип синонима \"{0}\""},
    {MSG_ALLOWED_ONLY_BEFORE_ALL_RULES,     "Директива \"{0}\" может быть использована только в начале блока"},
    {MSG_NOT_ALLOWED_IN_FILLER,             "Директива \"{0}\" не может быть использована внутри элемента filler"},
    {MSG_NOT_ALLOWED_IN_ROOT_OF_FORM,       "Директива \"{0}\" не может быть использована внутри элемента root"},
    {MSG_NOT_ALLOWED_IN_LIST_OF_VALUES,     "Директива \"{0}\" не может быть использована внутри списка значений сущности"},
    {MSG_NOT_ALLOWED_HERE,                  "Директива \"{0}\" не может быть использована в этом месте"},
    {MSG_INVALID_ARGUMENT,                  "Некорректный аргумент"},
    {MSG_INVALID_WEIGHT,                    "Некорректный вес"},
    {MSG_UNEXPECTED_PARAM_OF_DIRECTIVE,     "Некорректный параметр директивы \"{0}\""},
    {MSG_SYNONYM_TYPES_ARE_EMPTY,           "Пустой список типов синонимов"},

    // expression_tree_builder.cpp
    {MSG_EMPTY_EXPRESSION,                  "Пустое выражение"},
    {MSG_MODIFIER_ONLY_FOR_ELEMENTS,        "Модификатор разрешен только для элементов"},
    {MSG_INVALID_QUANTIFIER,                "Некорректный квантификатор"},
    {MSG_NO_WORDS_AFTER_NORMALIZATION,      "<TRANSLATE> No words after normalization"},
    {MSG_TOO_MANY_OPERANDS_IN_BAG,          "Слишком много элементов в мешке слов (максимум 32 элемента)"},

    // preprocessor.cpp
    {MSG_ABSOLUTE_PATH,                     "Абсолютные пути запрещены: \"{0}\""},
    {MSG_FILE_NOT_FOUND,                    "Файл не найден: \"{0}\""},
    {MSG_COMMENTS_INSIDE_BRACKETS,          "<TRANSLATE> Comments inside brackets are not allowed"},
    {MSG_OPENING_BRACKET_NOT_FOUND,         "Отсутствует открывающая скобка"},
    {MSG_CLOSING_QUOTE_NOT_FOUND,           "Отсутствует закрывающая кавычка"},
    {MSG_CLOSING_BRACKET_NOT_FOUND,         "Отсутствует закрывающая скобка"},
    {MSG_UNEXPECTED_INDENT,                 "Некорректный отступ"},
    {MSG_TAB_SYMBOL_IN_INDENT,              "Табуляция в отступе"},
    {MSG_EMPTY_KEY,                         "Пустой ключ"},
    {MSG_IMPLICIT_EMPTY_TOKEN_IS_FORBIDEN,  "Пустой элемент"},
    {MSG_INVALID_DIRECTIVE,                 "Некорректная директива"},

    // rule_parser.cpp
    {MSG_UNEXPECTED_TOKEN,                  "Некорректный элемент"},
    {MSG_UNEXPECTED_SYMBOL,                 "Некорректный символ"},

    // src_line.cpp
    {MSG_NO_VALUE,                          "Отсутствует значение"},
    {MSG_NOT_A_VALUE,                       "Некорректное значение"},
    {MSG_BOOLEAN_EXPECTED,                  "Ожидается булево значение"},
    {MSG_UNSIGNED_INT_EXPECTED,             "Ожидается целое неотрицательное число"},
};

} // namespace NGranet::NCompiler
