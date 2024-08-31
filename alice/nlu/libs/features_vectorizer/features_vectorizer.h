#pragma once
#include <alice/nlu/libs/sample_features/sample_features.h>
#include <util/folder/path.h>
#include <util/generic/vector.h>
#include <util/generic/hash.h>

namespace NAlice {

    using TSparseFeatureId = ui64;
    using TVectorizedSeqFeatures = TVector<TVector<float>>;

    class TFeaturesVectorizer {
    public:
        explicit TFeaturesVectorizer(const TVector<TString>& denseFeaturesOrder,
                                     const TVector<ui64>& denseFeaturesSizes,
                                     const TVector<TString>& sparseFeatures,
                                     const THashMap<TString, TSparseFeatureId>& sparseSeqFeaturesMapping);

        void Vectorize(const NVins::TSampleFeatures& sampleFeatures, TVectorizedSeqFeatures* result, IOutputStream* log = nullptr) const;

    private:
        void AddSparseSeqFeatures(const NVins::TSampleFeatures& sampleFeatures, TVectorizedSeqFeatures* result) const;
        void AddDenseSeqFeatures(const NVins::TSampleFeatures& sampleFeatures, TVectorizedSeqFeatures* result, IOutputStream* log) const;

    private:
        TVector<TString> DenseFeaturesOrder;
        TVector<ui64> DenseFeaturesSizes;
        TVector<TString> SparseFeatures;
        THashMap<TString, TSparseFeatureId> SparseSeqFeaturesMapping;
    };


    void ReadFeaturesVectorizerData(IInputStream* input,
                                    TVector<TString>* denseFeaturesOrder,
                                    TVector<ui64>* denseFeaturesSizes,
                                    TVector<TString>* sparseFeatures,
                                    THashMap<TString, TSparseFeatureId>* sparseSeqFeaturesMapping);
    TFeaturesVectorizer CreateFeaturesVectorizerFromStream(IInputStream* input);
} // namespace NAlice
