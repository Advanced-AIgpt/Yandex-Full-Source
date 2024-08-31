#include "finder_impl.h"
#include <alice/nlu/granet/lib/sample/entity_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/token/charfilter.h>
#include <util/stream/trace.h>
#include <util/string/join.h>

#define TRACE_LINE(log, arg) if (log) {*log << arg << Endl;}
#define TRACE_TEXT(log, arg) if (log) {*log << arg;}

namespace NGranet::NUserEntity {

// ~~~~ TEntityFinder ~~~~

TEntityFinder::TEntityFinder(const TVector<TString>& requestVariants, TStringBuf destAlignTokens,
        const TLangMask& languages, IOutputStream* log)
    : Log(log)
    , TokenBuilder(languages)
    , DestAlignTokens(destAlignTokens)
{
    TRACE_LINE(Log, "TEntityFinder:");

    // Optimization: skip duplicated variants.
    THashSet<TUtf16String> setOfNormalizedVariants;

    Requests.reserve(requestVariants.size());

    for (const TString& variant : requestVariants) {
        TRequest request;
        TUtf16String tokenized;
        request.Tokens = TokenBuilder.Tokenize(UTF8ToWide(variant), &tokenized);
        if (request.Tokens.empty()) {
            continue;
        }
        const TUtf16String normalized = NormalizeUnicode(tokenized);
        const auto [it, isNew] = setOfNormalizedVariants.insert(normalized);
        if (!isNew) {
            continue;
        }
        request.Alignment = NNlu::TTokenAligner().Align(WideToUTF8(tokenized), DestAlignTokens);

        TRACE_LINE(Log, "  Request variant " << Requests.size() << ": " << normalized);
        TRACE_LINE(Log, "    Alignment: " << request.Alignment.WriteToString());

        Requests.push_back(std::move(request));
    }
    TRACE_LINE(Log, "  Dest align tokens: " << DestAlignTokens);

    RequestTokenIdRangeEnd = TokenBuilder.GetIdCounterValue();
}

void TEntityFinder::Find(const TEntityDicts& dicts, TVector<TEntity>* entities) {
    for (const TEntityDictPtr& dict : dicts.Dicts) {
        Find(*dict, entities);
    }
}

void TEntityFinder::Find(const TEntityDict& dict, TVector<TEntity>* entities) {
    TRACE_LINE(Log, "  Find " << dict.EntityName << ':');

    GenerateHypotheses(dict);
    DumpHypotheses("after generate");

    FilterHypotheses();
    DumpHypotheses("after filter");

    WriteEntities(dict, entities);

    CleanUp();
}

void TEntityFinder::GenerateHypotheses(const TEntityDict& dict) {
    Hypotheses.clear();

    for (size_t i = 0; i < dict.Items.size(); ++i) {
        const TVector<TTokenInfo> tokens = TokenBuilder.Tokenize(UTF8ToWide(dict.Items[i].Text));

        GenerateWholeHypotheses(i, tokens);
        if (dict.Flags.HasFlags(EDF_ENABLE_PARTIAL_MATCH)) {
            GeneratePartialHypotheses(i, tokens, false);
            GeneratePartialHypotheses(i, tokens, true);
        }
    }
}

void TEntityFinder::GenerateWholeHypotheses(size_t itemIndex, const TVector<TTokenInfo>& tokens) {
    int strongWordCount = 0;
    for (const TTokenInfo& token : tokens) {
        if (!token.IsStrong()) {
            continue;
        }
        if (!CanPresentInRequest(token)) {
            return;
        }
        strongWordCount++;
    }
    const TTokenRange itemRange(tokens, 0, tokens.size());
    GenerateHypotheses(itemIndex, itemRange, strongWordCount);
}

static const TTokenInfo& GetToken(const TVector<TTokenInfo>& tokens, size_t index, bool fromEnd) {
    Y_ASSERT(index < tokens.size());
    if (fromEnd) {
        return tokens[tokens.size() - 1 - index];
    } else {
        return tokens[index];
    }
}

void TEntityFinder::GeneratePartialHypotheses(size_t itemIndex, const TVector<TTokenInfo>& tokens, bool fromEnd) {
    int strongWordCount = 0;
    size_t length = 0;
    while (length < tokens.size()) {
        // Example of generated prefix hypotheses:
        //   Token is strong:           -   +   -   -   +   -   -   +   -   +   -
        //   Token can be in request:   -   +   +   -   +   +   -   -   +   +   +
        //   Hypothesis 1:            |-------|
        //   Hypothesis 2:            |----------------------|
        while (length < tokens.size() && !GetToken(tokens, length, fromEnd).IsStrong()) {
            length++;
        }
        if (length == tokens.size()) {
            // Whole hypothesis generated in GenerateWholeHypotheses.
            break;
        }
        Y_ASSERT(GetToken(tokens, length, fromEnd).IsStrong());
        if (!CanPresentInRequest(GetToken(tokens, length, fromEnd))) {
            return;
        }
        length++;
        strongWordCount++;
        while (length < tokens.size()
            && !GetToken(tokens, length, fromEnd).IsStrong()
            && CanPresentInRequest(GetToken(tokens, length, fromEnd)))
        {
            length++;
        }
        if (length == tokens.size()) {
            // Whole hypothesis generated in GenerateWholeHypotheses.
            break;
        }
        if (fromEnd && strongWordCount < 2) {
            continue;
        }
        const size_t offset = fromEnd ? (tokens.size() - length) : 0;
        const TTokenRange itemRange(tokens, offset, length);
        GenerateHypotheses(itemIndex, itemRange, strongWordCount);
    }
}

// Fast check to break unpromissing search.
bool TEntityFinder::CanPresentInRequest(const TTokenInfo& token) const {
    return token.Lemma < RequestTokenIdRangeEnd;
}

void TEntityFinder::GenerateHypotheses(size_t itemIndex, const TTokenRange& itemRange, int itemStrongWordCount) {
    for (const TRequest& request : Requests) {
        const TVector<TTokenInfo>& tokens = request.Tokens;
        for (size_t from = 0; from < tokens.size(); ++from) {
            if (!itemRange.HasLemma(tokens[from])) {
                continue;
            }

            // Expand interval [from, to) until requestStrongWordCount < itemStrongWordCount.
            int requestStrongWordCount = 0;
            size_t to = from;
            while (to < tokens.size()
                && requestStrongWordCount < itemStrongWordCount)
            {
                const TTokenInfo& token = tokens[to];
                if (token.IsStrong()) {
                    if (!itemRange.HasLemma(token)) {
                        break;
                    }
                    requestStrongWordCount++;
                }
                to++;
            }
            if (requestStrongWordCount < itemStrongWordCount) {
                continue;
            }
            while (to < tokens.size()
                && !tokens[to].IsStrong()
                && itemRange.HasLemma(tokens[to]))
            {
                to++;
            }

            Y_ASSERT(requestStrongWordCount == itemStrongWordCount);
            const TTokenRange requestRange(tokens, from, to - from);
            GenerateHypothesis(itemIndex, itemRange, request, requestRange);
        }
    }
}

void TEntityFinder::GenerateHypothesis(size_t itemIndex, const TTokenRange& itemRange,
    const TRequest& request, const TTokenRange& requestRange)
{
    // At this moment we know that:
    //   - requestRange and itemRange have same number of strong lemmas,
    //   - all strong lemmas of requestRange are present in itemRange,
    //   - all strong lemmas of itemRange can present (CanPresentInRequest) in requestRange.
    // Now we must check that:
    //   - all strong lemmas of itemRange are present in requestRange.
    for (const TTokenInfo& token : itemRange.Range) {
        if (token.IsStrong() && !requestRange.HasLemma(token)) {
            return;
        }
    }

    // Calculate weight of hypothesis.
    double weight = 0.;
    for (const TTokenInfo& itemToken : itemRange.Range) {
        bool hasLemmaMatch = false;
        bool hasExactMatch = false;
        for (const TTokenInfo& requestToken : requestRange.Range) {
            hasLemmaMatch = hasLemmaMatch || requestToken.Lemma == itemToken.Lemma;
            hasExactMatch = hasExactMatch || requestToken.Normalized == itemToken.Normalized;
        }
        if (itemToken.IsStrong()) {
            if (hasExactMatch) {
                weight += 1.;
            } else if (hasLemmaMatch) {
                weight += 0.7;
            }
        } else {
            if (hasExactMatch) {
                weight += 0.2;
            } else if (hasLemmaMatch) {
                weight += 0.1;
            }
        }
    }

    // Penalty for partial match
    const double rangeLength = itemRange.Range.size();
    const double wholeLength = itemRange.WholeLength;
    Y_ASSERT(wholeLength > 0);
    weight -= (wholeLength - rangeLength) / wholeLength / 2;

    // Align hypothesis.
    const NNlu::TInterval interval = request.Alignment.GetMap1To2().ConvertInterval(requestRange.Interval);
    if (interval.Empty()) {
        return;
    }

    // Add hypothesis.
    THypothesis hypothesis;
    hypothesis.ItemIndex = itemIndex;
    hypothesis.Interval = interval;
    hypothesis.Weight = weight;
    Hypotheses.push_back(hypothesis);
}

void TEntityFinder::FilterHypotheses() {
    for (size_t index1 = 0; index1 < Hypotheses.size(); ++index1) {
        TMaybe<THypothesis>& hyp1 = Hypotheses[index1];
        for (size_t index2 = index1 + 1; index2 < Hypotheses.size(); ++index2) {
            TMaybe<THypothesis>& hyp2 = Hypotheses[index2];
            if (!hyp1.Defined()) {
                break;
            }
            if (!hyp2.Defined()) {
                continue;
            }
            if (!hyp1->Interval.Overlaps(hyp2->Interval)) {
                continue;
            }
            if (hyp1->Weight < hyp2->Weight) {
                hyp1.Clear();
            } else {
                hyp2.Clear();
            }
        }
    }
}

void TEntityFinder::WriteEntities(const TEntityDict& dict, TVector<TEntity>* entities) const {
    Y_ENSURE(entities);
    for (const TMaybe<THypothesis>& hyp : Hypotheses) {
        if (!hyp.Defined()) {
            continue;
        }
        const TEntityDictItem& item = dict.Items[hyp->ItemIndex];
        TEntity entity;
        entity.Interval = hyp->Interval;
        entity.Type = dict.EntityName;
        entity.Value = item.Value;
        entity.Flags = item.EntityFlags;
        entity.Source = NEntitySources::USER_ENTITY_FINDER;
        entity.LogProbability = NEntityLogProbs::USER;
        entities->push_back(std::move(entity));
    }
}

void TEntityFinder::DumpHypotheses(TStringBuf caption) {
    if (!Log) {
        return;
    }
    *Log << "    Hypotheses " << caption << ':' << Endl;
    const TVector<TStringBuf> tokens = StringSplitter(DestAlignTokens).Split(' ').SkipEmpty().ToList<TStringBuf>();
    for (const TMaybe<THypothesis>& hyp : Hypotheses) {
        if (!hyp.Defined()) {
            continue;
        }
        TStringBuilder line;
        *Log << Sprintf("      %3.2f [%2d, %2d) ", hyp->Weight,
            static_cast<int>(hyp->Interval.Begin),
            static_cast<int>(hyp->Interval.End));
        size_t tokenIndex = 0;
        for (const auto& token : StringSplitter(DestAlignTokens).Split(' ').SkipEmpty()) {
            *Log << ' ';
            if (hyp->Interval.Contains(tokenIndex)) {
                *Log << token.Token();
            } else {
                *Log << TString(GetNumberOfUTF8Chars(token.Token()), '_');
            }
            tokenIndex++;
        }
        *Log << Endl;
    }
}

void TEntityFinder::CleanUp() {
    Hypotheses.clear();
}

} // namespace NGranet::NUserEntity
