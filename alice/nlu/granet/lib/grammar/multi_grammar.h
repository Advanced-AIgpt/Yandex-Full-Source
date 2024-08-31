#pragma once

#include <alice/nlu/granet/lib/grammar/grammar.h>
#include <util/generic/hash.h>

namespace NGranet {

inline const TStringBuf EXPERIMENT_DELIMITER = ".ifexp.";

enum EGrammarSourceType {
    GST_STATIC      /* "static" */,
    GST_FRESH       /* "fresh" */,
    GST_EXTERNAL    /* "external" */,
};

struct TFreshForcingOptions {
    bool ForceEntireFresh = false;
    TVector<TString> ForceForForms;
    TVector<TString> ForceForEntities;
    TVector<TString> ForceForPrefixes;
    TVector<TString> ForceForExperiments;
};

class TMultiGrammar : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TMultiGrammar>;
    using TConstRef = TIntrusiveConstPtr<TMultiGrammar>;

    struct TTaskInfo {
        const TParserTask* Task = nullptr;
        size_t GrammarIndex = 0;
    };

    struct TGrammarInfo {
        TGrammar::TConstRef Grammar;
        EGrammarSourceType Type = GST_STATIC;
    };

public:
    static TRef Create();
    static TRef Create(const TGrammar::TConstRef& grammar);

    // Parameters staticGrammar and freshGrammar can be null.
    // If freshGrammar is null then parameter freshOptions will be ignored.
    static TRef CreateForBegemot(
        const TGrammar::TConstRef& staticGrammar,
        const TGrammar::TConstRef& freshGrammar,
        const TVector<TGrammar::TConstRef>& externalGrammars,
        const TFreshForcingOptions& freshOptions,
        const THashSet<TString>& experiments,
        const THashSet<TParserTaskKey>& enabledConditionalTasks);

    const TVector<TGrammarInfo>& GetGrammars() const {
        return Grammars;
    }

    const TMap<TParserTaskKey, TTaskInfo>& GetTasks() const {
        return Tasks;
    }

    const TTaskInfo* FindTask(const TParserTaskKey& key) const;
    const TTaskInfo& GetTask(const TParserTaskKey& key) const;

    void Dump(IOutputStream* log, const TString& indent = "") const;

private:
    void AddGrammar(const TGrammar::TConstRef& grammar, EGrammarSourceType grammarType);
    void AddFreshGrammar(const TGrammar::TConstRef& grammar, const TFreshForcingOptions& freshOptions);
    static bool ShouldUseFreshByOptions(const TParserTaskKey& key, const TFreshForcingOptions& options);
    bool ShouldUseFreshByFreshnessParam(const TParserTask& freshTask) const;
    void RemoveConditionalTasks(const THashSet<TParserTaskKey>& enabledConditionalTasks);
    void AdjustByExperiments(const THashSet<TString>& experiments);

private:
    TVector<TGrammarInfo> Grammars;
    TMap<TParserTaskKey, TTaskInfo> Tasks;
};

} // namespace NGranet
