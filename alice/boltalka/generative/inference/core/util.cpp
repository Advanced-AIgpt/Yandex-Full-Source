#include "util.h"

TVector<TVector<float>> LoadPtuneFromNpz(const NDict::NMT::NYNMT::TModelProtoWithMeta* proto) {
    TVector<float> flat;
    size_t numTokens, embSize;
    if (proto->Consts.HasTensor<TFloat16>("ptune_embeddings")) {
        auto tensor = proto->Consts.GetTensor<TFloat16>("ptune_embeddings");
        numTokens = tensor.Shape[0];
        embSize = tensor.Shape[1];
        flat = TVector<float>(tensor.Data, tensor.Data + numTokens * embSize);
    } else {
        auto tensor = proto->Consts.GetTensor<float>("ptune_embeddings");
        numTokens = tensor.Shape[0];
        embSize = tensor.Shape[1];
        flat = TVector<float>(tensor.Data, tensor.Data + numTokens * embSize);
    }
    TVector<TVector<float>> embeddings;
    embeddings.reserve(numTokens);
    for (auto i = flat.begin(); i < flat.end(); i += embSize) {
        embeddings.emplace_back(i, i + embSize);
    }
    return embeddings;
}
