#include <alice/beggins/internal/bert_tf/preparer.h>

#include <library/cpp/testing/common/env.h>
#include <library/cpp/testing/gtest/gtest.h>

#include <util/generic/xrange.h>

const TString dataPath = BuildRoot() + "/alice/beggins/internal/bert_tf/ut/data/";
const TString startTriePath = dataPath + "start.trie";
const TString continuationTriePath = dataPath + "cont.trie";
const TString vocabFilePath = dataPath + "vocab.txt";
const TString graphDefPath = dataPath + "model.pb";

using namespace NAlice::NBeggins::NBertTfApplier;

const TVector<TUtf32String> knownWordsTokens = {CLS, U"сколько", U"стоит", U"новый", U"чайник", SEP};
const TVector<TUtf32String> unknownWordsTokens = {CLS,    U"сколько", U"стоит", U"нове", U"##нькая",
                                                  U"308", U"##0t",    U"##i",   SEP};
const TVector<TUtf32String> unknownTokens = {CLS, U"ацунпнпнпваор", SEP};
const TVector<TUtf32String> trulyUnknownTokens = {CLS, U"[UNK]", SEP};
const TVector tokens = {knownWordsTokens, unknownWordsTokens, unknownTokens, trulyUnknownTokens};

class TBertTfPreparerTest : public testing::Test {
protected:
    void SetUp() override {
        const TBatchPreparer preparer(TBertDict::FromFile(vocabFilePath), 40);
        Batch = preparer.Prepare(tokens);
    }

    auto GetTensorMaps() {
        return TVector{Batch.InputIds.matrix<int>(), Batch.InputMask.matrix<int>(), Batch.SegmentIds.matrix<int>()};
    }

    TBatchPreparer::TPreparedTensors Batch;
};

TEST_F(TBertTfPreparerTest, TestShapes) {
    for (const auto& tensor : {Batch.InputIds, Batch.InputMask, Batch.SegmentIds}) {
        ASSERT_EQ(tensor.dims(), 2);
        ASSERT_EQ(tensor.dim_size(0), (int)tokens.size());
        ASSERT_EQ(tensor.dim_size(1), 40);
    }
}

TEST_F(TBertTfPreparerTest, TestUnknowns) {
    const TVector tensorMaps = GetTensorMaps();
    for (const auto tokenIdx : xrange(40u)) {
        for (const auto tensorIdx : xrange(3u)) {
            ASSERT_EQ(tensorMaps[tensorIdx](2, tokenIdx), tensorMaps[tensorIdx](3, tokenIdx));
        }
    }
}

TEST_F(TBertTfPreparerTest, TestPadding) {
    const TVector tensorMaps = GetTensorMaps();
    for (const auto sampleIdx : xrange(3)) {
        for (const auto tokenIdx : xrange(40u)) {
            ASSERT_EQ((bool)tensorMaps[0](sampleIdx, tokenIdx), tokenIdx < tokens[sampleIdx].size());
            ASSERT_EQ((bool)tensorMaps[1](sampleIdx, tokenIdx), tokenIdx < tokens[sampleIdx].size());
            ASSERT_EQ(tensorMaps[2](sampleIdx, tokenIdx), 0);
        }
    }
}
