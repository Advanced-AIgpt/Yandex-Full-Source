#pragma once

#include "lstm_relevance_computer.h"

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice {

class TMockTokenEmbedder : public ITokenEmbedder {
public:
    MOCK_METHOD(TEmbedding, EmbedToken, (const TString& token, const TMaybe<TArrayRef<const float>>& defaultEmbedding)
    , (const, override));
};

namespace NItemSelector {

class TMockNnComputer : public INnComputer {
public:
    MOCK_METHOD(float, Predict, (const TVector<int>& lengths, const TVector<TVector<TVector<float>>>& embeddings), (const, override));
};

} // namespace NItemSelector
} // namespace NAlice
