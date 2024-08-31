#pragma once
#include "type_parser.h"

namespace NAlice {
    class TUnionTypeParser : public TTypeParser {
    public:
        TUnionTypeParser(TVector<TTypeParser*>&& parsers);

    public:
        void Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const override;

    private:
        const TVector<THolder<TTypeParser>> Parsers;
    };
} // namespace NAlice
