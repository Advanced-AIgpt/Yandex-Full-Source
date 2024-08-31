#include "multi_parser.h"
#include "parser.h"
#include <alice/nlu/granet/lib/utils/string_utils.h>
#include <alice/nlu/granet/lib/utils/trace.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/generic/xrange.h>

namespace NGranet {

// static
TMultiParser::TRef TMultiParser::Create(const TMultiPreprocessedSample::TRef& preprocessedSample,
    bool ensureEntities)
{
    return new TMultiParser(preprocessedSample, ensureEntities);
}

// static
TMultiParser::TRef TMultiParser::Create(const TMultiGrammar::TConstRef& multiGrammar,
    const TSample::TRef& sample, bool ensureEntities)
{
    return new TMultiParser(TMultiPreprocessedSample::Create(multiGrammar, sample), ensureEntities);
}

// static
TMultiParser::TRef TMultiParser::Create(const TGrammar::TConstRef& grammar,
    const TSample::TRef& sample, bool ensureEntities)
{
    return new TMultiParser(TMultiPreprocessedSample::Create(grammar, sample), ensureEntities);
}

TMultiParser::TMultiParser(const TMultiPreprocessedSample::TRef& preprocessedSample,
        bool ensureEntities)
    : Sample(preprocessedSample)
    , ShouldEnsureEntities(ensureEntities)
{
}

TMultiParser& TMultiParser::SetLog(IOutputStream* log, bool isVerbose) {
    Log = log;
    IsLogVerbose = isVerbose;
    return *this;
}

TMultiParser& TMultiParser::SetNeedDebugInfo(bool value) {
    NeedDebugInfo = value;
    return *this;
}

TMultiParser& TMultiParser::SetCollectBlockersMode(bool value) {
    CollectBlockersMode = value;
    return *this;
}

const TMultiGrammar::TConstRef& TMultiParser::GetMultiGrammar() {
    return Sample->GetMultiGrammar();
}

TParserTaskResult::TRef TMultiParser::ParseTask(EParserTaskType type, TStringBuf name) {
    if (type == PTT_FORM) {
        return ParseForm(name);
    } else if (type == PTT_ENTITY) {
        return ParseEntity(name);
    } else {
        Y_ENSURE(false);
        return nullptr;
    }
}

TVector<TParserEntityResult::TConstRef> TMultiParser::ParseEntities() {
    TVector<TParserEntityResult::TConstRef> result;
    for (const auto& [key, task] : GetMultiGrammar()->GetTasks()) {
        if (key.Type == PTT_ENTITY) {
            result.push_back(ParseEntity(key.Name, task));
        }
    }
    return result;
}

TParserEntityResult::TRef TMultiParser::ParseEntity(TStringBuf name) {
    const TMultiGrammar::TTaskInfo& task = GetMultiGrammar()->GetTask({PTT_ENTITY, TString(name)});
    return ParseEntity(name, task);
}

TParserEntityResult::TRef TMultiParser::ParseEntity(TStringBuf name, const TMultiGrammar::TTaskInfo& task) {
    const bool isNewEntity = Sample->SetEntityWasParsed(name);

    TParserEntityResult::TRef result = ParseTask(name, task)->AsEntityResult();

    if (isNewEntity) {
        Sample->AddFoundEntitiesToSample(result->GetEntities());
    }
    return result;
}

TVector<TParserFormResult::TConstRef> TMultiParser::ParseForms() {
    TVector<TParserFormResult::TConstRef> result;
    for (const auto& [key, task] : GetMultiGrammar()->GetTasks()) {
        if (key.Type == PTT_FORM) {
            result.push_back(ParseTask(key.Name, task)->AsFormResult());
        }
    }
    return result;
}

TParserFormResult::TRef TMultiParser::ParseForm(TStringBuf name) {
    const TMultiGrammar::TTaskInfo& task = GetMultiGrammar()->GetTask({PTT_FORM, TString(name)});
    return ParseTask(name, task)->AsFormResult();
}

TParserTaskResult::TRef TMultiParser::ParseTask(TStringBuf name, const TMultiGrammar::TTaskInfo& task) {
    ParseDependencies(task);
    TRACE_LINE(Log, "Parse " << task.Task->GetTaskKey() << " from grammar " << task.GrammarIndex);
    TPreprocessedSample::TConstRef preprocessedSample = Sample->GetPreprocessedSample(task.GrammarIndex);
    TParser parser(preprocessedSample, *task.Task);
    parser.SetLog(Log, IsLogVerbose);
    parser.SetNeedDebugInfo(NeedDebugInfo);
    parser.SetCollectBlockersMode(CollectBlockersMode);
    TParserTaskResult::TRef result = parser.Parse();
    Y_ENSURE(result);
    // Replace full name (perhaps with EXPERIMENT_DELIMITER suffix) by refined name.
    result->SetName(name);
    return result;
}

void TMultiParser::ParseDependencies(const TMultiGrammar::TTaskInfo& task) {
    if (!ShouldEnsureEntities) {
        return;
    }
    for (const TString& entityName : task.Task->DependsOnEntities) {
        if (Sample->WasEntityParsed(entityName)) {
            continue;
        }
        const TMultiGrammar::TTaskInfo* entityTask = GetMultiGrammar()->FindTask({PTT_ENTITY, entityName});
        if (entityTask == nullptr) {
            continue;
        }
        ParseEntity(entityName, *entityTask);
    }
}

} // namespace NGranet
