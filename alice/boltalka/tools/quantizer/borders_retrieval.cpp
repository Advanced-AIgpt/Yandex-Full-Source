#include "borders_retrieval.h"

#include <mapreduce/yt/interface/client.h>

#include <util/generic/vector.h>
#include <util/generic/algorithm.h>
#include <util/generic/ymath.h>
#include <util/generic/set.h>
#include <util/string/vector.h>
#include <util/generic/queue.h>

namespace {

template <class Comparator>
using TTopElements = TPriorityQueue<float, TVector<float>, Comparator>;

template <class Comparator>
static void UpdateHeapWithArray(TTopElements<Comparator>& heap, ui64 topSize, const float* ptr, size_t len) {
    auto compare = Comparator();
    for (size_t i = 0; i < len; ++i) {
        auto newElement = ptr[i];
        if (heap.size() < topSize || compare(newElement, heap.top())) {
            heap.emplace(newElement);
            if (heap.size() > topSize) {
                heap.pop();
            }
        }
    }
}

template <class Comparator>
static TVector<float> GetHeapOrderStatistics(TTopElements<Comparator>& heap, const TVector<ui64>& topSizes) {
    TVector<float> elements;
    for (size_t topSizeIdx = 0; topSizeIdx < topSizes.size(); ++topSizeIdx) {
        while (heap.size() > topSizes[topSizeIdx]) {
            heap.pop();
        }
        elements.push_back(heap.top());
    }
    return elements;
}

}

class TBordersRetrieverMapper : public NYT::IMapper<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TBordersRetrieverMapper() = default;

    TBordersRetrieverMapper(size_t numEmbeddings, size_t embeddingDim, const TColumnToQuantiles& column2Quantiles)
        : EmbeddingDim(embeddingDim)
    {
        const ui64 numElements = static_cast<ui64>(numEmbeddings) * embeddingDim;
        for (const auto& p : column2Quantiles) {
            auto maxQuantile = *MaxElement(p.second.begin(), p.second.end());
            ui64 topSize = Max(1UL, static_cast<ui64>(maxQuantile * numElements));
            Column2TopSize.emplace(p.first, topSize);
        }
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        THashMap<TString, TTopElements<TGreater<float>>> column2MaxElements;
        THashMap<TString, TTopElements<TLess<float>>> column2MinElements;

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            for (const auto& p : Column2TopSize) {
                auto column = p.first;
                auto topSize = p.second;
                const auto* embeddingPtr = reinterpret_cast<const float*>(row[column].AsString().data());
                UpdateHeapWithArray(column2MaxElements[column], topSize, embeddingPtr, EmbeddingDim);
                UpdateHeapWithArray(column2MinElements[column], topSize, embeddingPtr, EmbeddingDim);
            }
        }

        for (const auto& p : Column2TopSize) {
            auto column = p.first;
            NYT::TNode result;
            result["column"] = column;

            while (!column2MaxElements[column].empty()) {
                result["max"] = column2MaxElements[column].top();
                result["min"] = column2MinElements[column].top();
                output->AddRow(result);
                column2MaxElements[column].pop();
                column2MinElements[column].pop();
            }
        }
    }

    Y_SAVELOAD_JOB(EmbeddingDim, Column2TopSize);

private:
    size_t EmbeddingDim;
    THashMap<TString, ui64> Column2TopSize;
};
REGISTER_MAPPER(TBordersRetrieverMapper);

class TBordersRetrieverReducer : public NYT::IReducer<NYT::TTableReader<NYT::TNode>, NYT::TTableWriter<NYT::TNode>> {
public:
    TBordersRetrieverReducer() = default;

    TBordersRetrieverReducer(size_t numEmbeddings, size_t embeddingDim, const TColumnToQuantiles& column2Quantiles)
        : NumElements(static_cast<ui64>(numEmbeddings) * embeddingDim)
        , Column2Quantiles(column2Quantiles)
    {
    }

    void Do(NYT::TTableReader<NYT::TNode>* input, NYT::TTableWriter<NYT::TNode>* output) override {
        TTopElements<TGreater<float>> maxElements;
        TTopElements<TLess<float>> minElements;

        const auto column = input->GetRow()["column"].AsString();
        auto quantiles = Column2Quantiles[column];
        Sort(quantiles.begin(), quantiles.end(), TGreater<float>());

        TVector<ui64> topSizes;
        for (auto q : quantiles) {
            topSizes.push_back(Max(1UL, static_cast<ui64>(q * NumElements)));
        }

        for (; input->IsValid(); input->Next()) {
            const auto& row = input->GetRow();
            float maxVal = row["max"].AsDouble();
            float minVal = row["min"].AsDouble();
            UpdateHeapWithArray(maxElements, topSizes[0], &maxVal, 1);
            UpdateHeapWithArray(minElements, topSizes[0], &minVal, 1);
        }

        TVector<float> leftBorders = GetHeapOrderStatistics(minElements, topSizes);
        TVector<float> rightBorders = GetHeapOrderStatistics(maxElements, topSizes);
        NYT::TNode result;
        result["column"] = column;
        for (size_t i = 0; i < quantiles.size(); ++i) {
            result["quantile"] = quantiles[i];
            result["left_border"] = leftBorders[i];
            result["right_border"] = rightBorders[i];
            output->AddRow(result);
        }
    }

    Y_SAVELOAD_JOB(NumElements, Column2Quantiles);

private:
    ui64 NumElements;
    TColumnToQuantiles Column2Quantiles;
};
REGISTER_REDUCER(TBordersRetrieverReducer);

void CalcEmbeddingsBorders(TIntrusivePtr<NYT::IClient> client,
                           TString inputTable,
                           TColumnToQuantiles* column2Quantiles,
                           TString bordersTable) {
    auto reader = client->CreateTableReader<NYT::TNode>(inputTable);
    const size_t embeddingDim = reader->GetRow()[column2Quantiles->begin()->first].AsString().length() / sizeof(float);
    const size_t numEmbeddings = client->Get(inputTable + "/@row_count").AsInt64();

    client->MapReduce(
        NYT::TMapReduceOperationSpec()
            .AddInput<NYT::TNode>(inputTable)
            .AddOutput<NYT::TNode>(bordersTable)
            .ReduceBy("column"),
        new TBordersRetrieverMapper(numEmbeddings, embeddingDim, *column2Quantiles),
        new TBordersRetrieverReducer(numEmbeddings, embeddingDim, *column2Quantiles));
    client->Sort(
        NYT::TSortOperationSpec()
            .AddInput(bordersTable)
            .Output(bordersTable)
            .SortBy({"column", "quantile"}));
}

TColumnToBorders ReadEmbeddingsBorders(TIntrusivePtr<NYT::IClient> client,
                                       TColumnToQuantiles* column2Quantiles,
                                       TString bordersTable) {
    auto reader = client->CreateTableReader<NYT::TNode>(bordersTable);
    (*column2Quantiles).clear();
    TColumnToBorders column2Borders;
    for (; reader->IsValid(); reader->Next()) {
        const auto& row = reader->GetRow();
        auto column = row["column"].AsString();
        auto quantile = row["quantile"].AsDouble();
        (*column2Quantiles)[column].emplace_back(quantile);
        auto leftBorder = row["left_border"].AsDouble();
        auto rightBorder = row["right_border"].AsDouble();
        column2Borders[column].emplace_back(leftBorder, rightBorder);
    }
    return column2Borders;
}
