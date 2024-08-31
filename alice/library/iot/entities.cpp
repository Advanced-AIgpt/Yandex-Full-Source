#include "entities.h"

#include "defs.h"

#include <alice/nlu/libs/fst/fst_date_time.h>
#include <alice/nlu/libs/fst/fst_date_time_range.h>
#include <alice/nlu/libs/fst/fst_time.h>
#include <alice/nlu/libs/fst/resource_data_loader.h>
#include <alice/nlu/libs/token_aligner/aligner.h>


// Helps using << easily, when logBuilder is in scope
#define LOG(x) IOT_LOG(logBuilder, x);


namespace NAlice::NIot {

namespace {

constexpr double NONSENSE_THRESHOLD = 0.75;

const THashMap<TString, TSimpleSharedPtr<NAlice::TFstBase>> FST_PARSERS = {
    {"TIME", MakeSimpleShared<NAlice::TFstTime>(NAlice::TResourceDataLoader("fst/ru/time"))},
    {"DATETIME", MakeSimpleShared<NAlice::TFstDateTime>(NAlice::TResourceDataLoader("fst/ru/datetime"))},
    {"DATETIME_RANGE", MakeSimpleShared<NAlice::TFstDateTimeRange>(NAlice::TResourceDataLoader("fst/ru/datetime_range"))}
};

void AddFstMatches(const TVector<TString>& tokens, const ELanguage language, TUniqueEntities& resultEntities) {
    if (language != LANG_RUS) {
        return;
    }

    auto utterance = JoinSeq(" ", tokens);
    for (const auto& [type, parser] : FST_PARSERS) {
        auto fstEntities = parser->Parse(utterance);
        for (const auto& e : fstEntities) {
            if (e.ParsedToken.Type != type) {
                continue;
            }
            // Fast solution to SEARCH-11010
            // TODO(igor-darov): align tokens properly
            if (e.End > tokens.size()) {
                continue;
            }

            auto text = JoinRange(" ", tokens.begin() + e.Start, tokens.begin() + e.End);
            TString resultType = TStringBuilder() << FST_PREFIX << type;
            resultEntities.Add({e.ParsedToken.Value, resultType, text, e.Start, e.End, /* Extra */ NSc::TValue::Null()});
        }
    }
}

bool AreAllNonsense(const TNluInput& nluInput, const size_t begin, const size_t end) {
    if (begin >= end) {
        return false;
    }

    Y_ASSERT(end <= nluInput.Extra.NonsenseProbabilities.size());
    for (size_t i = begin; i < end; ++i) {
        if (nluInput.Extra.NonsenseProbabilities[i] < NONSENSE_THRESHOLD) {
            return false;
        }
    }

    return true;
}

void AddNonsenseMatches(const TNluInput& nluInput, const TVector<TString>& normalizedTokens,
                        const ELanguage language, TUniqueEntities& resultEntities) {
    if (language != LANG_RUS) {
        return;
    }

    if (nluInput.Extra.Tokens.size() == 0 || nluInput.Extra.Tokens.size() != nluInput.Extra.NonsenseProbabilities.size()) {
        return;
    }

    const auto alignment = NNlu::TTokenAligner().Align(normalizedTokens, nluInput.Extra.Tokens);
    Y_ASSERT(alignment.GetSegments1().size() == alignment.GetSegments2().size());

    size_t begin1 = 0;
    size_t begin2 = 0;
    for (size_t groupId = 0; groupId < alignment.GetSegments1().size(); ++groupId) {
        const auto end1 = begin1 + alignment.GetSegments1()[groupId];
        const auto end2 = begin2 + alignment.GetSegments2()[groupId];
        if (AreAllNonsense(nluInput, begin2, end2)) {
            Y_ASSERT(end1 <= normalizedTokens.size());
            const auto text = JoinRange(" ", normalizedTokens.begin() + begin1, normalizedTokens.begin() + end1);
            resultEntities.Add({"nonsense", "common", text, begin1, end1, /* Extra */ NSc::TValue::Null()});
        }
        begin1 = end1;
        begin2 = end2;
    }
}

}  // namespace


TVector<TRawEntity> ComputeFstEntities(const TVector<TString>& tokens, ELanguage language) {
    TUniqueEntities entities;
    AddFstMatches(tokens, language, entities);
    return entities.Get();
}

TVector<TRawEntity> ComputeNonsenseEntities(const TNluInput& nluInput, const TVector<TString>& tokens, ELanguage language) {
    TUniqueEntities entities;
    AddNonsenseMatches(nluInput, tokens, language, entities);
    return entities.Get();
}


}  // namespace NAlice::NIot
