#include "dictionary.h"
#include <util/string/join.h>
#include <util/string/split.h>

namespace NAlice {
    TDictionaryTypeParser::TDictionaryTypeParser(const TBlob& annotator)
        : Annotator(annotator)
    {
    }

    void TDictionaryTypeParser::Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const {
        Y_ASSERT(entities);

        const auto annotationsByClass = Annotator.Annotate(JoinTokens(textTokens)); // TODO(smirnovpavel): pass tokens
        for (const auto& classAnnotations : annotationsByClass) {
            TString entityValue;
            EEntityType entityType;
            NormalizeValue(classAnnotations.ClassData, &entityValue, &entityType);

            for (const auto& position : classAnnotations.OccurencePositions) {
                AddEntity(TParsedEntity{
                    {position.StartToken, position.EndToken},
                    entityValue,
                    JoinRange(" ", textTokens.begin() + position.StartToken, textTokens.begin() + position.EndToken),
                    entityType
                }, entities);
            }
        }
    }

    void TDictionaryTypeParser::NormalizeValue(const TString& rawValue, TString* value, EEntityType* type) const {
        *value = rawValue;
        *type = EEntityType::TEXT;
    }
} // namespace NAlice
