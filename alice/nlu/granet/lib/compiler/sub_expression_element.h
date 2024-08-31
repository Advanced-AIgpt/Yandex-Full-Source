#pragma once

#include "compiler_data.h"
#include "directives.h"
#include "element_modification.h"
#include <alice/nlu/granet/lib/grammar/token_id.h>
#include <alice/nlu/libs/tuple_like_type/tuple_like_type.h>
#include <util/generic/vector.h>

namespace NGranet::NCompiler {

// ~~~~ TSubExpressionElementParams ~~~~

struct TSubExpressionElementParams {
    bool EnableFillers = false;
    ESynonymFlags EnableSynonymFlagsMask = 0;
    ESynonymFlags EnableSynonymFlags = 0;

    DECLARE_TUPLE_LIKE_TYPE(TSubExpressionElementParams, EnableFillers, EnableSynonymFlagsMask, EnableSynonymFlags);
};

// ~~~~ TSubExpressionElementKey ~~~~

struct TSubExpressionElementKey {
    TVector<TCompiledRule> Rules;
    TQuantityParams Quantity;
    TSubExpressionElementParams Params;

    DECLARE_TUPLE_LIKE_TYPE(TSubExpressionElementKey, Rules, Quantity, Params);
};

TString GenerateSubExpressionElementName(const TSubExpressionElementKey& data, const TTokenPool& tokenPool,
    const TDeque<TCompilerElement>& elements);

} // namespace NGranet::NCompiler

// ~~~~ global namespace ~~~~

template <>
struct THash<NGranet::NCompiler::TSubExpressionElementParams>: public TTupleLikeTypeHash {
};

template <>
struct THash<NGranet::NCompiler::TSubExpressionElementKey>: public TTupleLikeTypeHash {
};
