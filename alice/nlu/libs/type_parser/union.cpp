#include "union.h"

namespace NAlice {
    TUnionTypeParser::TUnionTypeParser(TVector<TTypeParser*>&& parsers)
        : Parsers(parsers.begin(), parsers.end())
    {
    }

    void TUnionTypeParser::Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const {
        for (const auto& parser : Parsers) {
            parser->Parse(textTokens, entities);
        }
    }
} // namespace NAlice
