#include "sample_processor.h"
#include <alice/nlu/granet/lib/compiler/compiler.h>
#include <alice/nlu/granet/lib/parser/multi_parser.h>
#include <alice/nlu/granet/lib/user_entity/collect_from_context.h>
#include <alice/nlu/granet/lib/user_entity/finder.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <dict/nerutil/tstimer.h>
#include <util/datetime/base.h>
#include <util/generic/xrange.h>
#include <util/system/hp_timer.h>

namespace NGranet {

using namespace NJson;

// ~~~~ Common ~~~~

static const double MICROSECONDS_IN_SECOND = 1000000;

static NUserEntity::TEntityDicts ReadUserEntityDicts(const TTsvSample& tsvSample, const TContextPatchStorage& contextStorage) {
    const TString patchIdsStr = tsvSample.TsvLine.Value(ESampleColumnId::Context, TString());
    if (patchIdsStr.empty()) {
        return {};
    }
    const TVector<TString> patchIds = StringSplitter(patchIdsStr).Split(',').ToList<TString>();
    const TJsonValue context = contextStorage.BuildContext(patchIds);
    NUserEntity::TEntityDicts dicts;
    NUserEntity::CollectDicts(context["request"]["device_state"], &dicts);
    return dicts;
}

// ~~~~ TSampleProcessorResult ~~~~

void TSampleProcessorResult::Dump(IOutputStream* log, const TString& indent) const {
    Y_ENSURE(log);
    *log << indent << "TSampleProcessorResult:" << Endl;
    *log << indent << "  Result: " << Result.PrintForReport(0) << Endl;
    *log << indent << "  Time: " << Time << Endl;
    if (!Blocker.empty()) {
        *log << indent << "  Blocker: " << Blocker << Endl;
    }
    *log << indent << "  ParserResult:" << Endl;
    if (ParserResult) {
        ParserResult->Dump(log, indent + "    ");
    } else {
        *log << indent << "    nullptr" << Endl;
    }
}

// ~~~~ TSampleProcessorByGrammar ~~~~

TSampleProcessorByGrammar::TSampleProcessorByGrammar(const TOptions& options, const TFsPath& grammarPath,
        const TVector<TFsPath>& grammarSourceDirs)
    : TSampleProcessorByGrammar(options, CompileGrammar(options.Domain, grammarPath, grammarSourceDirs))
{
}

// static
TGrammar::TRef TSampleProcessorByGrammar::CompileGrammar(const TGranetDomain& domain, const TFsPath& grammarPath,
    const TVector<TFsPath>& grammarSourceDirs)
{
    return NCompiler::TCompiler({.SourceDirs = grammarSourceDirs}).CompileFromPath(grammarPath, domain);
}

TSampleProcessorByGrammar::TSampleProcessorByGrammar(const TOptions& options, const TGrammar::TConstRef& grammar)
    : TSampleProcessorByGrammar(options, TMultiGrammar::Create(grammar))
{
}

TSampleProcessorByGrammar::TSampleProcessorByGrammar(const TOptions& options, const TMultiGrammar::TConstRef& grammar)
    : TSampleProcessorByGrammar(options, TPreprocessedSampleCreatorWithCache::Create(grammar, options.Domain))
{
}

TSampleProcessorByGrammar::TSampleProcessorByGrammar(const TOptions& options,
        const TPreprocessedSampleCreatorWithCache::TRef& creator)
    : Options(options)
    , Creator(creator)
{
    Y_ENSURE(Creator);

    if (Options.ContextStoragePath.IsDefined()) {
        ContextStorage.LoadFromPath(Options.ContextStoragePath);
    }
    if (Options.TaskName.empty()) {
        TVector<TString> taskNameVariants;
        for (const auto& [taskKey, taskInfo] : Creator->GetGrammar()->GetTasks()) {
            if (taskKey.Type == Options.TaskType) {
                taskNameVariants.push_back(taskKey.Name);
            }
        }
        Y_ENSURE(taskNameVariants.size() == 1, "Parser task name (form or entity name) is not defined.");
        Options.TaskName = taskNameVariants[0];
    }
}

TSampleProcessorResult TSampleProcessorByGrammar::ProcessSample(const TTsvSample& tsvSample,
    bool isGroundtruthPositive, IOutputStream* log)
{
    DEBUG_TIMER("NGranet::TSampleProcessorByGrammar::ProcessSample");
    TMultiPreprocessedSample::TRef sample = Creator->CreateSample(tsvSample, Options.EntitySources, Options.ContextStoragePath);

    if (ContextStorage.IsDefined() && sample->SetEntityWasParsed("_local_user_entities")) {
        const NUserEntity::TEntityDicts dicts = ReadUserEntityDicts(tsvSample, ContextStorage);
        sample->AddFoundEntitiesToSample(NUserEntity::FindEntitiesInSample(dicts, sample->GetSample(), Options.Domain.Lang));
    }

    TMultiParser::TRef parser = TMultiParser::Create(sample, true);
    parser->SetLog(log, false);
    parser->SetNeedDebugInfo(true);

    TSampleProcessorResult result;
    const THPTimer timer;
    result.ParserResult = parser->ParseTask(Options.TaskType, Options.TaskName);
    result.Time = timer.Passed() * MICROSECONDS_IN_SECOND;
    result.Result = result.ParserResult->ToMarkup();
    if (Options.CollectBlockers && !result.Result.IsPositive && isGroundtruthPositive) {
        result.Blocker = FindBlocker(sample);
    }
    return result;
}

TString TSampleProcessorByGrammar::FindBlocker(const TMultiPreprocessedSample::TRef& sample) const {
    TMultiParser::TRef parser = TMultiParser::Create(sample, false);
    parser->SetNeedDebugInfo(true);
    parser->SetCollectBlockersMode(true);
    TParserTaskResult::TConstRef parserResult = parser->ParseTask(Options.TaskType, Options.TaskName);
    Y_ENSURE(parserResult);
    if (parserResult->IsPositive()) {
        return "STATE_LIMIT";
    }
    if (parserResult->GetDebugInfo() == nullptr) {
        return "INTERNAL_ERROR";
    }
    const TString blocker = parserResult->GetDebugInfo()->FindParserBlockerTokenStr();
    if (!blocker.empty()) {
        return blocker;
    }
    return "WEAK_TEXT";
}

TString TSampleProcessorByGrammar::GetName() const {
    return TStringBuilder() << Options.TaskType << ' ' << Options.TaskName;
}

// ~~~~ TSampleProcessorByUserEntity ~~~~

TSampleProcessorByUserEntity::TSampleProcessorByUserEntity(const TOptions& options)
    : Options(options)
{
    if (Options.ContextStoragePath.IsDefined()) {
        ContextStorage.LoadFromPath(options.ContextStoragePath);
    }
}

TSampleProcessorResult TSampleProcessorByUserEntity::ProcessSample(const TTsvSample& tsvSample, bool,
    IOutputStream* log)
{
    DEBUG_TIMER("NGranet::TSampleProcessorByUserEntity::ProcessSample");

    TSampleProcessorResult result;
    result.Result.IsPositive = false;
    result.Result.Text = tsvSample.CleanText;

    const NUserEntity::TEntityDicts dicts = FilterDicts(ReadUserEntityDicts(tsvSample, ContextStorage));
    if (log) {
        dicts.Dump(log);
    }
    if (dicts.Dicts.empty()) {
        return result;
    }

    TSample::TRef sample = CreateSampleFromTsv(tsvSample, Options.Domain, EST_EMPTY);
    const TString fstText = NNlu::TRequestNormalizer::Normalize(Options.Domain.Lang, sample->GetText());

    const THPTimer timer;
    const TVector<TEntity> entities = NUserEntity::FindEntitiesInSample(dicts, sample, fstText, {Options.Domain.Lang, LANG_ENG}, log);
    sample->AddEntitiesOnTokens(entities);
    result.Time = timer.Passed() * MICROSECONDS_IN_SECOND;
    result.Result = sample->GetEntitiesAsMarkup(Options.EntityName, false);
    return result;
}

NUserEntity::TEntityDicts TSampleProcessorByUserEntity::FilterDicts(NUserEntity::TEntityDicts&& original) const {
    NUserEntity::TEntityDicts filtered;
    for (NUserEntity::TEntityDictPtr& dict : original.Dicts) {
        if (dict->EntityName == Options.EntityName) {
            filtered.Dicts.push_back(std::move(dict));
        }
    }
    return filtered;
}

TString TSampleProcessorByUserEntity::GetName() const {
    return "user_entity " + Options.EntityName;
}

// ~~~~ TSampleProcessorAlwaysTrue ~~~~

TSampleProcessorResult TSampleProcessorAlwaysTrue::ProcessSample(const TTsvSample& sample, bool, IOutputStream*) {
    TSampleProcessorResult result;
    result.Result = ReadSampleMarkup(true, sample.TaggedText);
    return result;
}

// ~~~~ TSampleProcessorByDatasetColumn ~~~~

TSampleProcessorByDatasetColumn::TSampleProcessorByDatasetColumn(const TVector<TFsPath>& datasets,
    const TString& columnName, const TString& columnValue)
{
    for (const TFsPath& path : datasets) {
        const TTsvSampleDataset dataset(path);
        for (const size_t i : xrange(dataset.Size())) {
            const TTsvSample sample = dataset.ReadSample(i);
            (sample.TsvLine[columnName] == columnValue ? HasPositive : HasNegative).insert(sample.MakeKey());
        }
    }
}

TSampleProcessorResult TSampleProcessorByDatasetColumn::ProcessSample(const TTsvSample& sample,
    bool isGroundtruthPositive, IOutputStream*)
{
    TSampleProcessorResult result;
    const bool isPositive = (isGroundtruthPositive ? HasPositive : HasNegative).contains(sample.MakeKey());
    result.Result = ReadSampleMarkup(isPositive, sample.TaggedText);
    return result;
}

} // namespace NGranet
