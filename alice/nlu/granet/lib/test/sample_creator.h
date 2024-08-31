#pragma once

#include "dataset.h"
#include "simple_cache.h"
#include <alice/nlu/granet/lib/grammar/domain.h>
#include <alice/nlu/granet/lib/grammar/multi_grammar.h>
#include <alice/nlu/granet/lib/parser/preprocessed_sample.h>
#include <alice/nlu/granet/lib/sample/sample.h>

namespace NGranet {

// ~~~~ EEntitySourceType ~~~~

enum EEntitySourceType : ui32 {
    EST_TSV     = FLAG32(0),
    EST_ONLINE  = FLAG32(1),
    EST_EMPTY   = FLAG32(2)
};

Y_DECLARE_FLAGS(EEntitySourceTypes, EEntitySourceType);
Y_DECLARE_OPERATORS_FOR_FLAGS(EEntitySourceTypes)

// ~~~~ Function ~~~~

TSample::TRef CreateSampleFromTsv(const TTsvSample& tsvSample, const TGranetDomain& domain,
    EEntitySourceTypes entitySources);

TMultiPreprocessedSample::TRef CreatePreprocessedSampleFromTsv(const TTsvSample& tsvSample,
    const TGranetDomain& domain, EEntitySourceTypes entitySources,
    const TMultiGrammar::TConstRef grammar);

// ~~~~ TSampleCreatorWithCache ~~~~

// CreateSampleFromTsv with cache.
class TSampleCreatorWithCache : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TSampleCreatorWithCache>;
    using TConstRef = TIntrusiveConstPtr<TSampleCreatorWithCache>;

public:
    static TSampleCreatorWithCache::TRef Create(const TGranetDomain& domain, size_t cacheLimit = 0);

    TSample::TRef CreateSample(const TTsvSample& tsvSample, EEntitySourceTypes entitySources);

private:
    using TKey = std::tuple<TString, TString, TString, EEntitySourceTypes>;

private:
    TSampleCreatorWithCache() = delete;
    TSampleCreatorWithCache(const TGranetDomain& domain, size_t cacheLimit);

private:
    const TGranetDomain Domain;
    TSimpleCache<TKey, TSample::TConstRef> Cache;
};

// ~~~~ TPreprocessedSampleCreatorWithCache ~~~~

// CreatePreprocessedSampleFromTsv with cache.
class TPreprocessedSampleCreatorWithCache : public TThrRefBase {
public:
    using TRef = TIntrusivePtr<TPreprocessedSampleCreatorWithCache>;
    using TConstRef = TIntrusiveConstPtr<TPreprocessedSampleCreatorWithCache>;

public:
    static TPreprocessedSampleCreatorWithCache::TRef Create(const TGrammar::TConstRef& grammar, size_t cacheLimit = 0);
    static TPreprocessedSampleCreatorWithCache::TRef Create(const TMultiGrammar::TConstRef& grammar,
        const TGranetDomain& domain, size_t cacheLimit = 0);

    const TMultiGrammar::TConstRef& GetGrammar() {
        return Grammar;
    }

    TMultiPreprocessedSample::TRef CreateSample(const TTsvSample& tsvSample,
        EEntitySourceTypes entitySources, const TFsPath& contextStoragePath);

private:
    using TKey = std::tuple<TString, TString, TString, TString, EEntitySourceTypes, TString>;

private:
    TPreprocessedSampleCreatorWithCache() = delete;
    TPreprocessedSampleCreatorWithCache(const TMultiGrammar::TConstRef& grammar, const TGranetDomain& domain,
        size_t cacheLimit);

private:
    const TMultiGrammar::TConstRef Grammar;
    const TGranetDomain Domain;
    TSimpleCache<TKey, TMultiPreprocessedSample::TRef> Cache;
};

} // namespace NGranet
