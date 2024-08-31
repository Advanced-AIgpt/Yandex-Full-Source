#include "directives.h"
#include "compiler_check.h"
#include "syntax.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <util/generic/hash.h>
#include <util/string/escape.h>
#include <util/string/strip.h>

namespace NGranet::NCompiler {

// ~~~~ ReadDirectives ~~~~

namespace {

struct TDirectiveInfo {
    bool IsFlag = false;
    ECompilerFlags ClearCompilerFlags = 0;
    ECompilerFlags SetCompilerFlags = 0;
    EDirectiveContextFlags ProhibitedContextFlags;
};

// Directive name -> directive info
static const THashMap<TStringBuf, TDirectiveInfo> DirectiveInfoTable = {
    {NSyntax::NDirective::Exact,            {true,  0, CF_EXACT,                                        0}},
    {NSyntax::NDirective::Lemma,            {true,  CF_EXACT | CF_LEMMA_FLAGS, CF_LEMMA_BEST,           0}},
    {NSyntax::NDirective::LemmaAsIs,        {true,  CF_EXACT | CF_LEMMA_FLAGS, CF_LEMMA_AS_IS,          0}},
    {NSyntax::NDirective::InflectCases,     {true,  CF_EXACT | CF_LEMMA_FLAGS, CF_INFLECT_CASES,        0}},
    {NSyntax::NDirective::InflectGenders,   {true,  CF_EXACT | CF_LEMMA_FLAGS, CF_INFLECT_GENDERS,      0}},
    {NSyntax::NDirective::InflectNumbers,   {true,  CF_EXACT | CF_LEMMA_FLAGS, CF_INFLECT_NUMBERS,      0}},
    {NSyntax::NDirective::Weight,           {false, 0, 0,                                               0}},
    {NSyntax::NDirective::Value,            {false, 0, 0,                                               DCF_IN_LIST_OF_VALUES}},
    {NSyntax::NDirective::Type,             {false, 0, 0,                                               DCF_IN_LIST_OF_VALUES}},
    {NSyntax::NDirective::Negative,         {false, CF_FORCED | CF_NEGATIVE, CF_NEGATIVE,               0}},
    {NSyntax::NDirective::Positive,         {false, CF_FORCED | CF_NEGATIVE, 0,                         0}},
    {NSyntax::NDirective::ForceNegative,    {false, CF_FORCED | CF_NEGATIVE, CF_FORCED | CF_NEGATIVE,   0}},
    {NSyntax::NDirective::ForcePositive,    {false, CF_FORCED | CF_NEGATIVE, CF_FORCED,                 0}},
    {NSyntax::NDirective::EnableSynonyms,   {false, 0, 0,                                               DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES}},
    {NSyntax::NDirective::DisableSynonyms,  {false, 0, 0,                                               DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES}},
    {NSyntax::NDirective::AnchorToBegin,    {true,  0, CF_ANCHOR_TO_BEGIN,                              DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES | DCF_IN_ROOT_OF_FORM}},
    {NSyntax::NDirective::AnchorToEnd,      {true,  0, CF_ANCHOR_TO_END,                                DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES | DCF_IN_ROOT_OF_FORM}},
    {NSyntax::NDirective::CoverFillers,     {true,  0, CF_ENABLE_FILLERS | CF_COVER_FILLERS,            DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES | DCF_IN_ROOT_OF_FORM | DCF_IN_FILLER}},
    {NSyntax::NDirective::Fillers,          {true,  0, CF_ENABLE_FILLERS,                               DCF_BETWEEN_RULES | DCF_IN_LIST_OF_VALUES | DCF_IN_FILLER}},
};

} // namespace

bool TryReadDirective(const TTextView& source, TStringPool* stringPool, TCompilerOptionsAccumulator* options) {
    Y_ENSURE(stringPool);
    Y_ENSURE(options);

    TStringBuf line = source.Str();
    line = StripString(line);
    if (!line.StartsWith('%')) {
        return false;
    }
    const TString directive = TString(line.NextTok(' '));
    line = StripString(line);

    const TDirectiveInfo* info = DirectiveInfoTable.FindPtr(directive);
    GRANET_COMPILER_CHECK(info, source, MSG_UNKNOWN_DIRECTIVE, directive);

    // Check location restrictions
    const EDirectiveContextFlags violation = options->ContextFlags & info->ProhibitedContextFlags;
    if (violation) {
        GRANET_COMPILER_CHECK(!violation.HasFlags(DCF_IN_FILLER), source, MSG_NOT_ALLOWED_IN_FILLER, directive);
        GRANET_COMPILER_CHECK(!violation.HasFlags(DCF_IN_ROOT_OF_FORM), source, MSG_NOT_ALLOWED_IN_ROOT_OF_FORM, directive);
        GRANET_COMPILER_CHECK(!violation.HasFlags(DCF_IN_LIST_OF_VALUES), source, MSG_NOT_ALLOWED_IN_LIST_OF_VALUES, directive);
        GRANET_COMPILER_CHECK(!violation.HasFlags(DCF_BETWEEN_RULES), source, MSG_ALLOWED_ONLY_BEFORE_ALL_RULES, directive);
        GRANET_COMPILER_CHECK(false, source, MSG_NOT_ALLOWED_HERE, directive);
    }

    // Apply directive to compiler options
    options->CompilerFlags &= ~info->ClearCompilerFlags;
    bool isOptionTrue = true;
    if (info->IsFlag && !line.empty()) {
        GRANET_COMPILER_CHECK(TryFromString(line, isOptionTrue), source, MSG_INVALID_ARGUMENT);
        line.Clear();
    }
    if (isOptionTrue) {
        options->CompilerFlags |= info->SetCompilerFlags;
    } else {
        options->CompilerFlags &= ~info->SetCompilerFlags;
    }

    if (directive == NSyntax::NDirective::EnableSynonyms) {
        ReadSynonyms(source, line, options->EnableSynonymFlagsMask, options->EnableSynonymFlags, true);
        return true;
    }
    if (directive == NSyntax::NDirective::DisableSynonyms) {
        ReadSynonyms(source, line, options->EnableSynonymFlagsMask, options->EnableSynonymFlags, false);
        return true;
    }
    if (directive == NSyntax::NDirective::Weight) {
        const bool ok = TryFromString(line, options->Weight) && options->Weight > 0;
        GRANET_COMPILER_CHECK(ok, source, MSG_INVALID_WEIGHT);
        return true;
    }
    if (directive == NSyntax::NDirective::Type) {
        options->DataType = stringPool->Insert(SafeUnquote(source, line));
        return true;
    }
    if (directive == NSyntax::NDirective::Value) {
        options->DataValue = stringPool->Insert(SafeUnquote(source, line));
        return true;
    }
    GRANET_COMPILER_CHECK(line.empty(), source, MSG_UNEXPECTED_PARAM_OF_DIRECTIVE, directive);
    return true;
}

} // namespace NGranet::NCompiler
