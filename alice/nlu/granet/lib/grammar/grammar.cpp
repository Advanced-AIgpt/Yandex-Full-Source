#include "grammar.h"
#include <alice/nlu/granet/lib/utils/utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/cast.h>
#include <util/string/join.h>

namespace NGranet {

// ~~~~ TGrammar ~~~~

TGrammar::TGrammar(TGrammarData&& data)
    : Data(data)
{
    Update();
}

void TGrammar::Update() {
    for (TGrammarElement& element : Data.Elements) {
        Y_ENSURE(element.RuleTrieIndex != NPOS);
        element.RulesBeginIterator = TSearchIterator<TRuleTrie>(Data.RuleTriePool[element.RuleTrieIndex]);
    }

    EntityNameToElements.clear();
    for (const TGrammarElement& element : Data.Elements) {
        if (element.IsEntity()) {
            EntityNameToElements[element.EntityName].push_back(element.Id);
        }
    }

    UpdateMap(Data.Forms, &FormNameToIndex);
    UpdateMap(Data.Entities, &EntityNameToIndex);
}

// static
void TGrammar::UpdateMap(const TVector<TParserTask>& tasks, THashMap<TString, size_t>* map) {
    Y_ENSURE(map);
    map->clear();
    for (const auto& [i, task] : Enumerate(tasks)) {
        Y_ENSURE(!map->contains(task.Name));
        (*map)[task.Name] = i;
    }
}

//static
TGrammar::TRef TGrammar::LoadFromStream(IInputStream* input) {
    TGrammar::TRef result = Create();
    ::Load(input, *result);
    return result;
}

void TGrammar::Save(IOutputStream* output) const {
    SaveVersion(output, GRAMMAR_DATA_CURRENT_VERSION);
    ::Save(output, Data);
}

void TGrammar::Load(IInputStream* input) {
    LoadVersion(input, GRAMMAR_DATA_CURRENT_VERSION);
    ::Load(input, Data);
    Update();
}

TString TGrammar::GetElementNameForLog(TElementId id) const {
    return id == UNDEFINED_ELEMENT_ID ? TString() : Data.Elements[id].Name;
}

TVector<TString> TGrammar::GetElementsNamesForLog(const TVector<TElementId>& ids) const {
    TVector<TString> names(Reserve(ids.size()));
    for (TElementId id : ids) {
        names.push_back(GetElementNameForLog(id));
    }
    return names;
}

void TGrammar::Dump(IOutputStream* log, bool verbose, const TString& indent) const {
    Y_ENSURE(log);

    *log << indent << "TGrammar:" << Endl;
    *log << indent << "  Domain: " << Data.Domain << Endl;

    *log << indent << "  WordTrie:" << Endl;
    const TTokenPool tokenPool(Data.WordTrie);
    tokenPool.Dump(log, verbose, indent + "    ");

    *log << indent << "  Elements:" << Endl;
    for (const TGrammarElement& element : Data.Elements) {
        element.Dump(*this, tokenPool, log, verbose, indent + "    ");
    }
    *log << indent << "  OptimizationInfo:" << Endl;
    if (verbose) {
        Data.OptimizationInfo.Dump(tokenPool, log, indent + "    ");
    } else {
        *log << indent << "    ... in verbose log" << Endl;
    }
    *log << indent << "  Entities:" << Endl;
    for (const TParserTask& entity : Data.Entities) {
        entity.Dump(*this, log, indent + "    ");
    }
    *log << indent << "  Forms:" << Endl;
    for (const TParserTask& form : Data.Forms) {
        form.Dump(*this, log, indent + "    ");
    }
}

void TGrammar::PrintWords(IOutputStream* log, bool needExact, bool needLemma) const {
    Y_ENSURE(log);

    THashSet<TTokenId> ids;
    for (const TGrammarElement& element : Data.Elements) {
        if (element.RuleTrieIndex == NPOS) {
            continue;
        }
        for (const auto& [rule, data] : Data.RuleTriePool[element.RuleTrieIndex]) {
            for (const TTokenId id : rule) {
                if ((NTokenId::IsExactWord(id) && needExact)
                    || (NTokenId::IsLemmaWord(id) && needLemma))
                {
                    ids.insert(id);
                }
            }
        }
    }

    TSet<TString> words;
    const TTokenPool tokenPool(Data.WordTrie);
    for (const TTokenId id : ids) {
        words.insert(tokenPool.GetWord(id));
    }

    for (const TString& word : words) {
        *log << word << Endl;
    }
}

} // namespace NGranet
