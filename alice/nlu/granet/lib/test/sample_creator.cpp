#include "sample_creator.h"
#include "fetcher.h"
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <dict/nerutil/tstimer.h>

namespace NGranet {

using namespace NJson;

// ~~~~ Functions ~~~~

static bool TryLoadEntitiesFromTsv(const TTsvSample& tsvSample, const TSample::TRef& parserSample) {
    if (tsvSample.Mock.empty()) {
        return false;
    }
    const TJsonValue json = NJsonUtils::ReadJsonStringVerbose(tsvSample.Mock, "");
    return parserSample->LoadEntitiesFromJson(json["Entities"]);
}

TSample::TRef CreateSampleFromTsv(const TTsvSample& tsvSample, const TGranetDomain& domain,
    EEntitySourceTypes entitySources)
{
    DEBUG_TIMER("NGranet::CreateSampleFromTsv");

    Y_ENSURE(domain.Lang != LANG_UNK, "Language not defined");

    TSample::TRef parserSample = TSample::Create(tsvSample.CleanText, domain.Lang);

    if (entitySources.HasFlags(EST_TSV) && TryLoadEntitiesFromTsv(tsvSample, parserSample)) {
        return parserSample;
    }
    if (entitySources.HasFlags(EST_ONLINE)) {
        TBegemotFetcherOptions options;
        options.Wizextra = tsvSample.Wizextra;
        FetchSampleEntities(parserSample, domain, options);
        return parserSample;
    }
    Y_ENSURE(entitySources.HasFlags(EST_EMPTY),
        "No entity sources for text " + Cite(tsvSample.CleanText) + " from " + tsvSample.TsvLine.FormatErrorPosition());
    return parserSample;
}

TMultiPreprocessedSample::TRef CreatePreprocessedSampleFromTsv(const TTsvSample& tsvSample,
    const TGranetDomain& domain, EEntitySourceTypes entitySources,
    const TMultiGrammar::TConstRef grammar)
{
    DEBUG_TIMER("NGranet::CreatePreprocessedSampleFromTsv");

    TSample::TRef sample = CreateSampleFromTsv(tsvSample, domain, entitySources);
    return TMultiPreprocessedSample::Create(grammar, sample);
}

// ~~~~ TSampleCreatorWithCache ~~~~

TSampleCreatorWithCache::TRef TSampleCreatorWithCache::Create(const TGranetDomain& domain, size_t cacheLimit) {
    return new TSampleCreatorWithCache(domain, cacheLimit);
}

TSampleCreatorWithCache::TSampleCreatorWithCache(const TGranetDomain& domain, size_t cacheLimit)
    : Domain(domain)
    , Cache(cacheLimit)
{
}

TSample::TRef TSampleCreatorWithCache::CreateSample(const TTsvSample& sample, EEntitySourceTypes entitySources) {
    DEBUG_TIMER("NGranet::TSampleCreatorWithCache::CreateSample");

    if (Cache.GetCacheLimit() == 0) {
        return CreateSampleFromTsv(sample, Domain, entitySources);
    }
    const TKey key{sample.CleanText, sample.Wizextra, sample.Mock, entitySources};
    if (const TMaybe<TSample::TConstRef> cached = Cache.Find(key); cached.Defined()) {
        return (*cached)->Copy();
    }
    TSample::TConstRef created = CreateSampleFromTsv(sample, Domain, entitySources);
    Cache.Insert(key, created);
    return created->Copy();
}

// ~~~~ TPreprocessedSampleCreatorWithCache ~~~~

TPreprocessedSampleCreatorWithCache::TRef TPreprocessedSampleCreatorWithCache::Create(
    const TGrammar::TConstRef& grammar, size_t cacheLimit)
{
    return new TPreprocessedSampleCreatorWithCache(TMultiGrammar::Create(grammar), grammar->GetDomain(), cacheLimit);
}

TPreprocessedSampleCreatorWithCache::TRef TPreprocessedSampleCreatorWithCache::Create(
    const TMultiGrammar::TConstRef& grammar, const TGranetDomain& domain, size_t cacheLimit)
{
    return new TPreprocessedSampleCreatorWithCache(grammar, domain, cacheLimit);
}

TPreprocessedSampleCreatorWithCache::TPreprocessedSampleCreatorWithCache(
        const TMultiGrammar::TConstRef& grammar, const TGranetDomain& domain, size_t cacheLimit)
    : Grammar(grammar)
    , Domain(domain)
    , Cache(cacheLimit)
{
}

TMultiPreprocessedSample::TRef TPreprocessedSampleCreatorWithCache::CreateSample(const TTsvSample& sample,
    EEntitySourceTypes entitySources, const TFsPath& contextStoragePath)
{
    DEBUG_TIMER("NGranet::TPreprocessedSampleCreatorWithCache::CreateSample");

    if (Cache.GetCacheLimit() == 0) {
        return CreatePreprocessedSampleFromTsv(sample, Domain, entitySources, Grammar);
    }
    const TKey key{sample.CleanText, sample.Wizextra, sample.Mock, sample.Context, entitySources, contextStoragePath};
    if (const TMaybe<TMultiPreprocessedSample::TRef> cached = Cache.Find(key); cached.Defined()) {
        return (*cached)->Copy();
    }
    TMultiPreprocessedSample::TRef created = CreatePreprocessedSampleFromTsv(sample, Domain, entitySources, Grammar);
    Cache.Insert(key, created);
    return created->Copy();
}

} // namespace NGranet
