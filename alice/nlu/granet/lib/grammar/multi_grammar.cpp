#include "multi_grammar.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/xrange.h>

namespace NGranet {

// static
TMultiGrammar::TRef TMultiGrammar::Create() {
    return new TMultiGrammar();
}

// static
TMultiGrammar::TRef TMultiGrammar::Create(const TGrammar::TConstRef& grammar) {
    TMultiGrammar::TRef self = Create();
    self->AddGrammar(grammar, GST_STATIC);
    return self;
}

// static
TMultiGrammar::TRef TMultiGrammar::CreateForBegemot(
    const TGrammar::TConstRef& staticGrammar,
    const TGrammar::TConstRef& freshGrammar,
    const TVector<TGrammar::TConstRef>& externalGrammars,
    const TFreshForcingOptions& freshOptions,
    const THashSet<TString>& experiments,
    const THashSet<TParserTaskKey>& enabledConditionalTasks)
{
    TMultiGrammar::TRef self = Create();

    if (freshOptions.ForceEntireFresh && freshGrammar != nullptr) {
        self->AddGrammar(freshGrammar, GST_FRESH);
    } else {
        self->AddGrammar(staticGrammar, GST_STATIC);
        self->AddFreshGrammar(freshGrammar, freshOptions);
    }
    for (const TGrammar::TConstRef& externalGrammar : externalGrammars) {
        self->AddGrammar(externalGrammar, GST_EXTERNAL);
    }
    self->AdjustByExperiments(experiments);
    self->RemoveConditionalTasks(enabledConditionalTasks);
    return self;
}

// Add tasks from grammar. New tasks override existed tasks with same names.
// grammarType - for logs.
void TMultiGrammar::AddGrammar(const TGrammar::TConstRef& grammar, EGrammarSourceType grammarType) {
    if (grammar == nullptr) {
        return;
    }
    const size_t grammarIndex = Grammars.size();
    Grammars.push_back({grammar, grammarType});

    for (const int t : xrange<int>(PTT_COUNT)) {
        for (const TParserTask& task : grammar->GetData().GetTasks(static_cast<EParserTaskType>(t))) {
            Tasks[task.GetTaskKey()] = {&task, grammarIndex};
        }
    }
}

void TMultiGrammar::AddFreshGrammar(const TGrammar::TConstRef& grammar, const TFreshForcingOptions& freshOptions) {
    if (grammar == nullptr) {
        return;
    }
    const size_t grammarIndex = Grammars.size();
    Grammars.push_back({grammar, GST_FRESH});

    // Erase tasks from static grammar which should be replaced by tasks from fresh.
    for (auto iter = Tasks.begin(), last = Tasks.end(); iter != last;) {
        if (ShouldUseFreshByOptions(iter->first, freshOptions)) {
            Tasks.erase(iter++);
        } else {
            ++iter;
        }
    }

    // Add tasks from fresh.
    for (const int t : xrange<int>(PTT_COUNT)) {
        for (const TParserTask& task : grammar->GetData().GetTasks(static_cast<EParserTaskType>(t))) {
            if (ShouldUseFreshByOptions(task.GetTaskKey(), freshOptions)
                || ShouldUseFreshByFreshnessParam(task))
            {
                Tasks[task.GetTaskKey()] = {&task, grammarIndex};
            }
        }
    }
}

// static
bool TMultiGrammar::ShouldUseFreshByOptions(const TParserTaskKey& key, const TFreshForcingOptions& options) {
    if (key.Type == PTT_FORM && IsIn(options.ForceForForms, key.Name)) {
        return true;
    }
    if (key.Type == PTT_ENTITY && IsIn(options.ForceForEntities, key.Name)) {
        return true;
    }
    for (const TString& prefix : options.ForceForPrefixes) {
        if (key.Name.StartsWith(prefix)) {
            return true;
        }
    }
    for (const TString& experiment : options.ForceForExperiments) {
        if (key.Name.EndsWith(experiment)
            && key.Name.EndsWith(EXPERIMENT_DELIMITER + experiment))
        {
            return true;
        }
    }
    return false;
}

bool TMultiGrammar::ShouldUseFreshByFreshnessParam(const TParserTask& freshTask) const {
    if (freshTask.Fresh) {
        return true;
    }
    if (freshTask.Freshness == 0) {
        return false;
    }
    const TTaskInfo* staticTask = Tasks.FindPtr(freshTask.GetTaskKey());
    return staticTask == nullptr || staticTask->Task->Freshness < freshTask.Freshness;
}

void TMultiGrammar::AdjustByExperiments(const THashSet<TString>& experiments) {
    TMap<TParserTaskKey, TTaskInfo> newTasks;
    for (const auto& [fullKey, info] : Tasks) {
        TStringBuf baseName;
        TStringBuf experiment;
        TStringBuf(fullKey.Name).Split(EXPERIMENT_DELIMITER, baseName, experiment);
        if (!experiment.empty() && !experiments.contains(experiment)) {
            // Task is experimental and experiment is not enabled. Remove task.
            continue;
        }
        TParserTaskKey baseKey = fullKey;
        baseKey.Name = TString(baseName);

        const auto& [it, isNew] = newTasks.try_emplace(baseKey, info);
        if (isNew) {
            // Accept
            continue;
        }
        if (it->second.Task->Name < fullKey.Name) {
            // Replace accepted
            it->second = info;
        }
    }
    Tasks = std::move(newTasks);
}

void TMultiGrammar::RemoveConditionalTasks(const THashSet<TParserTaskKey>& enabledConditionalTasks) {
    TVector<TParserTaskKey> toRemove;
    for (const auto& [key, info] : Tasks) {
        if (info.Task->IsConditional && !enabledConditionalTasks.contains(key)) {
            toRemove.push_back(key);
        }
    }
    for (const auto& key : toRemove) {
        Tasks.erase(key);
    }
}

const TMultiGrammar::TTaskInfo* TMultiGrammar::FindTask(const TParserTaskKey& key) const {
    return Tasks.FindPtr(key);
}

const TMultiGrammar::TTaskInfo& TMultiGrammar::GetTask(const TParserTaskKey& key) const {
    const TTaskInfo* info = Tasks.FindPtr(key);
    Y_ENSURE(info, "Error: " << key << " not found in grammar");
    return *info;
}

void TMultiGrammar::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TMultiGrammar:" << Endl;
    *log << indent << "  Grammars:" << Endl;
    for (const auto& [grammarIndex, grammar] : Enumerate(Grammars)) {
        *log << indent << "    Grammar " << grammarIndex << " (" << grammar.Type << "):" << Endl;
        for (const int t : xrange<int>(PTT_COUNT)) {
            for (const TParserTask& task : grammar.Grammar->GetData().GetTasks(static_cast<EParserTaskType>(t))) {
                *log << indent << "      " << task.GetTaskKey() << Endl;
            }
        }
    }
    *log << indent << "  Tasks:" << Endl;
    for (const auto& [key, task] : Tasks) {
        const TGrammarInfo& grammar = Grammars[task.GrammarIndex];
        *log << indent << "    " << key << ":" << Endl;
        *log << indent << "      From grammar " << task.GrammarIndex << " (" << grammar.Type << ")" << Endl;
        task.Task->Dump(*grammar.Grammar, log, indent + "      ");
    }
}

} // namespace NGranet
