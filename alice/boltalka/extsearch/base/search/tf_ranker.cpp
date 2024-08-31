#include "tf_ranker.h"

namespace NNlg {

TTfRanker::TTfRanker(const TString& modelDirName)
        : TTfNnModel(modelDirName)
        {
    SetInputNodes({"input.1"});
    SetOutputNodes({"add_4"});

    EstablishSessionIfNotYet();
}

void TTfRanker::Predict(const float* queryEmbedding, const TVector<const float*>& replyEmbeddings, size_t dimension, const TVector<double>& relevs, TVector<double>* scores, bool boostTop, bool boostRelev) {
    size_t batchSize = replyEmbeddings.size();
    if (batchSize < 1) {
        return;
    }
    double threshold = 0;
    if (boostTop) {
        TVector<double> sortedRelevs(relevs);
        const auto split = sortedRelevs.end() - Min(batchSize, 10UL);
        std::nth_element(sortedRelevs.begin(), split, sortedRelevs.end());
        threshold = *split;
    }
    NNeuralNet::TTensorList inputs = {tf::Tensor(tf::DT_FLOAT, {static_cast<long long>(batchSize), static_cast<long long>(dimension * 2 + boostRelev)})};
    auto&& input = inputs[0].matrix<float>();
    for (size_t i = 0; i < batchSize; ++i) {
        for (size_t j = 0; j < dimension; ++j) {
            input(i, j) = queryEmbedding[j];
            input(i, j + dimension) = replyEmbeddings[i][j];
        }
        if (boostTop) {
            input(i, 0) = relevs[i] > threshold;
        }
        if (boostRelev) {
            input(i, dimension * 2) = relevs[i];
        }
    }
    auto&& result = CreateWorker()->Process(inputs)[0].matrix<float>();
    scores->resize(relevs.size());
    for (size_t i = 0; i < scores->size(); ++i) {
        (*scores)[i] += result(i, 0);
    }
}

}
