#include "session_conversion.h"
#include <kernel/lemmer/core/language.h>
#include <kernel/lemmer/core/lemmer.h>
#include <kernel/lemmer/dictlib/grammar_index.h>
#include <library/cpp/json/json_reader.h>
#include <util/charset/wide.h>
#include <util/generic/maybe.h>
#include <util/string/join.h>
#include <util/string/split.h>

namespace NBg {
    namespace {
        const TVector<TString> PRONOUN_MARKS = {"SPRO", "APRO", "ADVPRO"};
        const TVector<TString> REGISTERED_PRONOUNS = {"он", "она", "оно", "они", "тот", "туда",
                                                      "оттуда", "там", "здесь", "сюда", "отсюда"};

        TMaybe<TString> GetFirstGrammeme(const char* commaSeparatedGrammemes) {
            TVector<TString> grammemes = StringSplitter(sprint_grammar(commaSeparatedGrammemes)).Split(',').Limit(2);
            if (grammemes.empty() || grammemes[0] == "") {
                return Nothing();
            }
            return grammemes[0];
        }

        void AnalyzeGrammar(const TVector<TString>& tokens, TVector<TString>* lemmas, TVector<TString>* grammemes) {
            Y_ASSERT(lemmas);
            Y_ASSERT(grammemes);

            lemmas->reserve(tokens.size());
            grammemes->reserve(tokens.size());

            const NLemmer::TAnalyzeWordOpt lemmerOptions;

            for (const auto& token : tokens) {
                TWLemmaArray lemmaArray;
                const TUtf16String tokenUtf16 = UTF8ToWide(token);
                NLemmer::AnalyzeWord(tokenUtf16.data(), tokenUtf16.size(), lemmaArray, NLanguageMasks::BasicLanguages(), nullptr);
                bool foundLemma = false;
                for (const auto& lemma : lemmaArray) {
                    const auto partOfSpeech = GetFirstGrammeme(lemma.GetStemGram());
                    if (partOfSpeech.Empty()) {
                        continue;
                    }

                    TString lemmaGrammemes = partOfSpeech.GetRef();

                    for (size_t flexGramIdx = 0; flexGramIdx < lemma.FlexGramNum(); ++flexGramIdx) {
                        if (const auto mainFlexGram = GetFirstGrammeme(lemma.GetFlexGram()[flexGramIdx])) {
                            lemmaGrammemes += " " + mainFlexGram.GetRef();
                            break;
                        }
                    }

                    foundLemma = true;
                    lemmas->push_back(WideToUTF8(lemma.GetText()));
                    grammemes->push_back(lemmaGrammemes);
                    break;
                }
                if (!foundLemma) {
                    lemmas->push_back(token);
                    grammemes->emplace_back();
                }
            }
        }

        bool IsPronoun(const TString& lemma, const TString& grammemes) {
            if (!IsIn(REGISTERED_PRONOUNS, lemma)) {
                return false;
            }

            TStringBuf grammemeMark = TStringBuf{grammemes}.Before(' ');
            if (grammemeMark.empty()) {
                return false;
            }

            return IsIn(PRONOUN_MARKS, grammemeMark);
        }
    } // anonymous

    void GenerateAllSegmentsAsEntities(const NVins::TSample& phrase,
                                       const size_t phraseIdx,
                                       const size_t maxSegmentLength,
                                       TVector<NAlice::TMentionInDialogue>* entities) {
        Y_ASSERT(entities);

        const auto& numTokens = phrase.Tokens.size();
        for (size_t start = 0; start < numTokens; ++start) {
            for (size_t end = start + 1; end <= start + maxSegmentLength && end <= numTokens; ++end) {
                entities->push_back(NAlice::TMentionInDialogue(/*phrasePos*/ phraseIdx,
                                                               start,
                                                               end,
                                                               NAlice::TMentionInPhrase::EPhraseType::Other));
            }
        }
    }

    NVins::TSample ConvertProtoPhraseToSample(const NProto::TAlicePhrase& phraseProto) {
        NVins::TSample sample;
        const auto& tokensProto = phraseProto.GetTokens();
        sample.Tokens.reserve(tokensProto.size());
        for (const auto& token : tokensProto) {
            sample.Tokens.push_back(token);
        }
        sample.Text = JoinSeq(" ", sample.Tokens);
        return sample;
    }

    void ParsePronounMentions(const NProto::TAlicePhrase& requestProto,
                              const size_t phraseIdx,
                              TVector<NAlice::TMentionInDialogue>* mentions,
                              TVector<TString>* pronounGrammemes) {
        Y_ASSERT(mentions);
        Y_ASSERT(pronounGrammemes);

        const auto& tokenLemmas = requestProto.GetLemmas();
        const auto& tokenGrammemes = requestProto.GetGrammemes();

        Y_ENSURE(tokenLemmas.size() == tokenGrammemes.size());
        Y_ENSURE(tokenLemmas.size() == requestProto.GetTokens().size());

        for (size_t tokenPos = 0; tokenPos < static_cast<size_t>(tokenGrammemes.size()); ++tokenPos) {
            const auto& grammemes = tokenGrammemes[tokenPos];
            if (!IsPronoun(tokenLemmas[tokenPos], grammemes)) {
                continue;
            }

            mentions->push_back(NAlice::TMentionInDialogue(/*phrasePos*/ phraseIdx,
                                                           tokenPos,
                                                           tokenPos + 1,
                                                           NAlice::TMentionInPhrase::EPhraseType::Pronoun));
            pronounGrammemes->push_back(grammemes);
        }
    }

    void AddGrammarAnalysisToPhrase(NProto::TAlicePhrase* phrase) {
        Y_ASSERT(phrase);

        TVector<TString> tokens;
        tokens.reserve(phrase->GetTokens().size());
        for (const auto& token : phrase->GetTokens()) {
            tokens.push_back(token);
        }

        TVector<TString> lemmas;
        TVector<TString> grammemes;
        AnalyzeGrammar(tokens, &lemmas, &grammemes);

        for (const auto& lemma : lemmas) {
            phrase->AddLemmas(lemma);
        }
        for (const auto& grammeme : grammemes) {
            phrase->AddGrammemes(grammeme);
        }
    }

} // namespace NAlice
