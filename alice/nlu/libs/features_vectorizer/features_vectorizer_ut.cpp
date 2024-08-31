#include <alice/nlu/libs/features_vectorizer/features_vectorizer.h>
#include <library/cpp/testing/unittest/registar.h>
#include <util/stream/buffer.h>
#include <util/string/join.h>

namespace {
    TString GetVectorizerData(const TVector<TString>& denseFeaturesOrder,
                              const TVector<ui64>& denseFeaturesSizes,
                              const TVector<TString>& sparseFeatures,
                              const THashMap<TString, NAlice::TSparseFeatureId>& sparseMapping) {
        TVector<TString> vectorizerDataLines = {
            "dense features order:",
            JoinSeq(" ", denseFeaturesOrder),
            "dense features sizes:",
            JoinSeq(" ", denseFeaturesSizes),
            "sparse features:",
            JoinSeq(" ", sparseFeatures),
            "sparse mapping:"
        };
        for (const auto& [key, value] : sparseMapping) {
            vectorizerDataLines.push_back(JoinSeq(" ", {key, ToString(value)}));
        }
        const auto vectorizerData = JoinSeq("\n", vectorizerDataLines);

        return vectorizerData;
    }

    NAlice::TFeaturesVectorizer GetEmptyVectorizer() {
        return NAlice::TFeaturesVectorizer({}, {}, {}, {});
    }

    NAlice::TFeaturesVectorizer GetSimpleVectorizer() {
        return NAlice::TFeaturesVectorizer({"dense_1", "dense_2", "dense_3"},
                                           {2, 1, 3},
                                           {"sparse_1", "sparse_2"},
                                           {{"sparse_1=A", 0}, {"sparse_1=B", 1}, {"sparse_2=C", 2}, {"sparse_2=D", 3}});
    }

    NVins::TSampleFeatures GetEmptySample() {
        return NVins::TSampleFeatures();
    }

    NVins::TSampleFeatures GetSimpleSample() {
        NVins::TSampleFeatures sampleFeatures;
        sampleFeatures.Sample.Text = "sample text";
        sampleFeatures.Sample.Tokens = {"sample", "text"};
        sampleFeatures.DenseSeq["dense_1"] = {{1, 2}, {3, 4}};
        sampleFeatures.DenseSeq["dense_3"] = {{5, 6, 7}, {8, 9, 10}};
        sampleFeatures.SparseSeq["sparse_1"] = {
            {NVins::TSparseFeature("A"), NVins::TSparseFeature("B")}, // token 1
            {NVins::TSparseFeature("B")}                              // token 2
        };
        sampleFeatures.SparseSeq["sparse_2"] = {
            {},                                                      // token 1
            {NVins::TSparseFeature("D"), NVins::TSparseFeature("D")} // token 2
        };
        return sampleFeatures;
    }

}

Y_UNIT_TEST_SUITE(FeaturesVectorizerTestSuite) {

Y_UNIT_TEST(NoFeatures) {
    const auto vectorizer = GetEmptyVectorizer();
    NAlice::TVectorizedSeqFeatures vectorizedFeatures;

    vectorizer.Vectorize(GetEmptySample(), &vectorizedFeatures);
    UNIT_ASSERT_EQUAL(vectorizedFeatures, NAlice::TVectorizedSeqFeatures());

    vectorizer.Vectorize(GetSimpleSample(), &vectorizedFeatures);
    UNIT_ASSERT_EQUAL(vectorizedFeatures, NAlice::TVectorizedSeqFeatures(2));
}

Y_UNIT_TEST(Simple) {
    const auto vectorizer = GetSimpleVectorizer();
    NAlice::TVectorizedSeqFeatures vectorizedFeatures;

    vectorizer.Vectorize(GetEmptySample(), &vectorizedFeatures);
    UNIT_ASSERT_EQUAL(vectorizedFeatures, NAlice::TVectorizedSeqFeatures());

    vectorizer.Vectorize(GetSimpleSample(), &vectorizedFeatures);
    const NAlice::TVectorizedSeqFeatures expected = {
        {/*sparse*/1, 1, 0, 0, /*dense_1*/1, 2, /*dense_2*/0, /*dense_3*/5, 6, 7},
        {/*sparse*/0, 1, 0, 2, /*dense_1*/3, 4, /*dense_2*/0, /*dense_3*/8, 9, 10}
    };
    UNIT_ASSERT_EQUAL(vectorizedFeatures, expected);
}

Y_UNIT_TEST(VectorizerData) {
    const TVector<TString> denseFeaturesOrder = {"dense_1", "dense_2", "dense_3"};
    const TVector<ui64> denseFeaturesSizes = {2, 1, 3};
    const TVector<TString> sparseFeatures = {"sparse_1", "sparse_2"};
    const THashMap<TString, NAlice::TSparseFeatureId> sparseSeqFeaturesMapping = {
        {"sparse_1=A", 0}, {"sparse_1=B", 1}, {"sparse_2=C", 2}, {"sparse_2=D", 3}
    };

    const auto data = GetVectorizerData(denseFeaturesOrder, denseFeaturesSizes, sparseFeatures, sparseSeqFeaturesMapping);
    TStringInput in(data);

    TVector<TString> readDenseFeaturesOrder;
    TVector<ui64> readDenseFeaturesSizes;
    TVector<TString> readSparseFeatures;
    THashMap<TString, NAlice::TSparseFeatureId> readSparseSeqFeaturesMapping;
    NAlice::ReadFeaturesVectorizerData(&in,
                                       &readDenseFeaturesOrder,
                                       &readDenseFeaturesSizes,
                                       &readSparseFeatures,
                                       &readSparseSeqFeaturesMapping);

    UNIT_ASSERT_EQUAL(denseFeaturesOrder, readDenseFeaturesOrder);
    UNIT_ASSERT_EQUAL(denseFeaturesSizes, readDenseFeaturesSizes);
    UNIT_ASSERT_EQUAL(sparseFeatures, readSparseFeatures);
    UNIT_ASSERT_EQUAL(sparseSeqFeaturesMapping, readSparseSeqFeaturesMapping);
}

}
