#include "features_vectorizer.h"

#include <util/stream/file.h>
#include <util/string/split.h>
#include <util/ysaveload.h>

namespace NAlice {
    namespace {
        void SkipLine(IInputStream* input, const TString& expectedLine) {
            Y_ASSERT(input);
            const TString line = input->ReadLine();
            Y_ASSERT(line == expectedLine);
        }

        bool HasProperDimensions(const TVector<TVector<float>>& denseFeature,
                                 const size_t numTokens,
                                 const size_t vectorSize) {
            if (denseFeature.size() != numTokens) {
                return false;
            }
            for (const auto& featureValueForToken : denseFeature) {
                if (featureValueForToken.size() != vectorSize) {
                    return false;
                }
            }
            return true;
        }
    } // namespace anonymous

    void ReadFeaturesVectorizerData(IInputStream* input,
                                    TVector<TString>* denseFeaturesOrder,
                                    TVector<ui64>* denseFeaturesSizes,
                                    TVector<TString>* sparseFeatures,
                                    THashMap<TString, TSparseFeatureId>* sparseSeqFeaturesMapping) {
        Y_ASSERT(input);
        Y_ASSERT(denseFeaturesOrder);
        Y_ASSERT(denseFeaturesSizes);
        Y_ASSERT(sparseFeatures);
        Y_ASSERT(sparseSeqFeaturesMapping);

        SkipLine(input, "dense features order:");
        const TString denseFeaturesOrderLine = input->ReadLine();
        *denseFeaturesOrder = StringSplitter(denseFeaturesOrderLine).Split(' ').SkipEmpty();

        SkipLine(input, "dense features sizes:");
        const TString denseFeaturesSizesLine = input->ReadLine();
        denseFeaturesSizes->reserve(denseFeaturesOrder->size());
        StringSplitter(denseFeaturesSizesLine).Split(' ').SkipEmpty().ParseInto(denseFeaturesSizes);
        Y_VERIFY(denseFeaturesOrder->size() == denseFeaturesSizes->size());

        SkipLine(input, "sparse features:");
        const TString sparseFeaturesLine = input->ReadLine();
        *sparseFeatures = StringSplitter(sparseFeaturesLine).Split(' ').SkipEmpty();

        SkipLine(input, "sparse mapping:");
        TString mappingElementLine;
        while (input->ReadLine(mappingElementLine)) {
            TString feature;
            TSparseFeatureId id;
            StringSplitter(mappingElementLine).Split(' ').CollectInto(&feature, &id);
            (*sparseSeqFeaturesMapping)[feature] = id;
        }
    }

    TFeaturesVectorizer CreateFeaturesVectorizerFromStream(IInputStream* input) {
        Y_ASSERT(input);

        TVector<TString> denseFeaturesOrder;
        TVector<ui64> denseFeaturesSizes;
        TVector<TString> sparseFeatures;
        THashMap<TString, TSparseFeatureId> sparseSeqFeaturesMapping;

        ReadFeaturesVectorizerData(input, &denseFeaturesOrder, &denseFeaturesSizes, &sparseFeatures, &sparseSeqFeaturesMapping);
        return TFeaturesVectorizer(denseFeaturesOrder, denseFeaturesSizes, sparseFeatures, sparseSeqFeaturesMapping);
    }

    TFeaturesVectorizer::TFeaturesVectorizer(const TVector<TString>& denseFeaturesOrder,
                                             const TVector<ui64>& denseFeaturesSizes,
                                             const TVector<TString>& sparseFeatures,
                                             const THashMap<TString, TSparseFeatureId>& sparseSeqFeaturesMapping)
        : DenseFeaturesOrder(denseFeaturesOrder)
        , DenseFeaturesSizes(denseFeaturesSizes)
        , SparseFeatures(sparseFeatures)
        , SparseSeqFeaturesMapping(sparseSeqFeaturesMapping) {
    }


    void TFeaturesVectorizer::Vectorize(const NVins::TSampleFeatures& sampleFeatures,
                                        TVectorizedSeqFeatures* result,
                                        IOutputStream* log) const {
        Y_ASSERT(result);

        size_t totalVectorSize = SparseSeqFeaturesMapping.size();
        for (auto denseFeatureSize : DenseFeaturesSizes) {
            totalVectorSize += denseFeatureSize;
        }

        result->resize(sampleFeatures.Sample.Tokens.size());
        for (auto& featuresForToken : *result) {
            featuresForToken.clear();
            featuresForToken.reserve(totalVectorSize);
        }

        AddSparseSeqFeatures(sampleFeatures, result);
        AddDenseSeqFeatures(sampleFeatures, result, log);
    }

    void TFeaturesVectorizer::AddSparseSeqFeatures(const NVins::TSampleFeatures& sampleFeatures,
                                                   TVectorizedSeqFeatures* result) const {
        Y_ASSERT(result);

        for (auto& vec : *result) {
            vec.assign(SparseSeqFeaturesMapping.size(), 0);
        }
        for (const auto& featureName : SparseFeatures) {
            if (!sampleFeatures.SparseSeq.contains(featureName)) {
                continue;
            }
            const auto& valuesForTokens = sampleFeatures.SparseSeq.at(featureName);
            for (size_t tokenId = 0; tokenId < valuesForTokens.size(); ++tokenId) {
                for (const auto& value : valuesForTokens[tokenId]) {
                    const auto mappingKey = featureName + "=" + value.Value;
                    if (!SparseSeqFeaturesMapping.contains(mappingKey)) {
                        continue;
                    }
                    const auto featureId = SparseSeqFeaturesMapping.at(mappingKey);
                    (*result)[tokenId][featureId] += 1.0;
                }
            }
        }
    }

    void TFeaturesVectorizer::AddDenseSeqFeatures(const NVins::TSampleFeatures& sampleFeatures,
                                                  TVectorizedSeqFeatures* result,
                                                  IOutputStream* log) const {
        Y_ASSERT(result);

        for (size_t denseFeatureId = 0; denseFeatureId < DenseFeaturesOrder.size(); ++denseFeatureId) {
            const auto& featureName = DenseFeaturesOrder[denseFeatureId];
            const auto& vectorSize = DenseFeaturesSizes[denseFeatureId];

            const auto featureValue = sampleFeatures.DenseSeq.find(featureName);
            const bool isValidFeatureValue = (featureValue != sampleFeatures.DenseSeq.end()) &&
                                             HasProperDimensions(featureValue->second, result->size(), vectorSize);
            if (log != nullptr && !isValidFeatureValue) {
                *log << "WARNING: dense feature " << featureName << " has improper dimensions." << Endl;
            }

            for (size_t tokenId = 0; tokenId < result->size(); ++tokenId) {
                for (size_t valuePos = 0; valuePos < vectorSize; ++valuePos) {
                    (*result)[tokenId].push_back(isValidFeatureValue ? featureValue->second[tokenId][valuePos] : 0.0);
                }
            }
        }
    }
} // namespace NAlice
