#pragma once

#include "context_storage.h"
#include "dataset.h"
#include "metrics.h"
#include "sample_creator.h"
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/nlu/granet/lib/parser/result.h>
#include <alice/nlu/granet/lib/user_entity/dictionary.h>

namespace NGranet {

// ~~~~ TSampleProcessorResult ~~~~

struct TSampleProcessorResult {
    TSampleMarkup Result;
    double Time = 0; // microseconds

    // Optional extra info from parser
    TParserTaskResult::TConstRef ParserResult;
    TString Blocker;

    void Dump(IOutputStream* log, const TString& indent = "") const;
};

// ~~~~ TSampleProcessor ~~~~

class TSampleProcessor : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TSampleProcessor>;
    using TConstRef = TIntrusiveConstPtr<TSampleProcessor>;

public:
    virtual ~TSampleProcessor() = default;

    virtual TSampleProcessorResult ProcessSample(const TTsvSample& record, bool isGroundtruthPositive,
        IOutputStream* log) = 0;

    virtual TString GetName() const {
        return "";
    }
};

// ~~~~ TSampleProcessorByGrammar ~~~~

class TSampleProcessorByGrammar : public TSampleProcessor {
public:
    struct TOptions {
        EParserTaskType TaskType = PTT_FORM;
        TString TaskName;
        TGranetDomain Domain;
        TFsPath ContextStoragePath;
        EEntitySourceTypes EntitySources = EST_TSV | EST_ONLINE;
        bool CollectBlockers = false;
    };

public:
    TSampleProcessorByGrammar(const TOptions& options, const TFsPath& grammarPath,
        const TVector<TFsPath>& grammarSourceDirs);
    TSampleProcessorByGrammar(const TOptions& options, const TGrammar::TConstRef& grammar);
    TSampleProcessorByGrammar(const TOptions& options, const TMultiGrammar::TConstRef& grammar);
    TSampleProcessorByGrammar(const TOptions& options, const TPreprocessedSampleCreatorWithCache::TRef& creator);

    virtual TSampleProcessorResult ProcessSample(const TTsvSample& record, bool isGroundtruthPositive,
        IOutputStream* log) override;

    virtual TString GetName() const override;

private:
    static TGrammar::TRef CompileGrammar(const TGranetDomain& domain, const TFsPath& grammarPath,
        const TVector<TFsPath>& grammarSourceDirs);
    TString FindBlocker(const TMultiPreprocessedSample::TRef& sample) const;

protected:
    TOptions Options;
    TPreprocessedSampleCreatorWithCache::TRef Creator;
    TContextPatchStorage ContextStorage;
};

// ~~~~ TSampleProcessorByUserEntity ~~~~

class TSampleProcessorByUserEntity : public TSampleProcessor {
public:
    struct TOptions {
        TString EntityName;
        TGranetDomain Domain;
        TFsPath ContextStoragePath;
    };

public:
    explicit TSampleProcessorByUserEntity(const TOptions& options);

    virtual TSampleProcessorResult ProcessSample(const TTsvSample& record, bool isGroundtruthPositive,
        IOutputStream* log) override;

    virtual TString GetName() const override;

private:
    NUserEntity::TEntityDicts FilterDicts(NUserEntity::TEntityDicts&& original) const;

private:
    TOptions Options;
    TContextPatchStorage ContextStorage;
};

// ~~~~ TSampleProcessorAlwaysTrue ~~~~

class TSampleProcessorAlwaysTrue : public TSampleProcessor {
public:
    virtual TSampleProcessorResult ProcessSample(const TTsvSample& record, bool isGroundtruthPositive,
        IOutputStream* log) override;
};

// ~~~~ TSampleProcessorByDatasetColumn ~~~~

class TSampleProcessorByDatasetColumn : public TSampleProcessor {
public:
    TSampleProcessorByDatasetColumn(const TVector<TFsPath>& datasets,
        const TString& columnName, const TString& columnValue);

    virtual TSampleProcessorResult ProcessSample(const TTsvSample& record, bool isGroundtruthPositive,
        IOutputStream* log) override;

private:
    THashSet<TTsvSampleKey> HasPositive;
    THashSet<TTsvSampleKey> HasNegative;
};

} // namespace NGranet
