#include "metrics.h"

#include <library/cpp/dot_product/dot_product.h>
#include <library/cpp/threading/local_executor/local_executor.h>

#include <mapreduce/yt/interface/client.h>
#include <mapreduce/yt/interface/operation.h>

#include <util/generic/vector.h>
#include <util/generic/ymath.h>
#include <util/string/vector.h>
#include <util/string/split.h>
#include <util/string/join.h>
#include <util/generic/queue.h>
#include <util/digest/city.h>
#include <util/generic/hash_set.h>

namespace {

struct TEmbedding {
    TEmbedding() = default;
    template <class T>
    TEmbedding(const T* ptr, size_t len)
        : Values(ptr, ptr + len)
        , Norm(sqrt(DotProduct(Values.data(), Values.data(), len)))
    {
    }

    TVector<float> Values;
    float Norm;
    Y_SAVELOAD_DEFINE(Values, Norm);
};

static float CalcCosine(const TEmbedding& e1, const TEmbedding& e2) {
    return DotProduct(e1.Values.data(), e2.Values.data(), e1.Values.size()) / (e1.Norm * e2.Norm);
}

struct THashScorePair {
    ui64 Hash;
    float Score;
    TString ToString() const {
        return ::ToString(Hash) + "," + ::ToString(Score);
    }
};

struct THashScorePairBetter {
    bool operator()(const THashScorePair& a, const THashScorePair& b) const {
        return a.Score > b.Score;
    }
};

static ui64 GetReplyHash(const NYT::TNode& row, const TVector<TString>& columns) {
    TVector<TString> parts;
    for (auto column : columns) {
        parts.push_back(row[column].AsString());
    }
    return CityHash64(JoinSeq("\t", parts));
}

using TTopHashScorePairs = TPriorityQueue<THashScorePair, TVector<THashScorePair>, THashScorePairBetter>;

static void UpdateQueryTopHashScorePairs(TTopHashScorePairs& topPairs, const THashScorePair& newPair, size_t topSize) {
    if (topPairs.size() < topSize || THashScorePairBetter()(newPair, topPairs.top())) {
        topPairs.emplace(newPair);
        if (topPairs.size() > topSize) {
            topPairs.pop();
        }
    }
}

static TVector<ui64> TopHashScorePairs2RankedHashes(TTopHashScorePairs& top) {
    TVector<ui64> rankedHashes(top.size());
    for (; !top.empty(); top.pop()) {
        rankedHashes[top.size() - 1] = top.top().Hash;
    }
    return rankedHashes;
}

static TVector<float> CalcRecall(const TVector<ui64>& trueTop, const TVector<ui64>& newTop) {
    Y_VERIFY(trueTop.size() == newTop.size());
    THashMultiSet<ui64> newSet(newTop.begin(), newTop.end());
    TVector<float> recall(trueTop.size());
    size_t intersectionSize = 0;
    for (size_t rank = 0; rank < trueTop.size(); ++rank) {
        auto trueKey = trueTop[rank];
        auto iter = newSet.find(trueKey);
        if (iter != newSet.end()) {
            ++intersectionSize;
            newSet.erase(iter);
        }
        recall[rank] = intersectionSize / (rank + 1.0);
    }
    return recall;
}

static TVector<float> CalcTir(const TVector<ui64>& trueTop, const TVector<ui64>& newTop) {
    Y_VERIFY(trueTop.size() == newTop.size());
    THashMultiSet<ui64> trueSet;
    THashMultiSet<ui64> newSet;
    TVector<float> tir(trueTop.size());
    size_t intersectionSize = 0;
    for (size_t rank = 0; rank < trueTop.size(); ++rank) {
        auto trueKey = trueTop[rank];
        auto newKey = newTop[rank];
        if (trueKey == newKey) {
            ++intersectionSize;
        } else {
            auto iter = newSet.find(trueKey);
            if (iter != newSet.end()) {
                ++intersectionSize;
                newSet.erase(iter);
            } else {
                trueSet.insert(trueKey);
            }
            iter = trueSet.find(newKey);
            if (iter != trueSet.end()) {
                ++intersectionSize;
                trueSet.erase(iter);
            } else {
                newSet.insert(newKey);
            }
        }
        tir[rank] = intersectionSize / (rank + 1.0);
    }
    return tir;
}

static void GetColumnEmbeddingsInRow(const NYT::TNode& row,
                                     TString column,
                                     size_t embeddingDim,
                                     const TVector<TQuantizer>& quantizers,
                                     TEmbedding* embedding,
                                     TVector<TEmbedding>* embeddingsQuant) {
    const auto* ptr = reinterpret_cast<const float*>(row[column].AsString().data());
    *embedding = TEmbedding(ptr, embeddingDim);
    for (const auto& quantizer : quantizers) {
        auto valuesQuant = quantizer.Apply(embedding->Values);
        embeddingsQuant->emplace_back(valuesQuant.data(), embeddingDim);
    }
}

static void GetColumnEmbeddingsInTable(TIntrusivePtr<NYT::IClient> client,
                                       TString table,
                                       TString column,
                                       const TVector<TQuantizer>& quantizers,
                                       TVector<TEmbedding>* embs,
                                       TVector<TVector<TEmbedding>>* embsQuant) {
    auto reader = client->CreateTableReader<NYT::TNode>(table);
    const size_t embeddingDim = reader->GetRow()[column].AsString().length() / sizeof(float);
    const size_t numRows = client->Get(table + "/@row_count").AsInt64();
    embs->resize(numRows);
    embsQuant->resize(numRows);
    size_t rowIdx = 0;
    for (; reader->IsValid(); reader->Next()) {
        const auto& row = reader->GetRow();
        GetColumnEmbeddingsInRow(row, column, embeddingDim, quantizers, &(embs->at(rowIdx)), &(embsQuant->at(rowIdx)));
        ++rowIdx;
    }
}

}

class TCalcQuantizationMseMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TCalcQuantizationMseMapper() = default;

    TCalcQuantizationMseMapper(const THashMap<TString, TVector<TQuantizer>>& column2Quantizers, TString queryColumn = "")
        : Column2Quantizers(column2Quantizers)
        , QueryColumn(queryColumn)
        , Columns({"context_embedding", "reply_embedding"})
    {
        for (auto column : Columns) {
            Y_VERIFY(Column2Quantizers.find(column) != Column2Quantizers.end());
        }
        if (QueryColumn != "") {
            Columns.push_back(QueryColumn);
            Column2Quantizers[QueryColumn] = Column2Quantizers["context_embedding"];
        }
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const size_t embeddingDim = input->GetRow()[Columns[0]].AsString().length() / sizeof(float);
        THashMap<TString, TVector<double>> embeddingMse;
        THashMap<TString, TVector<double>> cosineMse;

        for (size_t i = 0; i < Columns.size(); ++i) {
            auto leftColumn = Columns[i];
            embeddingMse[leftColumn].resize(Column2Quantizers[leftColumn].size());
            for (size_t j = i + 1; j < Columns.size(); ++j) {
                auto rightColumn = Columns[j];
                auto columnPairStr = leftColumn + "," + rightColumn;
                cosineMse[columnPairStr].resize(Column2Quantizers[leftColumn].size() * Column2Quantizers[rightColumn].size());
            }
        }

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();

            THashMap<TString, TEmbedding> column2Embedding;
            THashMap<TString, TVector<TEmbedding>> column2EmbeddingsQuant;
            for (const auto& p : Column2Quantizers) {
                auto column = p.first;
                GetColumnEmbeddingsInRow(row, column, embeddingDim, p.second, &column2Embedding[column], &column2EmbeddingsQuant[column]);
            }
            AddEmbeddingMseFromRow(column2Embedding, column2EmbeddingsQuant, &embeddingMse);
            AddCosineMseFromRow(column2Embedding, column2EmbeddingsQuant, &cosineMse);
        }

        NYT::TNode result;
        size_t quantilePairIdx = 0;
        for (size_t contextQuantileIdx = 0; contextQuantileIdx < Column2Quantizers["context_embedding"].size(); ++contextQuantileIdx) {
            result["context_quantile"] = Column2Quantizers["context_embedding"][contextQuantileIdx].GetQuantile();
            for (auto column : Columns) {
                if (column != "reply_embedding") {
                    result["embedding_mse(" + column + ")"] = embeddingMse[column][contextQuantileIdx];
                }
            }
            for (size_t replyQuantileIdx = 0; replyQuantileIdx < Column2Quantizers["reply_embedding"].size(); ++replyQuantileIdx) {
                result["reply_quantile"] = Column2Quantizers["reply_embedding"][replyQuantileIdx].GetQuantile();
                result["embedding_mse(reply_embedding)"] = embeddingMse["reply_embedding"][replyQuantileIdx];
                for (size_t i = 0; i < Columns.size(); ++i) {
                    auto leftColumn = Columns[i];
                    for (size_t j = i + 1; j < Columns.size(); ++j) {
                        auto rightColumn = Columns[j];
                        auto columnPairStr = leftColumn + "," + rightColumn;
                        result["cosine_mse(" + columnPairStr + ")"] = cosineMse[columnPairStr][quantilePairIdx];
                    }
                }
                output->AddRow(result);
                ++quantilePairIdx;
            }
        }
    }

    Y_SAVELOAD_JOB(Column2Quantizers, QueryColumn, Columns);

private:
    void AddEmbeddingMseFromRow(const THashMap<TString, TEmbedding>& column2Embedding,
                                const THashMap<TString, TVector<TEmbedding>>& column2EmbeddingsQuant,
                                THashMap<TString, TVector<double>>* embeddingMse) {
        for (auto column : Columns) {
            for (size_t quantileIdx = 0; quantileIdx < Column2Quantizers[column].size(); ++quantileIdx) {
                (*embeddingMse)[column][quantileIdx] += 2 * (1. - CalcCosine(column2Embedding.at(column), column2EmbeddingsQuant.at(column)[quantileIdx]));
            }
        }
    }

    void AddCosineMseFromRow(const THashMap<TString, TEmbedding>& column2Embedding,
                             const THashMap<TString, TVector<TEmbedding>>& column2EmbeddingsQuant,
                             THashMap<TString, TVector<double>>* cosineMse) {
        for (size_t i = 0; i < Columns.size(); ++i) {
            auto leftColumn = Columns[i];
            const auto& leftEmbedding = column2Embedding.at(leftColumn);
            for (size_t j = i + 1; j < Columns.size(); ++j) {
                auto rightColumn = Columns[j];
                const auto& rightEmbedding = column2Embedding.at(rightColumn);
                auto columnPairStr = leftColumn + "," + rightColumn;
                auto cosine = CalcCosine(leftEmbedding, rightEmbedding);
                size_t quantilePairIdx = 0;
                for (size_t leftQuantileIdx = 0; leftQuantileIdx < Column2Quantizers[leftColumn].size(); ++leftQuantileIdx) {
                    const auto& leftEmbeddingQuant = column2EmbeddingsQuant.at(leftColumn)[leftQuantileIdx];
                    for (size_t rightQuantileIdx = 0; rightQuantileIdx < Column2Quantizers[rightColumn].size(); ++rightQuantileIdx) {
                        const auto& rightEmbeddingQuant = column2EmbeddingsQuant.at(rightColumn)[rightQuantileIdx];
                        auto cosineQuant = CalcCosine(leftEmbeddingQuant, rightEmbeddingQuant);
                        (*cosineMse)[columnPairStr][quantilePairIdx] += Sqr(cosine - cosineQuant);
                        ++quantilePairIdx;
                    }
                }
            }
        }
    }

private:
    THashMap<TString, TVector<TQuantizer>> Column2Quantizers;
    TString QueryColumn;
    TVector<TString> Columns;
};
REGISTER_MAPPER(TCalcQuantizationMseMapper);

class TCalcQuantizationMseWithQueriesMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TCalcQuantizationMseWithQueriesMapper() = default;

    TCalcQuantizationMseWithQueriesMapper(const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                             const TVector<TEmbedding>& queriesEmbeddings,
                             const TVector<TVector<TEmbedding>>& queriesEmbeddingsQuant,
                             float contextWeight,
                             size_t threadCount)
        : Column2Quantizers(column2Quantizers)
        , QueriesEmbeddings(queriesEmbeddings)
        , QueriesEmbeddingsQuant(queriesEmbeddingsQuant)
        , ContextWeight(contextWeight)
        , ThreadCount(threadCount)
    {
        Y_VERIFY(QueriesEmbeddings.size() == QueriesEmbeddingsQuant.size());
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(ThreadCount - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const size_t embeddingDim = input->GetRow()["context_embedding"].AsString().length() / sizeof(float);
        auto numQuantilesPairs = Column2Quantizers["context_embedding"].size() * Column2Quantizers["reply_embedding"].size();
        TVector<TVector<double>> totalLoss(numQuantilesPairs, TVector<double>(QueriesEmbeddings.size(), 0.));

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();

            THashMap<TString, TEmbedding> column2Embedding;
            THashMap<TString, TVector<TEmbedding>> column2EmbeddingsQuant;
            for (const auto& p : Column2Quantizers) {
                auto column = p.first;
                GetColumnEmbeddingsInRow(row, column, embeddingDim, p.second, &column2Embedding[column], &column2EmbeddingsQuant[column]);
            }

            auto task = [&](size_t queryIdx) {
                const auto& queryEmbedding = QueriesEmbeddings[queryIdx];
                const auto& contextEmbedding = column2Embedding["context_embedding"];
                const auto& replyEmbedding = column2Embedding["reply_embedding"];
                auto score = ContextWeight * CalcCosine(queryEmbedding, contextEmbedding)
                    + (1. - ContextWeight) * CalcCosine(queryEmbedding, replyEmbedding);
                size_t quantilePairIdx = 0;
                for (size_t contextQuantileIdx = 0; contextQuantileIdx < Column2Quantizers["context_embedding"].size(); ++contextQuantileIdx) {
                    const auto& queryEmbeddingQuant = QueriesEmbeddingsQuant[queryIdx][contextQuantileIdx];
                    const auto& contextEmbeddingQuant = column2EmbeddingsQuant["context_embedding"][contextQuantileIdx];
                    auto queryContextScoreQuant = ContextWeight * CalcCosine(queryEmbeddingQuant, contextEmbeddingQuant);
                    for (size_t replyQuantileIdx = 0; replyQuantileIdx < Column2Quantizers["reply_embedding"].size(); ++replyQuantileIdx) {
                        const auto& replyEmbeddingQuant = column2EmbeddingsQuant["reply_embedding"][replyQuantileIdx];
                        auto scoreQuant = queryContextScoreQuant + (1. - ContextWeight) * CalcCosine(queryEmbeddingQuant, replyEmbeddingQuant);
                        auto loss = Sqr(score - scoreQuant);
                        totalLoss[quantilePairIdx][queryIdx] += loss;
                        ++quantilePairIdx;
                    }
                }
            };
            NPar::LocalExecutor().ExecRange(task, 0, QueriesEmbeddings.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        }

        NYT::TNode result;
        size_t quantilePairIdx = 0;
        for (size_t i = 0; i < Column2Quantizers["context_embedding"].size(); ++i) {
            result["context_quantile"] = Column2Quantizers["context_embedding"][i].GetQuantile();
            for (size_t j = 0; j < Column2Quantizers["reply_embedding"].size(); ++j) {
                result["reply_quantile"] = Column2Quantizers["reply_embedding"][j].GetQuantile();
                double lossSum = 0.;
                for (auto v : totalLoss[quantilePairIdx]) {
                    lossSum += v;
                }
                result["cosine_mse"] = lossSum / QueriesEmbeddings.size();
                output->AddRow(result);
                ++quantilePairIdx;
            }
        }
    }

    Y_SAVELOAD_JOB(Column2Quantizers, QueriesEmbeddings, QueriesEmbeddingsQuant, ContextWeight, ThreadCount);

private:
    THashMap<TString, TVector<TQuantizer>> Column2Quantizers;
    TVector<TEmbedding> QueriesEmbeddings;
    TVector<TVector<TEmbedding>> QueriesEmbeddingsQuant;
    float ContextWeight;
    size_t ThreadCount;
};
REGISTER_MAPPER(TCalcQuantizationMseWithQueriesMapper);

class TCalcQuantizationRankingMetricsWithQueriesMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TCalcQuantizationRankingMetricsWithQueriesMapper() = default;

    TCalcQuantizationRankingMetricsWithQueriesMapper(const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                                                     const TVector<TEmbedding>& queriesEmbeddings,
                                                     const TVector<TVector<TEmbedding>>& queriesEmbeddingsQuant,
                                                     float contextWeight,
                                                     size_t topSize,
                                                     const TVector<TString>& replyColumns,
                                                     size_t threadCount,
                                                     TString queryIdColumn)
        : Column2Quantizers(column2Quantizers)
        , QueriesEmbeddings(queriesEmbeddings)
        , QueriesEmbeddingsQuant(queriesEmbeddingsQuant)
        , ContextWeight(contextWeight)
        , TopSize(topSize)
        , ReplyColumns(replyColumns)
        , ThreadCount(threadCount)
        , QueryIdColumn(queryIdColumn)
    {
        Y_VERIFY(QueriesEmbeddings.size() == QueriesEmbeddingsQuant.size());
    }

    void Start(NYT::TTableWriter<NYT::TNode>*) override {
        NPar::LocalExecutor().RunAdditionalThreads(ThreadCount - 1);
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        const size_t embeddingDim = input->GetRow()["context_embedding"].AsString().length() / sizeof(float);
        TVector<TTopHashScorePairs> queriesTopPairs(QueriesEmbeddings.size());
        auto numQuantilesPairs = Column2Quantizers["context_embedding"].size() * Column2Quantizers["reply_embedding"].size();
        TVector<TVector<TTopHashScorePairs>> queriesTopPairsQuant(QueriesEmbeddings.size(), TVector<TTopHashScorePairs>(numQuantilesPairs));

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();

            THashMap<TString, TEmbedding> column2Embedding;
            THashMap<TString, TVector<TEmbedding>> column2EmbeddingsQuant;
            for (const auto& p : Column2Quantizers) {
                auto column = p.first;
                GetColumnEmbeddingsInRow(row, column, embeddingDim, p.second, &column2Embedding[column], &column2EmbeddingsQuant[column]);
            }

            auto replyHash = GetReplyHash(row, ReplyColumns);

            auto task = [&](size_t queryIdx) {
                const auto& queryEmbedding = QueriesEmbeddings[queryIdx];
                const auto& contextEmbedding = column2Embedding["context_embedding"];
                const auto& replyEmbedding = column2Embedding["reply_embedding"];
                float score = ContextWeight * CalcCosine(queryEmbedding, contextEmbedding)
                    + (1. - ContextWeight) * CalcCosine(queryEmbedding, replyEmbedding);
                UpdateQueryTopHashScorePairs(queriesTopPairs[queryIdx], {replyHash, score}, TopSize);
                size_t quantilePairIdx = 0;
                for (size_t contextQuantileIdx = 0; contextQuantileIdx < Column2Quantizers["context_embedding"].size(); ++contextQuantileIdx) {
                    const auto& queryEmbeddingQuant = QueriesEmbeddingsQuant[queryIdx][contextQuantileIdx];
                    const auto& contextEmbeddingQuant = column2EmbeddingsQuant["context_embedding"][contextQuantileIdx];
                    auto queryContextScoreQuant = ContextWeight * CalcCosine(queryEmbeddingQuant, contextEmbeddingQuant);
                    for (size_t replyQuantileIdx = 0; replyQuantileIdx < Column2Quantizers["reply_embedding"].size(); ++replyQuantileIdx) {
                        const auto& replyEmbeddingQuant = column2EmbeddingsQuant["reply_embedding"][replyQuantileIdx];
                        float scoreQuant = queryContextScoreQuant + (1. - ContextWeight) * CalcCosine(queryEmbeddingQuant, replyEmbeddingQuant);
                        UpdateQueryTopHashScorePairs(queriesTopPairsQuant[queryIdx][quantilePairIdx], {replyHash, scoreQuant}, TopSize);
                        ++quantilePairIdx;
                    }
                }
            };
            NPar::LocalExecutor().ExecRange(task, 0, QueriesEmbeddings.size(), NPar::TLocalExecutor::WAIT_COMPLETE);
        }

        for (size_t queryIdx = 0; queryIdx < QueriesEmbeddings.size(); ++queryIdx) {
            NYT::TNode result;
            result[QueryIdColumn] = queryIdx;
            for (; !queriesTopPairs[queryIdx].empty(); queriesTopPairs[queryIdx].pop()) {
                result["no_quantization"] = queriesTopPairs[queryIdx].top().ToString();
                size_t quantilePairIdx = 0;
                for (size_t i = 0; i < Column2Quantizers["context_embedding"].size(); ++i) {
                    auto contextQuantile = Column2Quantizers["context_embedding"][i].GetQuantile();
                    for (size_t j = 0; j < Column2Quantizers["reply_embedding"].size(); ++j) {
                        auto replyQuantile = Column2Quantizers["reply_embedding"][j].GetQuantile();
                        auto quantilePairStr = ToString(contextQuantile) + "," + ToString(replyQuantile);
                        result[quantilePairStr] = queriesTopPairsQuant[queryIdx][quantilePairIdx].top().ToString();
                        queriesTopPairsQuant[queryIdx][quantilePairIdx].pop();
                        ++quantilePairIdx;
                    }
                }
                output->AddRow(result);
            }
        }
    }

    Y_SAVELOAD_JOB(Column2Quantizers, QueriesEmbeddings, QueriesEmbeddingsQuant, ContextWeight, TopSize, ReplyColumns, ThreadCount, QueryIdColumn);

private:
    THashMap<TString, TVector<TQuantizer>> Column2Quantizers;
    TVector<TEmbedding> QueriesEmbeddings;
    TVector<TVector<TEmbedding>> QueriesEmbeddingsQuant;
    float ContextWeight;
    size_t TopSize;
    TVector<TString> ReplyColumns;
    size_t ThreadCount;
    TString QueryIdColumn;
};
REGISTER_MAPPER(TCalcQuantizationRankingMetricsWithQueriesMapper);

class TCalcQuantizationRankingMetricsWithQueriesReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TCalcQuantizationRankingMetricsWithQueriesReducer() = default;

    TCalcQuantizationRankingMetricsWithQueriesReducer(const TVector<size_t>& topSizes, TString queryIdColumn)
        : TopSizes(topSizes)
        , QueryIdColumn(queryIdColumn)
    {
        Sort(TopSizes.begin(), TopSizes.end());
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TTopHashScorePairs queriesTopPairs;
        THashMap<TString, TTopHashScorePairs> queriesTopPairsQuant;

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow().AsMap();
            for (const auto& p : row) {
                if (p.first == QueryIdColumn) {
                    continue;
                }
                ui64 hash;
                float score;
                StringSplitter(p.second.AsString()).Split(',').CollectInto(&hash, &score);
                if (p.first == "no_quantization") {
                    UpdateQueryTopHashScorePairs(queriesTopPairs, {hash, score}, TopSizes.back());
                } else {
                    UpdateQueryTopHashScorePairs(queriesTopPairsQuant[p.first], {hash, score}, TopSizes.back());
                }
            }
        }

        auto rankedHashes = TopHashScorePairs2RankedHashes(queriesTopPairs);

        for (auto& p : queriesTopPairsQuant) {
            TVector<TString> quantilesStr = StringSplitter(p.first).Split(',');
            NYT::TNode result;
            result["context_quantile"] = FromString<float>(quantilesStr[0]);
            result["reply_quantile"] = FromString<float>(quantilesStr[1]);

            auto rankedHashesQuant = TopHashScorePairs2RankedHashes(p.second);
            auto recallMetric = CalcRecall(rankedHashes, rankedHashesQuant);
            auto tirMetric = CalcTir(rankedHashes, rankedHashesQuant);

            for (auto topSize : TopSizes) {
                auto metricSuffix = ToString(topSize);
                result["recall" + metricSuffix] = recallMetric[topSize - 1];
                result["TIR" + metricSuffix] = tirMetric[topSize - 1];
            }

            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(TopSizes, QueryIdColumn);

private:
    TVector<size_t> TopSizes;
    TString QueryIdColumn;
};
REGISTER_REDUCER(TCalcQuantizationRankingMetricsWithQueriesReducer);

class TAggregateMetrics : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TAggregateMetrics() = default;

    TAggregateMetrics(size_t numEmbeddings)
        : NumEmbeddings(numEmbeddings)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        THashMap<TString, double> summary;
        auto result = input->GetRow();
        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow().AsMap();
            for (const auto& p : row) {
                if (!p.first.EndsWith("quantile")) {
                    summary[p.first] += p.second.AsDouble();
                }
            }
        }
        for (const auto& p : summary) {
            result[p.first] = p.second / NumEmbeddings;
        }
        output->AddRow(result);
    }

    Y_SAVELOAD_JOB(NumEmbeddings);

private:
    size_t NumEmbeddings;
};
REGISTER_REDUCER(TAggregateMetrics);

void CalcQuantizationMse(TIntrusivePtr<NYT::IClient> client,
                         TString inputTable,
                         TString outputTable,
                         const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                         const TVector<TString>& quantileColumns) {
    const size_t numEmbeddings = client->Get(inputTable + "/@row_count").AsInt64();
    ui64 memoryLimit = 1ULL << 30;

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .MapperSpec(NYT::TUserJobSpec().MemoryLimit(memoryLimit))
            .ReduceBy(quantileColumns),
        new TCalcQuantizationMseMapper(column2Quantizers),
        new TAggregateMetrics(numEmbeddings));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(outputTable)
            .Output(outputTable)
            .SortBy(quantileColumns));
}

void CalcQuantizationRankingMetricsWithQueries(TIntrusivePtr<NYT::IClient> client,
                                               TString inputTable,
                                               TString outputTable,
                                               TString queryTable,
                                               const THashMap<TString, TVector<TQuantizer>>& column2Quantizers,
                                               const TVector<size_t>& topSizes,
                                               float contextWeight,
                                               const TVector<TString>& replyColumns,
                                               size_t threadCount,
                                               const TVector<TString>& quantileColumns) {
    ui64 memoryLimit = 1ULL << 30;

    TVector<TEmbedding> queriesEmbs;
    TVector<TVector<TEmbedding>> queriesEmbsQuant;
    const TString queryColumn = "context_embedding";
    GetColumnEmbeddingsInTable(client, queryTable, queryColumn, column2Quantizers.at(queryColumn), &queriesEmbs, &queriesEmbsQuant);

    const size_t maxTopSize = *MaxElement(topSizes.begin(), topSizes.end());
    const TString queryIdColumn = "query_idx";
    memoryLimit += queriesEmbs.size() * queriesEmbs[0].Values.size() * (queriesEmbsQuant[0].size() + 1) * 10;

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .MapperSpec(NYT::TUserJobSpec().MemoryLimit(memoryLimit))
            .ReduceBy(queryIdColumn),
        new TCalcQuantizationRankingMetricsWithQueriesMapper(column2Quantizers, queriesEmbs, queriesEmbsQuant, contextWeight, maxTopSize, replyColumns, threadCount, queryIdColumn),
        new TCalcQuantizationRankingMetricsWithQueriesReducer(topSizes, queryIdColumn),
        NYT::TOperationOptions().Spec(NYT::TNode()
            ("mapper", NYT::TNode()("cpu_limit", threadCount))));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(outputTable)
            .Output(outputTable)
            .SortBy(quantileColumns));
    client->Reduce(
        NYT::TReduceOperationSpec()
            .AddInput<NYT::TNode>(outputTable)
            .AddOutput<NYT::TNode>(outputTable)
            .ReduceBy(quantileColumns),
        new TAggregateMetrics(queriesEmbs.size()));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(outputTable)
            .Output(outputTable)
            .SortBy(quantileColumns));
}
