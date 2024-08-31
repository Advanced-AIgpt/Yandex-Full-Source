#include "sample_features_converter.h"

namespace NBg {

    NVins::TSampleFeatures TSampleFeaturesConverter::ConvertSampleFeatures(
        const NProto::TAliceSampleFeaturesResult& sampleFeaturesResultProto
    ) const {
        NVins::TSampleFeatures sampleFeatures;
        const auto& sampleFeaturesProto = sampleFeaturesResultProto.Getsample_features();
        sampleFeatures.Sample = ConvertSample(sampleFeaturesProto.Getsample());
        for (const auto& [feature, value] : sampleFeaturesProto.Getdense_seq_features()) {
            sampleFeatures.DenseSeq[feature] = ConvertDenseSeq(value);
        }
        for (const auto& [feature, value] : sampleFeaturesProto.Getsparse_seq_features()) {
            sampleFeatures.SparseSeq[feature] = ConvertSparseSeq(value);
        }
        return sampleFeatures;
    }

    NVins::TSample TSampleFeaturesConverter::ConvertSample(const NProto::TAliceSample& sampleProto) const {
        NVins::TSample sample;
        sample.Text = sampleProto.Getutterance().Gettext();
        sample.Tokens.reserve(sampleProto.Gettokens().size());
        for (const auto& token : sampleProto.Gettokens()) {
            sample.Tokens.push_back(token);
        }
        return sample;
    }

    TVector<TVector<float>> TSampleFeaturesConverter::ConvertDenseSeq(const NProto::TAliceDenseSeqFeatures& denseSeqProto) const {
        TVector<TVector<float>> denseSeq;
        size_t shapeX = denseSeqProto.Getshape_x();
        if (shapeX == 0) {
            return denseSeq;
        }
        size_t shapeY = denseSeqProto.Getshape_y();
        denseSeq.reserve(shapeX);
        for (const auto& value : denseSeqProto.Getdata()) {
            if (denseSeq.size() == 0 || denseSeq.back().size() == shapeY) {
                denseSeq.emplace_back();
                denseSeq.back().reserve(shapeY);
            }
            denseSeq.back().push_back(value);
        }
        Y_ENSURE(denseSeq.size() == shapeX);
        Y_ENSURE(denseSeq.back().size() == shapeY);
        return denseSeq;
    }

    TVector<TVector<NVins::TSparseFeature>> TSampleFeaturesConverter::ConvertSparseSeq(
        const NProto::TAliceSparseSeqFeatures& sparseSeqProto
    ) const {
        TVector<TVector<NVins::TSparseFeature>> sparseSeq;
        sparseSeq.reserve(sparseSeqProto.Getdata().size());
        for (const auto& featuresForTokenProto : sparseSeqProto.Getdata()) {
            sparseSeq.emplace_back();
            for (const auto& feature : featuresForTokenProto.Getdata()) {
                sparseSeq.back().emplace_back(feature.Getvalue(), feature.Getweight());
            }
        }
        return sparseSeq;
    }

} // namespace NBg
