#include "sample_features.h"

#include <alice/nlu/granet/lib/sample/tag.h>
#include <alice/nlu/libs/token_aligner/aligner.h>
#include <alice/nlu/libs/tokenization/tokenizer.h>

#include <util/generic/singleton.h>
#include <util/generic/yexception.h>
#include <util/string/join.h>

namespace NVins {

TVector<TString> GetTags(const TString& tagging, const TVector<TString>& tokens, ELanguage lang) {
    Y_ENSURE(!tagging.empty());
    Y_ENSURE(!tokens.empty());

    TVector<TString> prefixedTags(Reserve(tokens.size()));

    TString text;
    TVector<NGranet::TTag> tags;
    Y_ENSURE(NGranet::TryReadTaggerMarkup(tagging, &text, &tags));
    if (tags.empty()) {
        for (size_t tokenIndex = 0; tokenIndex < tokens.size(); ++tokenIndex) {
            prefixedTags.push_back("O");
        }
        return prefixedTags;
    }
    Y_ENSURE(tags.back().Interval.End <= text.length());

    size_t begin = 0;
    TVector<TString> textTokens;
    TVector<NNlu::TInterval> tagTokenIntervals(Reserve(tags.size()));
    for (const NGranet::TTag& tag : tags) {
        TVector<TString> t = NNlu::TSmartTokenizer(text.substr(begin, tag.Interval.Begin - begin), lang).GetOriginalTokens();
        textTokens.insert(textTokens.end(), t.begin(), t.end());
        t = NNlu::TSmartTokenizer(text.substr(tag.Interval.Begin, tag.Interval.End - tag.Interval.Begin), lang).GetOriginalTokens();
        tagTokenIntervals.push_back({textTokens.size(), textTokens.size() + t.size()});
        textTokens.insert(textTokens.end(), t.begin(), t.end());
        begin = tag.Interval.End;
    }

    const TString normalizedText = JoinSeq(" ", tokens);
    const NNlu::TAlignment alignment = NNlu::TTokenAligner().Align(textTokens, tokens);

    size_t tagIndex = 0;
    size_t normalizedTokenIndex = 0;
    for (size_t tokenIndex = 0; tokenIndex < tokens.size(); ++tokenIndex) {
        if (tagIndex >= tagTokenIntervals.size() || normalizedTokenIndex < tagTokenIntervals.at(tagIndex).Begin) {
            prefixedTags.push_back("O");
        } else {
            TString prefix = (normalizedTokenIndex == tagTokenIntervals.at(tagIndex).Begin) ? "B-" : "I-";
            TString tagName = tags.at(tagIndex).Name;
            if (tagName.StartsWith("+") || tagName.StartsWith(" ")) {
                tagName = tagName.substr(1);
                prefix = "I-";
            }
            prefixedTags.push_back(prefix + tagName);
        }
        normalizedTokenIndex = alignment.GetMap2To1().ConvertInterval({tokenIndex + 1, tokenIndex + 1}).Begin;
        if (tagIndex < tagTokenIntervals.size() && normalizedTokenIndex >= tagTokenIntervals.at(tagIndex).End) {
            ++tagIndex;
        }
    }
    return prefixedTags;
}

} // namespace NVins
