#include "word_logprob_table.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>
#include <alice/nlu/libs/normalization/normalize.h>
#include <dict/nerutil/tstimer.h>
#include <library/cpp/resource/resource.h>
#include <util/generic/hash.h>
#include <util/string/split.h>

namespace NGranet::NWordLogProbTable {

    namespace {

        const float UNKNOWN_EXACT_WORD_LOG_PROB = -13.f;
        const float UNKNOWN_LEMMA_WORD_LOG_PROB = -11.f;

        class TWordLogProbTable {
        public:
            TWordLogProbTable() {
                LoadTable(LANG_RUS, "/granet/lang/word_logprob_ru.txt");
            }

            bool IsLanguageSupported(ELanguage lang) const {
                return Tables.contains(std::make_pair(false, lang));
            }

            float GetWordLogProb(bool isLemma, TStringBuf word, ELanguage lang) const {
                DEBUG_TIMER("TWordLogProbTable::GetWordLogProb");
                const float defaultLogProb = isLemma ? UNKNOWN_LEMMA_WORD_LOG_PROB : UNKNOWN_EXACT_WORD_LOG_PROB;
                const THashMap<TString, float>* table = Tables.FindPtr(std::make_pair(isLemma, lang));
                return table ? table->Value(word, defaultLogProb) : defaultLogProb;
            }

        private:
            void LoadTable(ELanguage lang, TStringBuf tsvPath) {
                DEBUG_TIMER("TWordLogProbTable::LoadTable");
                THashMap<TString, float>& lemmaTable = Tables[std::make_pair(true, lang)];
                THashMap<TString, float>& exactTable = Tables[std::make_pair(false, lang)];
                const TString tsvText = NResource::Find(tsvPath);
                size_t lineIndex = 0;
                for (const TStringBuf line : StringSplitter(tsvText).Split('\n').SkipEmpty()) {
                    float logprob = 0;
                    TString word;
                    StringSplitter(line).Split('\t').CollectInto(&logprob, &word);
                    AddWord(NNlu::NormalizeWord(word, lang), logprob, &exactTable);
                    if (lineIndex < 1000) {
                        // Optimization: lemmatize top 1000 words only
                        for (const TString& lemma : NNlu::LemmatizeWord(word, lang, NNlu::ANY_LEMMA_THRESHOLD)) {
                            AddWord(lemma, logprob, &lemmaTable);
                        }
                    }
                    lineIndex++;
                }
            }

            static void AddWord(const TString& word, float logprob, THashMap<TString, float>* table) {
                Y_ASSERT(table);
                Y_ASSERT(logprob < 0);
                const auto& [it, isNew] = (*table).try_emplace(word, logprob);
                if (!isNew) {
                    it->second = LogSumExp(it->second, logprob);
                }
            }

        private:
            THashMap<std::pair<bool, ELanguage>, THashMap<TString, float>> Tables;
        };

    } // namespace

    float IsLanguageSupported(ELanguage lang) {
        return Singleton<TWordLogProbTable>()->IsLanguageSupported(lang);
    }

    float GetWordLogProb(bool isLemma, TStringBuf word, ELanguage lang) {
        return Singleton<TWordLogProbTable>()->GetWordLogProb(isLemma, word, lang);
    }

} // namespace NGranet::NWordLogProbTable
