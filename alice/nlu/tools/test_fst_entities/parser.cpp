#include "parser.h"
#include "library/cpp/langs/langs.h"
#include <alice/nlu/libs/fst/fst_base.h>
#include <alice/nlu/tools/test_fst_entities/common.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

TParser::TParser(const TConfigFst& fstConfig)
    : FstName(fstConfig.FstName)
    , EntityTypeName(fstConfig.EntityTypeName)
    , Language(LanguageByName(fstConfig.Language))
    , Fst(MakeFst(fstConfig.FstName, Language))
{
}

TString TParser::Parse(const TString& text) {
    const TString normalizedText = NNlu::TRequestNormalizer::Normalize(Language, text);
    const TVector<NAlice::TEntity> entities = Fst->Parse(normalizedText);
    return MakeResultString(entities, normalizedText);
}

TString TParser::MakeResultString(const TVector<NAlice::TEntity>& entities, const TString& normalizedText) {
    const TVector<TString> tokens = StringSplitter(normalizedText).Split(' ');
    TStringBuilder result;
    bool hasTargetEntity = false;

    for (const auto& entity : entities) {
        if (!result.Empty()) {
            result << ' ';
        }
        if (entity.ParsedToken.Type == EntityTypeName) {
            hasTargetEntity = true;
            result << '\'';
        }
        for (size_t i = entity.Start; i < entity.End; ++i) {
            if (i != entity.Start)
                result << ' ';
            Y_ENSURE(i < tokens.size(), "Entity out the bounds of tokens");
            result << tokens[i];
        }
        if (entity.ParsedToken.Type == EntityTypeName) {
            result << "\'(" << entity.ParsedToken.Value << ')';
        }
    }

    if (hasTargetEntity)
        return result;
    else
        return "";
}
