#pragma once

#include "directives.h"
#include <alice/nlu/granet/lib/utils/text_view.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <kernel/lemmer/dictlib/grambitset.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

// ~~~~ TElementModification ~~~~

struct TElementModification {
    // Subset of CF_LEMMA_FLAGS | CF_INFLECT_FLAGS
    ECompilerFlags CompilerFlags;
    TString CustomInflection;

    DECLARE_TUPLE_LIKE_TYPE(TElementModification, CompilerFlags, CustomInflection)
};

// ~~~~ Utils ~~~~

TMaybe<TVector<TGramBitSet>> MakeModificationGramsList(ECompilerFlags compilerFlags, TStringBuf customInflection);
TElementModification OverrideModification(const TElementModification& prev, const TElementModification& curr);
TElementModification ParseElementModification(TStringBuf modificationStr, const TTextView& source);

} // namespace NGranet::NCompiler

// ~~~~ global namespace ~~~~

template <>
struct THash<NGranet::NCompiler::TElementModification>: public TTupleLikeTypeHash {
};
