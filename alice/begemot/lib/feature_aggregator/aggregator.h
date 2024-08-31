#pragma once

#include "config.h"


#include <alice/begemot/lib/fresh_options/fresh_options.h>
#include <alice/protos/api/nlu/feature_container.pb.h>

#include <search/begemot/rules/alice/entities_collector/proto/alice_entities_collector.pb.h>
#include <search/begemot/rules/alice/gc_dssm_classifier/proto/alice_gc_dssm_classifier.pb.h>
#include <search/begemot/rules/alice/multi_intent_classifier/proto/alice_multi_intent_classifier.pb.h>
#include <search/begemot/rules/alice/parsed_frames/proto/alice_parsed_frames.pb.h>
#include <search/begemot/rules/porn_query/protos/result.pb.h>
#include <search/begemot/rules/wiz_detection/proto/wiz_detection.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice::NFeatureAggregator {

struct TFeatureAggregatorSources {
    const NBg::NProto::TAliceEntitiesCollectorResult* AliceEntities = nullptr;
    const NBg::NProto::TAliceGcDssmClassifierResult* AliceGcDssmClassifier = nullptr;
    const NBg::NProto::TAliceMultiIntentClassifierResult* AliceMultiIntentClassifier = nullptr;
    const NBg::NProto::TAliceParsedFramesResult* AliceParsedFrames = nullptr;
    const NBg::NProto::TAliceWizDetectionResult* AliceWizDetection = nullptr;
    const NBg::NProto::TPornQueryResult* PornQuery = nullptr;
};

struct TFeatureAggregatorResult {
    TFeatureContainer Features;
    bool UsedEntireFresh = false;
    TVector<TString> UsedFreshForFeatures;
};

class TFeatureAggregator {
public:
    explicit TFeatureAggregator(const TFeatureAggregatorConfig& staticConfig);

    TFeatureAggregator(const TFeatureAggregatorConfig& staticConfig, 
                       const TFeatureAggregatorConfig& freshConfig);


    [[nodiscard]] TFeatureAggregatorResult Aggregate(const TFeatureAggregatorSources& sources,
                                              const THashSet<TString>& requestExperiments,
                                              const TFreshOptions& options) const;

private:
    TFeatureAggregatorConfig StaticConfig;
    TMaybe<TFeatureAggregatorConfig> FreshConfig;
};

} // namespace NAlice::NFeatureAggregator
