#pragma once

#include <alice/nlu/granet/lib/grammar/grammar_data.h>
#include <alice/nlu/granet/lib/utils/flag_utils.h>
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <kernel/lemmer/dictlib/grambitset.h>

namespace NGranet::NCompiler {

// ~~~~ ECompilerFlag ~~~~

enum ECompilerFlag : ui32 {
    // Rule options
    CF_EXACT                        = FLAG32(0),
    // TODO(samoylovboris) Merge CF_LEMMA_BEST and CF_LEMMA_GOOD to single flag CF_LEMMA
    CF_LEMMA_BEST                   = FLAG32(1),
    CF_LEMMA_GOOD                   = FLAG32(2),
    CF_LEMMA_AS_IS                  = FLAG32(3),
    CF_INFLECT_CASES                = FLAG32(4),
    CF_INFLECT_GENDERS              = FLAG32(5),
    CF_INFLECT_NUMBERS              = FLAG32(6),
    CF_NEGATIVE                     = FLAG32(8),
    CF_FORCED                       = FLAG32(9),

    // Element options
    CF_ENABLE_FILLERS               = FLAG32(16),
    CF_COVER_FILLERS                = FLAG32(17),
    CF_ANCHOR_TO_BEGIN              = FLAG32(18),
    CF_ANCHOR_TO_END                = FLAG32(19),
};

Y_DECLARE_FLAGS(ECompilerFlags, ECompilerFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(ECompilerFlags);

const ECompilerFlags CF_INFLECT_FLAGS = CF_INFLECT_CASES | CF_INFLECT_GENDERS | CF_INFLECT_NUMBERS;
const ECompilerFlags CF_LEMMA_FLAGS = CF_LEMMA_BEST | CF_LEMMA_GOOD | CF_LEMMA_AS_IS;

// ~~~~ EDirectiveContextFlag ~~~~

enum EDirectiveContextFlag : ui32 {
    DCF_BETWEEN_RULES       = FLAG32(0),
    DCF_IN_FILLER           = FLAG32(1),
    DCF_IN_ROOT_OF_FORM     = FLAG32(2),
    DCF_IN_LIST_OF_VALUES   = FLAG32(3),
};

Y_DECLARE_FLAGS(EDirectiveContextFlags, EDirectiveContextFlag);
Y_DECLARE_OPERATORS_FOR_FLAGS(EDirectiveContextFlags);

// ~~~~ TCompilerOptionsAccumulator ~~~~

struct TCompilerOptionsAccumulator {
    EDirectiveContextFlags ContextFlags;
    ECompilerFlags CompilerFlags;
    TString CustomInflection;
    ESynonymFlags EnableSynonymFlagsMask;
    ESynonymFlags EnableSynonymFlags;
    float Weight = 1.f;
    TStringId DataType = 0;
    TStringId DataValue = 0;
};

// ~~~~ Directive tools ~~~~

bool TryReadDirective(const TTextView& line, TStringPool* stringPool, TCompilerOptionsAccumulator* options);

} // namespace NGranet::NCompiler
