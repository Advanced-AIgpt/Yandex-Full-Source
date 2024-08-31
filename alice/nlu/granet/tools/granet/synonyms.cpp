#include "synonyms.h"

#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <alice/nlu/libs/lemmatization/lemmatize.h>

#include <kernel/gazetteer/gazetteer.h>
#include <search/wizard/common/thesaurus/proto/thesaurus.pb.h>

#include <util/generic/hash.h>
#include <util/charset/wide.h>
#include <util/stream/output.h>

namespace NGranet {

namespace {

bool ShouldMatchSynonyms(const TGrammarElement& element, const TParserTask& parserTask) {
    ESynonymFlags enabledSynonyms = 0;
    enabledSynonyms |= (parserTask.EnableSynonymFlagsMask & parserTask.EnableSynonymFlags);
    enabledSynonyms &= ~(parserTask.EnableSynonymFlagsMask & ~parserTask.EnableSynonymFlags);
    enabledSynonyms |= (element.EnableSynonymFlagsMask & element.EnableSynonymFlags);
    enabledSynonyms &= ~(element.EnableSynonymFlagsMask & ~element.EnableSynonymFlags);

    return enabledSynonyms;
}

void GetLemmas(const TGrammarElement& element, const TGrammarData& data, const TTokenPool& tokenPool,
        TSet<TString>& currentLemmas, THashMap<TString, TString>& possibleLemmas)
{
    for (const auto& [rule, _] : data.RuleTriePool[element.RuleTrieIndex]) {
        for (const TTokenId id : rule) {
            if (NTokenId::IsLemmaWord(id)) {
                currentLemmas.insert(tokenPool.GetWord(id));
            }
        }
    }
    for (const auto& [rule, _] : data.RuleTriePool[element.RuleTrieIndex]) {
        for (const TTokenId id : rule) {
            if (NTokenId::IsExactWord(id)) {
                const auto& word = tokenPool.GetWord(id);
                if (possibleLemmas.contains(word)) {
                    continue;
                }
                const auto lemma = NNlu::LemmatizeWordBest(word, ELanguage::LANG_RUS);
                if (currentLemmas.contains(lemma)) {
                    continue;
                }
                possibleLemmas[word] = lemma;
            }
        }
    }
}

void PrintSynonymsForLemma(const TString& lemma, const THashMap<TString, TSet<TString>>& synonyms, IOutputStream* log) {
    *log << lemma << ";";
    if (const auto lemmaSynonyms = synonyms.FindPtr(lemma)) {
        for (const auto& w : *lemmaSynonyms) {
            *log << w << ";";
        }
    }
    *log << Endl;
}

void PrintSynonymsForElement(const TGrammarElement& element, const TGrammarData& data, const TTokenPool& tokenPool,
        const THashMap<TString, TSet<TString>>& synonyms, IOutputStream* log)
{
    *log << "ELEMENT: " << element.Name << Endl;
    TSet<TString> currentLemmas;
    THashMap<TString, TString> possibleLemmas;
    GetLemmas(element, data, tokenPool, currentLemmas, possibleLemmas);
    *log << "CURRENT LEMMAS: " << currentLemmas.size() << Endl;
    for (const auto& lemma : currentLemmas) {
        PrintSynonymsForLemma(lemma, synonyms, log);
    }
    *log << "POSSIBLE LEMMAS: " << possibleLemmas.size() << Endl;
    TVector<const TString*> orderedKeys;
    orderedKeys.reserve(possibleLemmas.size());
    for (const auto& [k, v] : possibleLemmas) {
        orderedKeys.push_back(&k);
    }
    SortBy(orderedKeys, [](const auto& key) { return key; });
    for (const auto* word : orderedKeys) {
        const auto lemma = possibleLemmas[*word];
        *log << *word << " -> ";
        PrintSynonymsForLemma(lemma, synonyms, log);
    }
    *log << Endl;
}

} // namespace anonymus

THashMap<TString, TSet<TString>> LoadSynonyms(const TString& pathMain, const TString& pathFixlist) {
    THashMap<TString, TSet<TString>> result;
    TGazetteer gztMain(pathMain);
    TGazetteer gztFixlist(pathFixlist);
    for (const auto& [word, _] : gztMain.GztTrie().GetWordTrie()) {
        TArticleIter<TVector<TUtf16String>> iter;
        TVector<TUtf16String> query{word};
        TString from = WideToUTF8(word);
        THashSet<TString> tos;
        for (gztMain.IterArticles(query, &iter); iter.Ok(); ++iter)  {
            const auto* article = gztMain.GetArticle(iter).As<TRawExt>();
            TString to{TStringBuf{article->GetTo()}};
            tos.insert(to);
        }
        for (gztFixlist.IterArticles(query, &iter); iter.Ok(); ++iter)  {
            const auto* article = gztFixlist.GetArticle(iter).As<TRawExt>();
            TString to{TStringBuf{article->GetTo()}};
            if (article->GetType() == NWiz::EExtType::etRemove) {
                tos.erase(to);
            } else {
                tos.insert(to);
            }
        }
        for (const auto& to : tos) {
            result[to].insert(from);
        }

    }

    return result;
}

void PrintFormSynonyms(const TGrammar& grammar, const TString& formName, const THashMap<TString, TSet<TString>>& synonyms, IOutputStream* log) {
    Y_ENSURE(log);

    const TGrammarData& data = grammar.GetData();
    const TParserTask* parserTask;
    for (const TParserTask& form : data.Forms) {
        if (form.Name == formName) {
            parserTask = &form;
        }
    }
    if (!parserTask) {
        Cerr << "From " << formName << " not found" << Endl;
        return;
    }

    THashSet<TElementId> elementsInForm;
    TVector<TElementId> consideredElements;
    consideredElements.push_back(parserTask->Root);
    while(!consideredElements.empty()) {
        TElementId curElem = consideredElements[consideredElements.size() - 1];
        consideredElements.pop_back();
        elementsInForm.insert(curElem);
        for (const TElementId elem : data.Elements[curElem].ElementsInRules) {
            if (!elementsInForm.contains(elem)) {
                consideredElements.push_back(elem);
            }
        }
    }
    TVector<TElementId> sortedElements(elementsInForm.begin(), elementsInForm.end());
    SortBy(sortedElements, [&data](TElementId id) { return data.Elements[id].Name; });

    const TTokenPool tokenPool(data.WordTrie);
    TVector<TElementId> elementsDisabledSynonyms;
    *log << "===SYNONYMS ENABLED FOR THESE ELEMENTS===" << Endl;
    for (const TElementId elem : sortedElements) {
        const auto& element = data.Elements[elem];
        if (element.RuleTrieIndex == NPOS) {
            continue;
        }
        if (!ShouldMatchSynonyms(element, *parserTask)) {
            elementsDisabledSynonyms.push_back(elem);
            continue;
        }
        PrintSynonymsForElement(element, data, tokenPool, synonyms, log);
    }

    *log << Endl;
    *log << "===SYNONYMS DISABLED FOR THESE ELEMENTS===" << Endl;
    for (const TElementId elem : elementsDisabledSynonyms) {
        const auto& element = data.Elements[elem];
        PrintSynonymsForElement(element, data, tokenPool, synonyms, log);
    }
}

} // namespace NGranet
