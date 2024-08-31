#include "type_parser.h"
#include <util/generic/algorithm.h>
#include <util/string/join.h>

namespace NAlice {
    TEntityParsingResult TTypeParser::Parse(const TArrayRef<const TString>& textTokens) const {
        TEntityParsingResult entities;
        Parse(textTokens, &entities);
        return entities;
    }

    void TTypeParser::Parse(const TArrayRef<const TString>& /*textTokens*/, TEntityParsingResult* entities) const {
        Y_ASSERT(entities);
    }

    TString TTypeParser::JoinTokens(const TArrayRef<const TString>& textTokens) const {
        Y_ENSURE(AllOf(textTokens, [](const TString& token) {
            return AllOf(token, [](const char& symbol) {
                return symbol != ' ';
            });
        }));

        return JoinSeq(" ", textTokens);
    }

    void TTypeParser::AddEntity(TParsedEntity&& entity, TEntityParsingResult* entities) const {
        (*entities)[entity.Type].emplace_back(entity);
    }
} // namespace NAlice
