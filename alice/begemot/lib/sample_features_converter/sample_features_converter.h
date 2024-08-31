#pragma once

#include <alice/nlu/libs/sample_features/sample_features.h>
#include <search/begemot/core/rulecontext.h>
#include <search/begemot/rules/alice/sample_features/proto/alice_sample_features.pb.h>

namespace NBg {

    class TSampleFeaturesConverter {
    public:
        NVins::TSampleFeatures ConvertSampleFeatures(const NProto::TAliceSampleFeaturesResult& sampleFeaturesResultProto) const;

    private:
        NVins::TSample ConvertSample(const NProto::TAliceSample& sampleProto) const;
        TVector<TVector<float>> ConvertDenseSeq(const NProto::TAliceDenseSeqFeatures& denseSeqProto) const;
        TVector<TVector<NVins::TSparseFeature>> ConvertSparseSeq(const NProto::TAliceSparseSeqFeatures& sparseSeqProto) const;
    };

} // namespace NBg
