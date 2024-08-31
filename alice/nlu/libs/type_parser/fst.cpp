#include "fst.h"
#include <alice/nlu/libs/fst/fst_base.h>
#include <util/generic/hash.h>

namespace NAlice {
    namespace {
        const THashMap<TString, EEntityType> FstTypeToEntityType {
            {"TIME", EEntityType::TIME}
        };

        EEntityType GetEntityType(const TString& fstType) {
            const auto value = FstTypeToEntityType.find(fstType);
            if (value == FstTypeToEntityType.end()) {
                return EEntityType::UNKNOWN;
            }
            return value->second;
        }
    } // namespace anonymous

    TFstTypeParser::TFstTypeParser(THolder<TFstBase>&& fst)
        : Fst(std::move(fst))
    {
    }

    void TFstTypeParser::Parse(const TArrayRef<const TString>& textTokens, TEntityParsingResult* entities) const {
        Y_ASSERT(entities);

        const auto fstResult = Fst->Parse(JoinTokens(textTokens));
        for (const auto& entity : fstResult) {
            const auto type = GetEntityType(entity.ParsedToken.Type);
            if (type == EEntityType::UNKNOWN) {
                continue;
            }
            AddEntity(TParsedEntity{
                {entity.Start, entity.End},
                entity.ParsedToken.Value.ToJsonSafe(),
                entity.ParsedToken.StringValue,
                type
            }, entities);
        }
    }
} // namespace NAlice
