#include "lstm_relevance_computer.h"
#include "ut/mock.h"

#include <library/cpp/testing/unittest/env.h>
#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/algorithm.h>
#include <util/generic/hash.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>


using namespace testing;
using namespace NAlice;
using namespace NAlice::NItemSelector;


void TestCropping(const TString& request, const TVector<TString>& synonyms, const size_t maxTokens, const size_t modelInputLength) {
    static const THashMap<TString, TVector<float>> specialEmbeddings = {
        {"[CLF]", {}},
        {"[SEP]", {}}
    };

    TMockTokenEmbedder embedder;
    EXPECT_CALL(embedder, EmbedToken(_, _)).WillRepeatedly(Return(TEmbedding{}));

    THolder<const TMockNnComputer> innerComputer = MakeHolder<const TMockNnComputer>();
    const TVector<TVector<TVector<float>>> modelInput(1, TVector<TVector<float>>(modelInputLength));
    EXPECT_CALL(*innerComputer, Predict(TVector<int>{static_cast<int>(modelInputLength)}, modelInput))
        .Times(1)
        .WillRepeatedly(Return(0));

    TLSTMRelevanceComputer computer(
        static_cast<THolder<const NAlice::NItemSelector::INnComputer>>(std::move(innerComputer)),
        &embedder,
        specialEmbeddings,
        /* maxTokens = */ maxTokens
    );
    computer.ComputeRelevance(request, synonyms);
}

Y_UNIT_TEST_SUITE(LSTMRelevanceComputerTestSuite) {
    Y_UNIT_TEST(Cropping) {
        // no cropping is happening
        // мама мыла раму [CLF] рама мыла маму [SEP] мало [SEP]
        // 1    2    3    4     5    6    7    8     9    10    <- modelInputLength
        TestCropping("мама мыла раму", {"рама мыла маму", "мало"}, /* maxTokens = */ 16, /* modelInputLength = */ 10);

        // crop
        // мама мыла раму [CLF] рама мыла маму [SEP] мало [SEP] |->
        // мама мыла раму [CLF]
        // 1    2    3    4     <- modelInputLength
        TestCropping("мама мыла раму", {"рама мыла маму", "мало"}, /* maxTokens = */ 4, /* modelInputLength = */ 4);
    }
}
