#include "nlgsearch_simple.h"

#include <search/config/virthost.h>
#include <ysite/yandex/srchmngr/arcmgr.h>
#include <search/reqparam/reqparam.h>

#include <util/folder/path.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NNlg {

namespace {

TString ParseKnnIndexConfig(TStringBuf configLine) {
    TVector<TString> knnIndexNames;
    for (const auto& knnIndexParams : StringSplitter(configLine).Split(',')) {
        TVector<TString> tokens = StringSplitter(knnIndexParams.Token()).Split(':');
        knnIndexNames.push_back(tokens[0]);
    }
    return JoinSeq(",", knnIndexNames);
}

}

TNlgSearchSimple::TNlgSearchSimple(const TNlgSearchSimpleParams& params) : ArchiveManager(TFsPath(params.IndexDir) / "index") {
    TSearchConfig searchConfig;
    searchConfig.UserDirectives["MemoryMode"] = params.MemoryMode;
    searchConfig.UserDirectives["LoggingQueueSize"] = "0";
    searchConfig.UserDirectives["SearchFor"] = "context_and_reply";
    searchConfig.UserDirectives["SearchBy"] = "context";
    searchConfig.UserDirectives["MaxResults"] = ToString(params.MaxResults);
    searchConfig.UserDirectives["DssmModelNames"] = params.DssmModelNames;
    searchConfig.UserDirectives["KnnIndexesToLoad"] = params.KnnIndexNames;
    searchConfig.UserDirectives["KnnIndexNames"] = ParseKnnIndexConfig(params.KnnIndexNames);
    searchConfig.UserDirectives["MklThreads"] = ToString(params.NumThreads);
    searchConfig.UserDirectives["Seq2SeqExternalUri"] = ToString(params.Seq2SeqExternalUri);
    searchConfig.ProtoCollection_.SetRequestThreads(params.NumThreads);

    if (!params.BaseDssmModelName.empty()) {
        searchConfig.UserDirectives["BaseDssmModelName"] = params.BaseDssmModelName;
    }

    if (!params.FactorDssmModelNames.empty()) {
        searchConfig.UserDirectives["FactorDssmModelNames"] = params.FactorDssmModelNames;
    }

    if (!params.RankerModelNamesToLoad.empty()) {
        searchConfig.UserDirectives["RankerModelsToLoad"] = params.RankerModelNamesToLoad;
    }

    if (!params.RankerModelName.empty()) {
        searchConfig.UserDirectives["RankerModelName"] = params.RankerModelName;
    } else if (!params.RankerModelNamesToLoad.empty()) {
        TVector<TString> rankerModelNamesToLoad = StringSplitter(params.RankerModelNamesToLoad).Split(',');
        searchConfig.UserDirectives["RankerModelName"] = StringSplitter(rankerModelNamesToLoad[0]).Split(':').ToList<TString>()[0];
    }

    if (!params.BaseKnnIndexName.empty()) {
        searchConfig.UserDirectives["BaseKnnIndexName"] = params.BaseKnnIndexName;
    }

    Searcher = CreateNlgSearch(params.IndexDir, searchConfig);

    TRequestParams requestParams;
    if (!params.Experiments.empty()) {
        for (const auto& experimentName : StringSplitter(params.Experiments).Split(',')) {
            requestParams.Experiments.insert(TString{experimentName.Token()});
        }
    }

    Relevance = Searcher->CreateRelevanceDeprecated(ArchiveManager, &requestParams);
    for (auto split : StringSplitter(params.DssmModelNames).Split(',')) {
        const auto& modelName = ToString(split.Token());
        DssmModelNamesPool.insert(modelName);
        Dimensions[modelName] = Relevance->GetDimension(modelName);
    }
}

TVector<float> TNlgSearchSimple::EmbedContext(const TString& dssmModelName, const TVector<TString>& context) const {
    Y_ENSURE(DssmModelNamesPool.find(dssmModelName) != DssmModelNamesPool.end(), "Model is not found");
    TVector<TString> reversed_context(context);
    std::reverse(reversed_context.begin(), reversed_context.end());
    return Relevance->GetIndexQueryEmbeddings(dssmModelName, reversed_context)[0];
}

TVector<float> TNlgSearchSimple::EmbedReply(const TString& dssmModelName, const TString& reply) const {
    Y_ENSURE(DssmModelNamesPool.find(dssmModelName) != DssmModelNamesPool.end(), "Model is not found");
    return Relevance->GetIndexReplyEmbedding(dssmModelName, reply);
}

TString TNlgSearchSimple::MakeQueryFromContext(const TVector<TString>& context) const {
    return JoinRange("\n", context.begin(), context.end());
}

TVector<TString> GetTextsFromDocs(TSearchResultDocs& docs) {
    TVector<TString> texts;
    for (auto& doc : docs.Docs) {
        texts.push_back(docs.Replies[docs.DocIdToIdx.find(doc.DocId)->second]);
    }
    return texts;
}

TVector<TString> TNlgSearchSimple::GetReplyTexts(const TVector<TString>& context) const {
    auto results = Relevance->CalcFilteringResultsForQuery(MakeQueryFromContext(context));
    return GetTextsFromDocs(results);
}

TVector<const float*> TNlgSearchSimple::GetReplyEmbeddings(const TString& dssmModelName, const TVector<TString>& context) const {
    Y_ENSURE(DssmModelNamesPool.find(dssmModelName) != DssmModelNamesPool.end(), "Model is not found");
    auto results = Relevance->CalcFilteringResultsForQuery(MakeQueryFromContext(context));
    TVector<const float*> contextEmbeddings;
    TVector<const float*> replyEmbeddings;
    Relevance->GetEmbeddingsFromKnnIndex(dssmModelName, results, &contextEmbeddings, &replyEmbeddings, Dimensions.at(dssmModelName));
    return replyEmbeddings;
}

void TNlgSearchSimple::GetCandidates(const TString& dssmModelName, const TVector<TString>& context, TVector<const float *>* replyEmbeddings, TVector<TString>* replies, TVector<float>* contextEmbedding, TVector<float>* scores) const {
    Y_ENSURE(DssmModelNamesPool.find(dssmModelName) != DssmModelNamesPool.end(), "Model is not found");
    auto results = Relevance->CalcFilteringResultsForQuery(MakeQueryFromContext(context));
    TVector<const float*> contextEmbeddings;
    TVector<TString> reversed_context(context);
    std::reverse(reversed_context.begin(), reversed_context.end());
    *contextEmbedding = Relevance->GetIndexQueryEmbeddings(dssmModelName, reversed_context)[0];
    Relevance->GetEmbeddingsFromKnnIndex(dssmModelName, results, &contextEmbeddings, replyEmbeddings, Dimensions.at(dssmModelName));
    *replies = GetTextsFromDocs(results);
    scores->reserve(results.Docs.size());
    for (const auto& el : results.Docs) {
        scores->push_back(el.Score);
    }
}

size_t TNlgSearchSimple::GetDimension(const TString& dssmModelName) const {
    Y_ENSURE(DssmModelNamesPool.find(dssmModelName) != DssmModelNamesPool.end(), "Model is not found");
    return Dimensions.at(dssmModelName);
}

TVector<TNlgSearchSimple::TSearchResult> TNlgSearchSimple::GetSearchResults(const TVector<TString>& context) const {
    auto results = Relevance->CalcFilteringResultsForQuery(MakeQueryFromContext(context));
    TVector<TVector<TString>> unusedContexts;
    TVector<TString> replies;
    TVector<TString> displayReplies;
    Relevance->GetTextsFromArchive(results.Docs, &unusedContexts, &replies, &displayReplies);
    TVector<TSearchResult> searchResults;
    for (size_t i = 0; i < results.Docs.size(); ++i) {
        searchResults.push_back({results.Docs[i].Score, displayReplies[i]});
    }
    return searchResults;
}

}
