#pragma once

#include "grammar_data.h"
#include <alice/nlu/granet/lib/utils/serialization.h>
#include <library/cpp/langs/langs.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {

// ~~~~ TGrammar ~~~~

// Granet compiled grammar (model).
// Main related classes:
//   - NCompiler::TCompiler - grammar compiler (reads config and data files, creates TGrammar).
//   - TGrammar - grammar data (complete description of grammar).
//   - TParser - grammar applier (parse sample using TGrammar).
class TGrammar : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TGrammar>;
    using TConstRef = TIntrusiveConstPtr<TGrammar>;

public:
    static TRef Create(TGrammarData&& data) {
        return new TGrammar(std::move(data));
    }

    static TRef Create() {
        return new TGrammar();
    }

    static TRef LoadFromStream(IInputStream* input);

    const TGrammarData& GetData() const {
        return Data;
    }

    const TGranetDomain& GetDomain() const {
        return Data.Domain;
    }

    ELanguage GetLanguage() const {
        return Data.Domain.Lang;
    }

    const THashMap<TString, size_t>& GetEntityNameToIndex() const {
        return EntityNameToIndex;
    }

    const THashMap<TString, size_t>& GetFormNameToIndex() const {
        return FormNameToIndex;
    }

    const THashMap<TString, TVector<TElementId>>& GetEntityNameToElements() const {
        return EntityNameToElements;
    }

    TString GetElementNameForLog(TElementId id) const;
    TVector<TString> GetElementsNamesForLog(const TVector<TElementId>& ids) const;

    void Dump(IOutputStream* log, bool verbose = true, const TString& indent = "") const;
    void PrintWords(IOutputStream* log, bool needExact, bool needLemma) const;

    void Save(IOutputStream* output) const;
    void Load(IInputStream* input);

private:
    TGrammar() = default;
    explicit TGrammar(TGrammarData&& data);

    void Update();
    static void UpdateMap(const TVector<TParserTask>& tasks, THashMap<TString, size_t>* map);

private:
    TGrammarData Data;
    THashMap<TString, size_t> EntityNameToIndex;
    THashMap<TString, size_t> FormNameToIndex;
    THashMap<TString, TVector<TElementId>> EntityNameToElements;
};

} // namespace NGranet
