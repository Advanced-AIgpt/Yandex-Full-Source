#pragma once
#include "types.h"
#include <alice/nlu/libs/interval/interval.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/array_ref.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {
    struct TParsedEntity {
        NNlu::TInterval Interval;
        TString Value;
        TString Text;
        EEntityType Type;
    };

    // entities by type
    using TEntityParsingResult = THashMap<EEntityType, TVector<TParsedEntity>>;

    class TTypeParser {
    public:
        virtual ~TTypeParser() = default;

        // 'text' argument must be fst-normalized (see alice/nlu/libs/request_normalizer/request_normalizer.h)
        TEntityParsingResult Parse(const TArrayRef<const TString>& textTokens) const;
        virtual void Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const;

    protected:
        TString JoinTokens(const TArrayRef<const TString>& textTokens) const;
        void AddEntity(TParsedEntity&& entity, TEntityParsingResult* entities) const;
    };
} // namespace NAlice
