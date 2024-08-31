#include "postprocessing.h"

#include "util.h"

#include <kernel/lemmer/core/language.h>

#include <contrib/libs/re2/re2/re2.h>

#include <library/cpp/langs/langs.h>

#include <util/charset/utf8.h>
#include <util/charset/wide.h>
#include <util/stream/file.h>
#include <util/string/builder.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NGenerativeBoltalka {
    namespace {

        TString GetWordLemma(TStringBuf word) {
            const auto wideWord = UTF8ToWide(word);
            TWLemmaArray lemmas;
            const auto lemmasCount = NLemmer::AnalyzeWord(wideWord.c_str(), wideWord.length(), lemmas, TLangMask(LANG_RUS));
            if (lemmasCount == 0) {
                return "";
            }

            return WideToUTF8(lemmas.front().GetText(), lemmas.front().GetTextLength());
        }

        TVector<THolder<RE2>> ReadPatternsFromFile(TString path) {
            auto patternStrings = LoadFileToHashSet<TString>(path);

            TVector<THolder<RE2>> result;
            result.reserve(patternStrings.size());
            for (auto& patternString : patternStrings) {
                Cerr << "BANNING pattern: " << patternString << Endl;
                result.emplace_back(MakeHolder<RE2>(patternString));
            }
            return result;
        }

        class TAggregateFilter : public IGenerativeFilter {
        public:
            explicit TAggregateFilter(const TVector<TPtr>& filters)
                : Filters(filters)
            {
            }

        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                return std::any_of(Filters.begin(), Filters.end(), [&response] (const auto& f) { return f->ShouldFilterResponse(response); });
            }

        private:
            const TVector<TPtr> Filters;
        };

        class TEmptyFilter : public IGenerativeFilter {
        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                return response.ResponseWords.empty();
            }
        };

        class TDuplicateWordsFilter : public IGenerativeFilter {
        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                THashSet<TStringBuf> foundWords;

                for (const auto& word : response.ResponseWords) {
                    if (foundWords.contains(word)) {
                        return true;
                    }
                    foundWords.insert(word);
                }

                return false;
            }
        };

        class TBadVocabularyRegexpFilter : public IGenerativeFilter {
        public:
            TBadVocabularyRegexpFilter(TString vocPath, bool shouldUseLemmatizer)
                : Patterns(ReadPatternsFromFile(vocPath))
                , ShouldUseLemmatizer(shouldUseLemmatizer)
            {
            }

        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                for (const auto& word : response.ResponseWords) {
                    const auto& checkWord = ShouldUseLemmatizer ? GetWordLemma(word) : word;

                    if (std::any_of(Patterns.begin(),
                                    Patterns.end(),
                                    [&checkWord] (auto&& patternHolder) {
                                        return RE2::PartialMatch(checkWord, *patternHolder);
                                    })) {
                        return true;
                    }
                }
                return false;
            }

        private:
            const TVector<THolder<RE2>> Patterns;
            const bool ShouldUseLemmatizer;
        };

        class TBadVocabularyFilter : public IGenerativeFilter {
        public:
            TBadVocabularyFilter(TString vocPath, bool shouldUseLemmatizer)
                    : BadWords(LoadFileToHashSet<TString>(vocPath))
                    , ShouldUseLemmatizer(shouldUseLemmatizer)
            {
            }

        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                for (const auto& word : response.ResponseWords) {
                    const auto& checkWord = ShouldUseLemmatizer ? GetWordLemma(word) : word;
                    if (BadWords.contains(checkWord)) {
                        return true;
                    }
                }
                return false;
            }

        private:
            const THashSet<TString> BadWords;
            const bool ShouldUseLemmatizer;
        };

        class TNGramDuplicatesWordsFilter : public IGenerativeFilter {
        public:
            explicit TNGramDuplicatesWordsFilter(size_t nGramSize)
                    : NGramSize(nGramSize)
            {
            }

        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                if (response.ResponseWords.size() < NGramSize) {
                    return false;
                }
                THashSet<TString> foundNGrams;

                for (size_t i = 0; i < response.ResponseWords.size() - NGramSize + 1; i++) {
                    TStringBuilder nGram;
                    for (size_t j = i; j < i + NGramSize; j++) {
                        nGram << response.ResponseWords[j];
                    }
                    if (foundNGrams.contains(nGram)) {
                        return true;
                    }
                    foundNGrams.insert(nGram);
                }

                return false;
            }

        private:
            const size_t NGramSize;
        };

        class TUniqueWordsRatioFilter : public IGenerativeFilter {
        public:
            explicit TUniqueWordsRatioFilter(float minUniqueRatio)
                    : MinUniqueRatio(minUniqueRatio)
            {
            }

        public:
            bool ShouldFilterResponse(const TGenerativeResponse& response) const override {
                if (response.ResponseWords.empty()) {
                    return false;
                }

                THashSet<TStringBuf> foundWords;

                for (const auto& word : response.ResponseWords) {
                    if (foundWords.contains(word)) {
                        continue;
                    }
                    foundWords.insert(word);
                }

                return MinUniqueRatio > (float(foundWords.size()) / response.ResponseWords.size());
            }

        private:
            float MinUniqueRatio;
        };

        TString FixPunctuation(TString text) {
            static const RE2 fixPunctuationRegEx1{" ([!%,.:;?\\)\\]\\}])"};
            static const RE2 fixPunctuationRegEx2{"([\\(\\[\\{]) "};
            static const RE2 fixPunctuationRegEx3{"\"\\s*([^\"]*\\S)\\s*\""};
            static const RE2 fixPunctuationRegEx4{"\'\\s*([^\"]*\\S)\\s*\'"};
            static const RE2 fixPunctuationRegEx5{"((?: |^|[\"\'\\(\\[\\{]])(?:в|во|по|кое|пол))\\s*-\\s*"};
            static const RE2 fixPunctuationRegEx6{"\\s*-\\s*((?:то|либо|нибудь|ка|таки)(?: |$|[\"\'!%,.:;?\\)\\]\\}]))"};
            static const RE2 fixPunctuationRegEx7{" - "};
            RE2::GlobalReplace(&text, fixPunctuationRegEx1, "\\1");
            RE2::GlobalReplace(&text, fixPunctuationRegEx2, "\\1");
            RE2::GlobalReplace(&text, fixPunctuationRegEx3, "\"\\1\"");
            RE2::GlobalReplace(&text, fixPunctuationRegEx4, "\'\\1\'");
            RE2::GlobalReplace(&text, fixPunctuationRegEx5, "\\1-");
            RE2::GlobalReplace(&text, fixPunctuationRegEx6, "-\\1");
            RE2::GlobalReplace(&text, fixPunctuationRegEx7, " – ");
            return text;
        }

        TString Capitalize(const TString& text) {
            auto wideText = UTF8ToWide(text);
            bool isNewSentance = true;
            THashSet<wchar16> delimeters{'.', '?', '!'};
            for (auto& ch : wideText) {
                if (isNewSentance && ch != ' ') {
                    ch = ToTitle(ch);
                    isNewSentance = false;
                }
                if (delimeters.contains(ch)) {
                    isNewSentance = true;
                }
            }
            return WideToUTF8(wideText);
        }

    } // namespace anonymous

    IGenerativeFilter::TPtr CreateFilter(const IGenerativeFilter::TParams& params) {
        TVector<IGenerativeFilter::TPtr> filters;
        if (params.FilterEmpty) {
            filters.emplace_back(new TEmptyFilter());
        }
        if (params.FilterBadWords) {
            IGenerativeFilter* filter;
            if (params.BadWordsAreRegexps) {
                filter = new TBadVocabularyRegexpFilter(params.BadWordsDictPath, params.BadWordsUseLemmatizer);
            } else {
                filter = new TBadVocabularyFilter(params.BadWordsDictPath, params.BadWordsUseLemmatizer);
            }
            filters.emplace_back(filter);
        }
        if (params.FilterDuplicateWords) {
            filters.emplace_back(new TDuplicateWordsFilter());
        }
        if (params.FilterDuplicateNGrams) {
            filters.emplace_back(new TNGramDuplicatesWordsFilter(params.NGramSize));
        }
        if (params.FilterByUniqueWordsRatio) {
            filters.emplace_back(new TUniqueWordsRatioFilter(params.MinUniqueRatio));
        }

        return new TAggregateFilter(filters);
    }

    TPostProcessor::TPostProcessor(const TPostProcessor::TParams& params)
        : Params(params)
    {
        for (auto& word : {"кто", "что", "чем" /* an exception for что, since lemmatizer converts it to чем */,
                           "где", "когда", "куда", "откуда",
                           "почему", "зачем", "как", "сколько", "какой", "чей"}) {
            HyphenPrefixes.insert(word);
        }
        for (auto& word : {"то", "нибудь", "либо"}) {
            HyphenPostfixes.insert(word);
        }
    }

    bool TPostProcessor::ShouldInsertHyphen(TString& word1, TString& word2) const {
        // First rule: "кое-кому", "кое-где", ...
        // Second rule: "почему-то", "кого-нибудь", "где-либо", ...
        return word1 == "кое" && HyphenPrefixes.contains(GetWordLemma(word2)) ||
               HyphenPrefixes.contains(GetWordLemma(word1)) && HyphenPostfixes.contains(word2);
    }

    TString TPostProcessor::HyphenAddition(TString& str) const {
        if (str.empty()) {
            return str;
        }

        TVector<TString> splitResult;
        Split(str, " ", splitResult);

        TStringBuilder processedString;
        processedString << splitResult.front();
        for (size_t i = 1; i < splitResult.size(); i++) {
            auto prevWord = splitResult[i - 1];
            auto curWord = splitResult[i];

            if (ShouldInsertHyphen(prevWord, curWord)) {
                processedString << "-";
            } else {
                processedString << " ";
            }
            processedString << curWord;
        }
        return processedString;
    }

    TString TPostProcessor::PostProcessText(TString& str) const {
        auto result = str;

        if (Params.AddHyphens) {
            result = HyphenAddition(str);
        }
        if (Params.FixPunctuation) {
            result = FixPunctuation(result);
        }
        if (Params.Capitalize) {
            result = Capitalize(result);
        }

        if (Params.PostProcessMapping.size() > 0) {
            for (auto& [fromRegex, toRegex] : Params.PostProcessMapping) {
                RE2::GlobalReplace(&result, fromRegex, toRegex);
            }
        }

        return result;
    }

} // namespace NGenerativeBoltalka
