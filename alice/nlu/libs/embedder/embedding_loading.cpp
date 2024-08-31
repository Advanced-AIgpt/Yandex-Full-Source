#include "embedding_loading.h"

#include <library/cpp/json/json_reader.h>

#include <util/stream/input.h>

namespace NAlice {

    THashMap<TString, TEmbedding> LoadEmbeddingsFromJson(IInputStream* input) {
        Y_ASSERT(input);

        NJson::TJsonValue jsonEmbeddingsMap;
        const bool readCorrectly = NJson::ReadJsonTree(input, &jsonEmbeddingsMap);
        Y_ENSURE(readCorrectly);

        THashMap<TString, TEmbedding> embeddings;
        const auto& embeddingsMap = jsonEmbeddingsMap.GetMapSafe();
        for (const auto& [token, jsonEmbedding] : embeddingsMap) {
            auto& embedding = embeddings[token];

            const auto& embeddingArray = jsonEmbedding.GetArraySafe();
            for (const auto& jsonValue : embeddingArray) {
                embedding.push_back(jsonValue.GetDoubleSafe());
            }
        }

        return embeddings;
    }

} // namespace NAlice
