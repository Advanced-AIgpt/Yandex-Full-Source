#pragma once

#include "data.h"

#include <util/generic/algorithm.h>
#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>

namespace NGenerativeBoltalka {

    class IGenerativeFilter: public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<IGenerativeFilter>;

        struct TParams {
            bool FilterEmpty = true;

            bool FilterDuplicateWords = true;

            bool FilterBadWords = true;
            bool BadWordsAreRegexps = false;
            TString BadWordsDictPath;
            bool BadWordsUseLemmatizer = true;

            bool FilterDuplicateNGrams = false;
            size_t NGramSize = 3;

            bool FilterByUniqueWordsRatio = false;
            float MinUniqueRatio = 0.65;
        };

    public:
        virtual ~IGenerativeFilter() = default;

        void FilterResponses(TVector<TGenerativeResponse>& responses) const {
            EraseIf(responses, [this](const auto& item) { return ShouldFilterResponse(item); });
        }

        virtual bool ShouldFilterResponse(const TGenerativeResponse& response) const = 0;
    };

    IGenerativeFilter::TPtr CreateFilter(const IGenerativeFilter::TParams& params);

    class TPostProcessor: public TThrRefBase {
    public:
        using TPtr = TIntrusivePtr<TPostProcessor>;

        struct TParams {
            // adding "-" to some cases in outputs because this symbol was removed from training data
            bool AddHyphens = true;
            bool FixPunctuation = true;
            bool Capitalize = true;

            // mapping from regex to regex for postprocessing after generation
            THashMap<TString, TString> PostProcessMapping;
        };

        TPostProcessor(const TParams& params);

        TString HyphenAddition(TString& string) const;
        TString PostProcessText(TString& string) const;

    private:
        bool ShouldInsertHyphen(TString& word1, TString& word2) const;

    private:
        TPostProcessor::TParams Params;
        THashSet<TString> HyphenPostfixes;
        THashSet<TString> HyphenPrefixes;
    };

} // namespace NGenerativeBoltalka

