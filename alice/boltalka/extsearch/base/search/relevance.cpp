#include "relevance.h"
#include "util.h"

#include <alice/boltalka/extsearch/base/factors/factor_names.h>
#include <alice/boltalka/extsearch/base/calc_factors/factor_calcer_ctx.h>
#include <alice/boltalka/extsearch/base/calc_factors/dssm_cos_factors.h>
#include <alice/boltalka/generative/service/proto/generative_request.pb.h>
#include <alice/boltalka/generative/service/proto/generative_response.pb.h>
#include <alice/boltalka/generative/service/proto/bert_request.pb.h>
#include <alice/boltalka/generative/service/proto/bert_response.pb.h>
#include <alice/boltalka/libs/text_utils/context_hash.h>

#include <kernel/tarc/docdescr/docdescr.h>
#include <search/rank/reqresults.h>
#include <ysite/yandex/srchmngr/arcmgr.h>

#include <util/random/shuffle.h>
#include <util/random/fast.h>
#include <util/generic/algorithm.h>
#include <util/string/join.h>
#include <util/generic/ymath.h>
#include <util/digest/city.h>
#include <util/system/hp_timer.h>
#include <util/string/cast.h>

#include <library/cpp/neh/http_common.h>
#include <library/cpp/neh/neh.h>
#include <library/cpp/string_utils/base64/base64.h>

namespace NNlg {

namespace {

constexpr auto FAKE_SEQ2SEQ_INDEX_NAME = "__Seq2Seq__";
static const TString DSSM_MODEL_NAME_FOR_FACTORS = "factor_dssm_0_index";

TVector<TBertOutput> GoForBertFactorByUri(const TStringBuf uri, const TVector<TString>& context, const TVector<TString>& candidates,
                                          const ui64 timeout, TUnistatRegistry* unistatRegistry, bool isMultitargetHead) {
    NGenerativeBoltalka::Proto::TBertFactorRequest requestProto;
    auto* requestProtoContext = requestProto.MutableContext();
    for (auto iter = context.rbegin(); iter != context.rend(); ++iter) { // reverse context into sane order
        requestProtoContext->Add(TString(*iter));
    }
    auto* requestProtoCandidates = requestProto.MutableCandidates();
    for (auto& c : candidates) {
        requestProtoCandidates->Add(TString(c));
    }

    auto msg = NNeh::TMessage::FromString(ToString(uri));
    const bool ok = MakeFullRequest(
        msg,
        "Content-Type: application/protobuf\r\nAccept: application/protobuf",
        requestProto.SerializeAsString(),
        "application/protobuf",
        NNeh::NHttp::ERequestType::Post);
    if (!ok) {
        unistatRegistry->BertRequestErrorMakeCount->PushSignal(1);
        ythrow yexception() << "MakeFullRequest";
    }
    const auto resp = NNeh::Request(msg)->Wait(TDuration::MilliSeconds(timeout));

    if (!resp) {
        unistatRegistry->BertRequestErrorTimeoutCount->PushSignal(1);
        ythrow yexception() << "Timeout";
    }
    if (resp->IsError()) {
        unistatRegistry->BertRequestErrorServerCount->PushSignal(1);
        ythrow yexception() << resp->GetErrorText();
    }

    NGenerativeBoltalka::Proto::TBertFactorResponse response;
    if (!response.ParseFromString(resp->Data)) {
        unistatRegistry->BertRequestErrorFormatCount->PushSignal(1);
        ythrow yexception() << "Could not parse response from server";
    }

    TVector<TBertOutput> scores;
    scores.reserve(response.ScoresSize());

    if (!isMultitargetHead) {
        for (const auto& score: response.GetScores()) {
            scores.push_back({score});
        }
    } else {
        for (const auto& bertOutput : response.GetCandidatesScores()) {
            const auto& bertOutputScores = bertOutput.GetScores();
            scores.emplace_back(bertOutputScores.begin(), bertOutputScores.end());
        }
    }
    return scores;
}

NGenerativeBoltalka::Proto::TGenerativeResponse GoForSeq2SeqByUri(const TStringBuf uri, const TVector<TString>& contexts,
                                                                    const ui64 timeout, const ui64 seed, const int numHypos, TUnistatRegistry* unistatRegistry) {
    NGenerativeBoltalka::Proto::TGenerativeRequest requestProto;
    requestProto.SetNumHypos(numHypos);
    auto* requestProtoContext = requestProto.MutableContext();
    for (auto& context : contexts) {
        requestProtoContext->Add(TString(context));
    }

    if (seed) {
        requestProto.MutableSeed()->SetValue(seed);
    }

    auto msg = NNeh::TMessage::FromString(ToString(uri));
    const bool ok = MakeFullRequest(
        msg,
        "Content-Type: application/protobuf\r\nAccept: application/protobuf",
        requestProto.SerializeAsString(),
        "application/protobuf",
        NNeh::NHttp::ERequestType::Post);
    if (!ok) {
        unistatRegistry->Seq2SeqRequestErrorMakeCount->PushSignal(1);
        ythrow yexception() << "MakeFullRequest";
    }
    const auto resp = NNeh::Request(msg)->Wait(TDuration::MilliSeconds(timeout));

    if (!resp) {
        unistatRegistry->Seq2SeqRequestErrorTimeoutCount->PushSignal(1);
        ythrow yexception() << "Timeout";
    }
    if (resp->IsError()) {
        unistatRegistry->Seq2SeqRequestErrorServerCount->PushSignal(1);
        ythrow yexception() << resp->GetErrorText();
    }

    NGenerativeBoltalka::Proto::TGenerativeResponse response;
    if (!response.ParseFromString(resp->Data)) {
        unistatRegistry->Seq2SeqRequestErrorFormatCount->PushSignal(1);
        ythrow yexception() << "Could not parse response from server";
    }

    return response;
}

static TVector<TString> ParseContextFromQuery(const TString& query) {
    TVector<TString> context = StringSplitter(query).Split('\n');
    std::reverse(context.begin(), context.end());
    return context;
}

static void BoostByFactor(const TStaticFactorsStoragePtr& staticFactors, size_t factorId, float weight, TVector<TSearchResult>* docs) {
    if (!staticFactors || weight == 0.0) {
        return;
    }
    // TODO(alipov): mapping from factorId to staticFactorId
    if (factorId == Max<size_t>() || factorId >= GetNlgFactorsInfo()->GetFactorCount() || !GetNlgFactorsInfo()->IsStaticFactor(factorId)) {
        return;
    }
    for (auto& doc : *docs) {
        doc.Score += weight * staticFactors->GetFactor(doc.DocId, factorId);
    }
}

static void SampleFromTopDocs(const TSearchOptions& options, TVector<TSearchResult>* docs) {
    if (docs->size() <= options.MaxResults) {
        return;
    }
    std::sort(docs->begin(), docs->end(), [](const auto& a, const auto& b) {
        return a.Score > b.Score;
    });
    const float bestScore = docs->front().Score;
    for (size_t i = 1; i < docs->size(); ++i) {
        float score = (*docs)[i].Score;
        if (bestScore * options.MinRatioWithBestResponse > score && i >= options.MaxResults) {
            docs->resize(i);
            break;
        }
    }
    if (docs->size() == options.MaxResults) {
        return;
    }
    if (options.RandomSeed) {
        TFastRng64 rng(options.RandomSeed);
        ShuffleRange(*docs, rng);
    } else {
        ShuffleRange(*docs);
    }
    docs->resize(options.MaxResults);
    std::sort(docs->begin(), docs->end(), [](const auto& a, const auto& b) {
        return a.Score > b.Score;
    });
}

static void SelectDocsWithUniqueReplies(const TVector<TString>& replies, TVector<TSearchResult>* docs) {
    THashMap<ui64, TSearchResult> uniqueDocs;
    for (size_t i = 0; i < docs->size(); ++i) {
        auto replyHash = CityHash64(replies[i]);
        if (uniqueDocs.find(replyHash) == uniqueDocs.end() || uniqueDocs[replyHash].Score < (*docs)[i].Score) {
            uniqueDocs[replyHash] = (*docs)[i];
        }
    }
    size_t insertIdx = 0;
    for (const auto& pair: uniqueDocs) {
        (*docs)[insertIdx++] = pair.second;
    }
    docs->resize(insertIdx);
}

static void ApplyTfRanker(const TTfRankerPtr& ranker, bool boostTop, bool boostRelev, const TVector<double>& relevs, TVector<double>* scores, const TFactorCalcerCtx& ctx) {
    const auto& dssmCtx = ctx.DssmFactorCtxs.at("insight_c3_rus_lister");
    ranker->Predict(dssmCtx.QueryEmbedding, dssmCtx.ReplyEmbeddings, dssmCtx.Dimension, relevs, scores, boostTop, boostRelev);
}

double CalcRelevThreshold(const TVector<double>& relevs, const size_t topSize) {
    TVector<double> sortedRelevs(relevs);
    const auto split = sortedRelevs.end() - Min(sortedRelevs.size(), topSize);
    std::nth_element(sortedRelevs.begin(), split, sortedRelevs.end());
    return *split;
}

} // namespace anonymous

TNlgRelevance::TNlgRelevance(const TNlgRelevanceCtx& ctx)
    : Options(ctx.Options)
    , ContextTransform(ctx.Lang)
    , ReplyTransform(ctx.Lang)
    , DssmModelsWithIndexes(ctx.DssmModelsWithIndexes)
    , RankerModel(ctx.RankerModel)
    , BertFactorRankerModel(ctx.BertFactorRankerModel)
    , NumStaticFactors(ctx.NumStaticFactors)
    , FactorDssmModels(ctx.FactorDssmModels)
    , FactorCalcer(ctx.FactorCalcer)
    , StaticFactors(ctx.StaticFactors)
    , TfRanker(ctx.TfRanker)
    , Seq2SeqCandidatesEnabled(Options.Seq2SeqCandidatesEnabled)
    , IndexCandidatesEnabled(Options.IndexCandidatesEnabled)
    , Archive(ctx.Archive)
    , Seq2SeqExecutor(ctx.Seq2SeqExecutor)
    , Logger(ctx.Logger)
    , UnistatRegistry(ctx.UnistatRegistry)
    , ZeroStaticFactors(TVector<float>(NumStaticFactors))
{
}

THashMap<TString, TVector<TVector<float>>> TNlgRelevance::GetQueryEmbeddings(const TVector<TString>& context) const {
    THashMap<TString, TVector<TVector<float>>> queryEmbeddings;
    for (const TString& dssmModelName : Options.DssmModelNames) {
        if (!DssmModelsWithIndexes.count(dssmModelName)) {
            continue;
        }
        auto dssmModelWithIndexes = DssmModelsWithIndexes.at(dssmModelName);
        queryEmbeddings[dssmModelName] = dssmModelWithIndexes->GetQueryEmbeddings(context, Options);
    }
    return queryEmbeddings;
}

TSearchResultDocs TNlgRelevance::GetFixlistDocs(const TVector<TString>& context) const {
    const auto fixlistAnswers = Fixlist.MatchContext(context);
    TVector<TSearchResult> docs;
    TVector<TVector<TString>> contexts;
    TVector<TString> replies;
    TVector<TString> displayReplies;
    for (size_t i = 0; i < fixlistAnswers.size(); ++i) {
        TSearchResult searchResult;
        searchResult.DocId = Archive.GetDocCount() + i;
        searchResult.DssmIndexName = FAKE_SEQ2SEQ_INDEX_NAME;
        searchResult.KnnIndexName = FAKE_SEQ2SEQ_INDEX_NAME;
        searchResult.Score = 1;
        docs.push_back(searchResult);
        contexts.emplace_back(TVector<TString>(context.size(), ""));
        replies.push_back(fixlistAnswers[i]);
        displayReplies.push_back(fixlistAnswers[i]);;
    }
    THashMap<TString, TVector<TVector<float>>> seq2SeqReplyEmbeddings;
    TVector<TSeq2SeqFactorCtx> seq2SeqFactors;
    return {docs, contexts, replies, displayReplies, seq2SeqReplyEmbeddings, seq2SeqFactors};
}

TSearchResultDocs TNlgRelevance::GetDocsFromIndex(const THashMap<TString, TVector<TVector<float>>>& queryEmbeddings) const {
    TVector<TSearchResult> docs;
    TVector<TString> priorityKnnIndexes = Options.ProactivityPrioritizedInIndex ? TVector<TString>(Options.ProactivityKnnIndexNames) : TVector<TString>{};
    priorityKnnIndexes.insert(priorityKnnIndexes.end(), Options.Entities.begin(), Options.Entities.end());
    for (const auto& pair : queryEmbeddings) {
        const auto& dssmModelName = pair.first;
        const auto& queryEmbedding = pair.second;
        auto allowedKnnIndexes = Options.KnnIndexNames;
        allowedKnnIndexes.insert(allowedKnnIndexes.end(), Options.Entities.begin(), Options.Entities.end());
        const auto& indexDocs = DssmModelsWithIndexes.at(dssmModelName)->GetReplies(queryEmbedding, allowedKnnIndexes, Options, priorityKnnIndexes);
        docs.reserve(docs.size() + indexDocs.size());
        for (const auto& doc : indexDocs) {
            docs.emplace_back(doc, dssmModelName);
        }
    }

    TVector<TVector<TString>> contexts;
    TVector<TString> replies;
    TVector<TString> displayReplies;
    GetTextsFromArchive(docs, &contexts, &replies, &displayReplies);

    THashMap<TString, TVector<TVector<float>>> seq2SeqReplyEmbeddings;
    TVector<TSeq2SeqFactorCtx> seq2SeqFactors;
    if (Seq2SeqCandidatesEnabled) {
        for (const auto& [dssmName, _] : DssmModelsWithIndexes) {
            seq2SeqReplyEmbeddings[dssmName] = TVector<TVector<float>>(docs.size(), TVector<float>());
        }
        seq2SeqFactors = TVector<TSeq2SeqFactorCtx>(docs.size(), {/*IsSeq2Seq*/ false, /*Score*/ 0.0, /*NumTokens*/ 0});
    }

    return {docs, contexts, replies, displayReplies, seq2SeqReplyEmbeddings, seq2SeqFactors};
}

TSearchResultDocs TNlgRelevance::GetDocsFromSeq2Seq(const TVector<TString>& context) const {
    TVector<TSearchResult> docs;
    TVector<TVector<TString>> contexts;
    TVector<TString> replies;
    TVector<TString> displayReplies;
    THashMap<TString, TVector<TVector<float>>> replyEmbeddings;

    THPTimer watchGet;
    auto responses = GoForSeq2SeqByUri(Options.Seq2SeqExternalUri, context, Options.Seq2SeqTimeout, Options.RandomSeed, Options.Seq2SeqNumHypos, UnistatRegistry).GetResponses();
    UnistatRegistry->Seq2SeqRequestGetTime->PushSignal(watchGet.Passed()*1000);
    UnistatRegistry->Seq2SeqRequestCandidatesCount->PushSignal(responses.size());
    TVector<TString> seq2seqResult;
    TVector<TSeq2SeqFactorCtx> seq2SeqFactors;

    for (int i = 0; i < responses.size(); ++i) {
        TSearchResult seq2SeqCandidate;
        seq2SeqCandidate.DocId = Archive.GetDocCount() + i; // assign docids that aren't used in the index
        seq2SeqCandidate.DssmIndexName = FAKE_SEQ2SEQ_INDEX_NAME;
        seq2SeqCandidate.KnnIndexName = FAKE_SEQ2SEQ_INDEX_NAME;
        docs.push_back(seq2SeqCandidate);

        contexts.emplace_back(TVector<TString>(contexts.size(), ""));  // historical context is empty for seq2seq
        auto response = responses[i].GetResponse();
        replies.push_back(ReplyTransform.Transform(response));
        displayReplies.push_back(response);
        seq2SeqFactors.push_back({/*IsSeq2Seq*/ true, responses[i].GetScore(), responses[i].GetNumTokens()});
    }

    THPTimer watchEmdedding;
    for (const auto& [dssmName, dssmModel] : DssmModelsWithIndexes) {
        if (Options.UseSeq2SeqDssmEmbedding) {
            replyEmbeddings[dssmName] = dssmModel->GetReplyEmbeddings(replies);
        } else {
            replyEmbeddings[dssmName] = TVector<TVector<float>>(replies.size(), TVector<float>(dssmModel->GetDimension()));
        }
    }
    UnistatRegistry->Seq2SeqRequestEmbeddingTime->PushSignal(watchEmdedding.Passed()*1000);

    return {docs, contexts, replies, displayReplies, replyEmbeddings, seq2SeqFactors};
}

TSeq2SeqFuture TNlgRelevance::GetDocsFromSeq2SeqAsync(const TVector<TString>& context) const {
    Y_ASSERT(Seq2SeqExecutor->GetQueueSize() < Seq2SeqExecutor->GetThreadCount());

    auto seq2SeqPromise = NThreading::NewPromise<TSeq2SeqFuture::value_type>();
    auto job = [seq2SeqPromise, &context, this](int) mutable {
            THPTimer watchSeq2Seq;
            TSeq2SeqAsyncResult result;
            try {
                result.ResultDocs = GetDocsFromSeq2Seq(context);
            } catch (...) {
                if (Logger) {
                    TStringBuilder builder;
                    builder << "Context:\t" + JoinRange("\\t", context.rbegin(), context.rend());
                    builder << " Seq2SeqError: " << CurrentExceptionMessage();
                    Logger->Write(builder);
                }

                result.HasError = true;
                UnistatRegistry->Seq2SeqRequestErrorCount->PushSignal(1);
            }
            result.TimeElapsed = watchSeq2Seq.Passed();
            seq2SeqPromise.SetValue(std::move(result));
        };

    Seq2SeqExecutor->Exec(job, 0, 0);

    return seq2SeqPromise.GetFuture();
}

TSearchResultDocs TNlgRelevance::CalcFilteringResultsForQuery(const TString& query) {
    if (Options.DebugStub) {
        usleep(Options.DebugStub * 1000);
        TSearchResultDocs allDocs;
        for (size_t i = 0; i < Options.MaxResults; ++i) {
            TSearchResult doc;
            doc.DocId = i;
            doc.Score = 1;
            allDocs.Docs.push_back(doc);
        }

        return allDocs;
    }
    auto originalContext = ParseContextFromQuery(query);
    auto context = TransformContext(originalContext);

    // TODO: remove RandomSalt after requests from protocol
    if (!Options.RandomSeed && Options.RandomSalt) {
        Options.RandomSeed = NNlgTextUtils::CalculateContextHash(context, Options.RandomSalt.GetRef());
    }

    if (Logger) {
        Logger->Write("Context:\t" + JoinRange("\\t", context.rbegin(), context.rend()));
    }

    auto fixlistDocs = GetFixlistDocs(context);
    if (!fixlistDocs.Docs.empty()) {
        fixlistDocs.Finalize();
        return fixlistDocs;
    }

    THPTimer watch;
    TSearchResultDocs allDocs;
    auto queryEmbeddings = GetQueryEmbeddings(context);

    TSeq2SeqFuture seq2seqFuture;
    if (Seq2SeqCandidatesEnabled) {
        UnistatRegistry->Seq2SeqRequestCount->PushSignal(1);
        seq2seqFuture = GetDocsFromSeq2SeqAsync(originalContext);
    }

    THPTimer watchIndex;
    if (IndexCandidatesEnabled) {
        UnistatRegistry->IndexRequestCount->PushSignal(1);
        auto indexDocs = GetDocsFromIndex(queryEmbeddings);
        allDocs.Add(indexDocs);
    }
    double docsFromIndexTimeElapsed = watchIndex.Passed();

    TSeq2SeqAsyncResult seq2seqResult;
    if (Seq2SeqCandidatesEnabled) {
        seq2seqFuture.Wait();
        seq2seqResult = std::move(seq2seqFuture.GetValue());
        if (!seq2seqResult.HasError) {
            allDocs.Add(seq2seqResult.ResultDocs);
        }
    }
    allDocs.Finalize();

    TFactorCalcerCtx ctx(allDocs.Contexts, allDocs.Replies, allDocs.Seq2SeqFactorCtxs);

    if (Options.SearchBy == TDssmIndex::ESearchBy::Context
            && Options.SearchFor == TDssmIndex::ESearchFor::ContextAndReply
            && RankerModel
            && FactorCalcer
            && !queryEmbeddings.empty()
            && !ctx.Contexts.empty()) {
        THPTimer watchReranker;
        RerankDocs(context, queryEmbeddings, &ctx, allDocs);
        UnistatRegistry->RequestRerankerTime->PushSignal(watchReranker.Passed()*1000);
    }
    BoostByFactor(StaticFactors, Options.BoostFactorId, Options.BoostFactorWeight, &allDocs.Docs);
    if (Options.UniqueReplies) {
        SelectDocsWithUniqueReplies(ctx.Replies, &allDocs.Docs);
    }
    SampleFromTopDocs(Options, &allDocs.Docs);
    const auto totalTimeElapsed = watch.Passed();

    if (IndexCandidatesEnabled) {
        UnistatRegistry->IndexRequestTime->PushSignal(docsFromIndexTimeElapsed*1000);
    }
    if (Seq2SeqCandidatesEnabled) {
        UnistatRegistry->Seq2SeqRequestTime->PushSignal(seq2seqResult.TimeElapsed*1000);
    }
    UnistatRegistry->RequestTime->PushSignal(totalTimeElapsed*1000);
    if (Logger) {
        TStringBuilder builder;
        builder << "Context:\t" + JoinRange("\\t", context.rbegin(), context.rend());
        builder << "\tTotalTimeElapsed: " << totalTimeElapsed;
        builder << " GetDocsFromIndex: " << docsFromIndexTimeElapsed;
        builder << " GetDocsFromSeq2Seq: " << seq2seqResult.TimeElapsed;
        Logger->Write(builder);
    }

    return allDocs;
}

bool TNlgRelevance::CalcFilteringResults(TCalcFilteringResultsContext& ctx) {
    const auto result = CalcFilteringResultsForQuery(ctx.RP.UserRequest);

    for (const auto& scoredDoc : result.Docs) {
        const auto docId = scoredDoc.DocId;
        TRelevance relev = Max((Options.TfRankerLogitRelev ? scoredDoc.TfRankerScore : scoredDoc.Score) * 1e9, 0.0);
        TWeighedDoc doc(docId, 0, relev);
        ctx.Results->SearchResult.push_back(doc);
        if (scoredDoc.DssmIndexName == FAKE_SEQ2SEQ_INDEX_NAME) {
            ctx.Results->Seq2SeqResult.emplace(docId, result.DisplayReplies[result.DocIdToIdx.find(docId)->second]);
        }
        ctx.Results->NlgSearchFactorsResult.emplace(docId, Base64Encode(result.DocIdToFactors.find(docId)->second.SerializeAsString()));
    }
    ctx.Results->AnswerIsComplete = true;
    ctx.Results->YxErrCode = yxOK;
    ctx.Results->NumDocs[0] = result.Docs.size();
    return true;
}

TVector<TString> TNlgRelevance::TransformContext(TVector<TString> context) const {
    // some of transformations rely on parity of turn (even - for user, odd - for model)
    // this hack ensures that transformations are applied to right turns
    if (Options.SearchBy != TDssmIndex::ESearchBy::Context) {
        context.insert(context.begin(), ""); // fake empty user turn
    }
    context = ContextTransform.Transform(context);
    if (Options.SearchBy != TDssmIndex::ESearchBy::Context) {
        context.erase(context.begin());
    }
    return context;
}

TVector<TBertOutput> TNlgRelevance::GetBertScoresByThreshold(const TVector<TString>& queryContext, const TVector<TString>& replies, const TVector<double>& relevs, double relevThreshold) const {
    TVector<TString> candidates;
    for (size_t i = 0; i < relevs.size(); ++i) {
        if (relevs[i] >= relevThreshold) {
            candidates.push_back(replies[i]);
        }
    }
    UnistatRegistry->BertRequestCount->PushSignal(1);
    THPTimer start;
    TVector<TBertOutput> bertScores;
    try {
        bertScores = GoForBertFactorByUri(Options.BertFactorExternalUri, queryContext, candidates, Options.BertFactorTimeout, UnistatRegistry, Options.BertIsMultitargetHead);
    } catch (...) {
        if (Logger) {
            Logger->Write(CurrentExceptionMessage());
            UnistatRegistry->BertRequestErrorCount->PushSignal(1);
        }
        return {};
    }
    UnistatRegistry->BertRequestTime->PushSignal(start.Passed() * 1000);
    if (bertScores.empty()) {
        return {};
    }
    if (bertScores.back().size() <= Options.BertOutputRelevIdx) {
        if (Logger) {
            Logger->Write("BERT relevance score is missing");
        }
        return {};
    }
    if (Options.BertOutputHasInterest && bertScores.back().size() <= Options.BertOutputInterestIdx) {
        if (Logger) {
            Logger->Write("Requested BERT interestingness score is missing");
        }
        return {};
    }
    return bertScores;
}

void TNlgRelevance::RerankWithBertFactor(const TVector<TString>& queryContext, const TVector<TVector<float>>& factors, const TVector<TString>& replies, TVector<double>* relevs) const {
    if (!Options.BertFactorEnabled || !BertFactorRankerModel || !Options.BertFactorExternalUri) {
        return;
    }
    const double relevThreshold = CalcRelevThreshold(*relevs, Options.BertFactorTopSize);
    auto bertScores = GetBertScoresByThreshold(queryContext, replies, *relevs, relevThreshold);
    if (bertScores.empty()) {
        return;
    }
    TVector<TBertOutput> bertFactors;
    bertFactors.reserve(Options.BertFactorTopSize);
    size_t bertScoreIdx = 0;
    for (size_t i = 0; i < relevs->size(); ++i) {
        if ((*relevs)[i] >= relevThreshold) {
            bertFactors.push_back(factors[i]);
            bertFactors.back().push_back(bertScores[bertScoreIdx++][Options.BertOutputRelevIdx]);
        }
    }
    TVector<double> bertRelevs;
    BertFactorRankerModel->CalcRelevs(bertFactors, bertRelevs);
    bertScoreIdx = 0;
    for (size_t i = 0; i < relevs->size(); ++i) {
        (*relevs)[i] = (*relevs)[i] >= relevThreshold ? bertRelevs[bertScoreIdx++] : -1e6;
    }
    Y_ASSERT(bertScoreIdx == bertScores.size());
}

void TNlgRelevance::RerankByLinearCombination(const TVector<TString>& queryContext, const TVector<TString>& replies, const TVector<TVector<float>>& factors, TVector<double>* relevs) const {
    if (!Options.RankByLinearCombination || !Options.BertFactorExternalUri) {
        return;
    }
    const double relevThreshold = CalcRelevThreshold(*relevs, Options.BertFactorTopSize);
    auto bertScores = GetBertScoresByThreshold(queryContext, replies, *relevs, relevThreshold);
    if (bertScores.empty()) {
        return;
    }
    size_t bertScoreIdx = 0;
    for (size_t i = 0; i < relevs->size(); ++i) {
        if ((*relevs)[i] >= relevThreshold) {
            (*relevs)[i] = Options.LinearCombinationBertCoeff * bertScores[bertScoreIdx][Options.BertOutputRelevIdx] +
                           Options.LinearCombinationInformativityCoeff * GetFactorValue(factors[i], INFORMATIVENESS_FACTOR_NAME) +
                           Options.LinearCombinationSeq2SeqCoeff * GetFactorValue(factors[i], SEQ2SEQ_FACTOR_NAME);
            if (Options.BertOutputHasInterest) {
                (*relevs)[i] += Options.LinearCombinationInterestCoeff * bertScores[bertScoreIdx][Options.BertOutputInterestIdx];
            }
            ++bertScoreIdx;
        } else {
            (*relevs)[i] = -1e6;
        }
    }
    Y_ASSERT(bertScoreIdx == bertScores.size());
}

void TNlgRelevance::RerankDocs(const TVector<TString>& queryContext,
                               const THashMap<TString, TVector<TVector<float>>>& queryEmbeddings,
                               TFactorCalcerCtx* ctx,
                               TSearchResultDocs& docs) const {
    const size_t numDocs = docs.Docs.size();

    TVector<TVector<float>> factors(numDocs);
    GetStaticFactors(docs.Docs, &factors, ctx);

    ctx->QueryContext = queryContext;

    for (const auto& pair : queryEmbeddings) {
        const auto& dssmModelName = pair.first;
        auto& baseModelDssmCtx = ctx->DssmFactorCtxs[dssmModelName];
        const auto& queryEmbedding = pair.second[0];
        baseModelDssmCtx.QueryEmbedding = queryEmbedding.data();
        baseModelDssmCtx.Dimension = queryEmbedding.size();
        baseModelDssmCtx.ContextEmbeddings.reserve(numDocs);
        baseModelDssmCtx.ReplyEmbeddings.reserve(numDocs);
        GetEmbeddingsFromKnnIndex(dssmModelName, docs, &baseModelDssmCtx.ContextEmbeddings, &baseModelDssmCtx.ReplyEmbeddings, baseModelDssmCtx.Dimension);

        if (baseModelDssmCtx.ContextEmbeddings.empty()) {
            return;
        }
    }

    // as DssmFactorCtx does not own embeddings we need to store them separately
    TList<TVector<float>> factorDssmQueryEmbeddings;
    GetEmbeddingsFromFactorDssms(docs, queryContext, &factorDssmQueryEmbeddings, ctx);

    for (const auto& doc : docs.Docs) {
        ctx->DssmIndexNames.push_back(doc.DssmIndexName);
        ctx->KnnIndexNames.push_back(doc.KnnIndexName);
    }

    FactorCalcer->CalcFactors(ctx, &factors);
    for (size_t i = 0; i < numDocs; ++i) {
        const auto& docFactors = factors[i];
        NAlice::NBoltalka::TNlgSearchFactors nlgSearchFactors;
        nlgSearchFactors.SetSeq2Seq(GetFactorValue(docFactors, SEQ2SEQ_FACTOR_NAME));
        nlgSearchFactors.SetInformativeness(GetFactorValue(docFactors, INFORMATIVENESS_FACTOR_NAME));
        if (FindPtr(Options.DssmModelNames, DSSM_MODEL_NAME_FOR_FACTORS)) {
            nlgSearchFactors.SetDssmScore(GetFactorValue(docFactors, DSSM_COS_FACTORS_NAME_PREFIX + DSSM_MODEL_NAME_FOR_FACTORS, DSSM_QUERY_REPLY_COSINE_FACTOR_SHIFT));
        }
        docs.SetDocFactors(docs.Docs[i].DocId, std::move(nlgSearchFactors));
    }

    TVector<double> relevs;
    RankerModel->CalcRelevs(factors, relevs);
    if (Options.Seq2SeqPriority) {
        double maxRelev = *std::max_element(relevs.begin(), relevs.end());
        if (Seq2SeqCandidatesEnabled) {
            for (size_t i = 0; i < numDocs; ++i) {
                if (docs.Docs[i].DssmIndexName == FAKE_SEQ2SEQ_INDEX_NAME) {
                    relevs[i] = maxRelev;
                }
            }
        }
    }

    RerankWithBertFactor(queryContext, factors, docs.Replies, &relevs);

    RerankByLinearCombination(queryContext, docs.Replies, factors, &relevs);

    for (size_t i = 0; i < numDocs; ++i) {
        if (FindPtr(Options.ProactivityKnnIndexNames, docs.Docs[i].KnnIndexName)) {
            relevs[i] += Options.ProactivityBoost;
        }
        if (FindPtr(Options.EntityIndexNames, DropEntityIdFromIndexName(docs.Docs[i].KnnIndexName))) {
            relevs[i] += Options.EntityBoost;
        }
        if (Seq2SeqCandidatesEnabled) {
            relevs[i] += Options.Seq2SeqBoost * GetFactorValue(factors[i], SEQ2SEQ_FACTOR_NAME);
        }
    }

    TVector<double> tfRankerScores;
    if (TfRanker) {
        if (Options.TfRankerBoostRelev) {
            double maxRelev = *std::max_element(relevs.begin(), relevs.end());
            for (size_t i = 0; i < numDocs; ++i) {
                relevs[i] = exp(relevs[i] - maxRelev);
            }
        }
        ApplyTfRanker(TfRanker, Options.TfRankerBoostTop, Options.TfRankerBoostRelev, relevs, &tfRankerScores, *ctx);
        double maxScore = *std::max_element(tfRankerScores.begin(), tfRankerScores.end());
        for (size_t i = 0; i < numDocs; ++i) {
            docs.Docs[i].TfRankerScore = exp(tfRankerScores[i] - maxScore);
            relevs[i] += Options.TfRankerAlpha * tfRankerScores[i];
        }
    }

    double maxRelev = *std::max_element(relevs.begin(), relevs.end());
    for (size_t i = 0; i < numDocs; ++i) {
        docs.Docs[i].Score = exp(relevs[i] - maxRelev);
    }
}

void TNlgRelevance::GetEmbeddingsFromKnnIndex(const TString& dssmIndexName,
                                              const TSearchResultDocs& docs,
                                              TVector<const float*>* contextEmbeddings,
                                              TVector<const float*>* replyEmbeddings,
                                              size_t dimension) const {
    auto dssmModelWithIndexes = DssmModelsWithIndexes.at(dssmIndexName);
    for (const auto& doc : docs.Docs) {
        size_t idx = docs.DocIdToIdx.at(doc.DocId);
        if (doc.KnnIndexName == FAKE_SEQ2SEQ_INDEX_NAME) {
            auto& docsSeq2SeqReplyEmbeddings = docs.Seq2SeqReplyEmbeddings.at(dssmIndexName);
            contextEmbeddings->push_back(dssmModelWithIndexes->GetZeroEmbedding());
            replyEmbeddings->push_back(&docsSeq2SeqReplyEmbeddings[idx][0]);
            continue;
        }
        auto dssmIndex = dssmModelWithIndexes->GetDssmIndex(doc.KnnIndexName);
        if (!dssmIndex) {
            return;
        }
        auto knnIndex = dssmIndex->GetKnnIndex(TDssmIndex::ESearchFor::ContextAndReply);
        if (!knnIndex) {
            if (IsEntityIndex(doc.KnnIndexName)) {
                knnIndex = dssmIndex->GetKnnIndex(TDssmIndex::ESearchFor::Reply);
                if (!knnIndex) {
                    return;
                }
                const auto* embedding = knnIndex->GetVector(doc.VectorId);
                contextEmbeddings->push_back(dssmModelWithIndexes->GetZeroEmbedding());
                replyEmbeddings->push_back(embedding);
                continue;
            }
            return;
        }
        const auto* embedding = knnIndex->GetVector(doc.VectorId);
        contextEmbeddings->push_back(embedding);
        replyEmbeddings->push_back(embedding + dimension);
    }
}

void TNlgRelevance::GetEmbeddingsFromFactorDssms(const TSearchResultDocs& docs,
                                                 const TVector<TString>& queryContext,
                                                 TList<TVector<float>>* queryEmbeddings,
                                                 TFactorCalcerCtx* ctx) const {
    for (const auto& pair : FactorDssmModels) {
        const auto& modelName = pair.first;
        const auto& model = pair.second;
        queryEmbeddings->emplace_back(model->Fprop(queryContext));

        auto& dssmCtx = ctx->DssmFactorCtxs[modelName];
        dssmCtx.Dimension = model->GetDimension();
        dssmCtx.QueryEmbedding = queryEmbeddings->back().data();
        dssmCtx.ContextEmbeddings.reserve(docs.Docs.size());
        dssmCtx.ReplyEmbeddings.reserve(docs.Docs.size());
        for (const auto& doc : docs.Docs) {
            if (doc.KnnIndexName == FAKE_SEQ2SEQ_INDEX_NAME) {
                dssmCtx.ContextEmbeddings.push_back(model->GetZeroEmbedding());
                dssmCtx.ReplyEmbeddings.push_back(model->GetZeroEmbedding());
            } else {
                const auto* embedding = model->GetContextReplyEmbedding(doc.DocId);
                dssmCtx.ContextEmbeddings.push_back(embedding);
                dssmCtx.ReplyEmbeddings.push_back(embedding + dssmCtx.Dimension);
            }
        }
    }
}

size_t TNlgRelevance::GetDimension(const TString& dssmIndexName) const {
    auto dssmModelWithIndexes = DssmModelsWithIndexes.at(dssmIndexName);
    auto dssmIndex = dssmModelWithIndexes->GetDssmIndex(Options.BaseKnnIndexName);
    if (!dssmIndex) {
        return 0;
    }
    auto knnIndex = dssmIndex->GetKnnIndex(TDssmIndex::ESearchFor::ContextAndReply);
    if (!knnIndex) {
        return 0;
    }
    return knnIndex->GetDimension() / 2;
}

void TNlgRelevance::GetTextsFromArchive(const TVector<TSearchResult>& docs,
                                        TVector<TVector<TString>>* contexts,
                                        TVector<TString>* replies,
                                        TVector<TString>* displayReplies) const {
    for (const auto& doc : docs) {
        TBlob blob = Archive.GetDocExtInfo(doc.DocId)->UncompressBlob();
        TDocDescr desc;
        desc.UseBlob(blob.AsCharPtr(), blob.Size());

        THashMap<TString, TString> infos;
        desc.ConfigureDocInfos(infos);
        // TODO(alipov): simplify format of contexts in archive
        TVector<TString> context = StringSplitter(infos["context"]).SplitByString(" _EOS_ ");
        std::reverse(context.begin(), context.end());
        contexts->push_back(ContextTransform.Transform(context));
        auto reply = infos["reply"];
        replies->push_back(ReplyTransform.Transform(reply));
        displayReplies->push_back(reply);
    }
}

TVector<TVector<float>> TNlgRelevance::GetIndexQueryEmbeddings(const TString& dssmIndexName, const TVector<TString>& context) const {
    return DssmModelsWithIndexes.at(dssmIndexName)->GetQueryEmbeddings(TransformContext(context), Options);
}

TVector<float> TNlgRelevance::GetIndexReplyEmbedding(const TString& dssmIndexName, const TString& reply) const {
    return DssmModelsWithIndexes.at(dssmIndexName)->GetReplyEmbedding(ReplyTransform.Transform(reply));
}

void TNlgRelevance::GetStaticFactors(const TVector<TSearchResult>& docs, TVector<TVector<float>>* factors, TFactorCalcerCtx* ctx) const {
    ctx->NumStaticFactors = NumStaticFactors;
    if (!StaticFactors) {
        Y_ENSURE(NumStaticFactors == 0, "Static factors were requested but had not been provided");
        return;
    }
    Y_ENSURE(NumStaticFactors <= StaticFactors->GetNumFactors(), "Not enough static factors");
    for (size_t i = 0; i < docs.size(); ++i) {
        const auto& doc = docs[i];
        auto& docFactors = (*factors)[i];

        const float* staticFactors;
        if (doc.DocId < Archive.GetDocCount()) {
            staticFactors = StaticFactors->GetFactors(doc.DocId);
        } else {
            staticFactors = ZeroStaticFactors.data();
        }
        docFactors.reserve(docFactors.size() + NumStaticFactors);
        docFactors.insert(docFactors.end(), staticFactors, staticFactors + NumStaticFactors);
        ctx->StaticFactors.push_back(staticFactors);
    }
}

float TNlgRelevance::GetFactorValue(const TVector<float>& factors, const TString& factorName, size_t factorShift) const {
    const auto factorsLocation = FactorCalcer->GetFactorsLocation(factorName);
    Y_ENSURE(factorsLocation, "'" + factorName + "'" + " factor is not found");
    Y_ENSURE(factorsLocation->first + factorShift < factorsLocation->second, "factor index is out of bounds");
    return factors[NumStaticFactors + factorsLocation->first + factorShift];
}

const IFactorsInfo* TNlgRelevance::GetFactorsInfo() const {
    return NNlg::GetNlgFactorsInfo();
}

void TNlgRelevance::CalcFactors(TCalcFactorsContext& /*ctx*/) {
}

void TNlgRelevance::CalcRelevance(TCalcRelevanceContext& /*ctx*/) {
}

TNlgRelevance* CreateNlgRelevance(const TNlgRelevanceCtx& ctx) {
    ctx.UnistatRegistry->RequestCount->PushSignal(1);
    return new TNlgRelevance(ctx);
}

} // namespace NNlg
