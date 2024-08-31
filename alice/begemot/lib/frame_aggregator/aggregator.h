#pragma once

#include <alice/begemot/lib/frame_aggregator/proto/config.pb.h>
#include <alice/begemot/lib/fresh_options/proto/fresh_options.pb.h>
#include <alice/library/frame/description.h>
#include <alice/megamind/protos/common/frame.pb.h>

#include <search/begemot/rules/alice/action_recognizer/proto/alice_action_recognizer.pb.h>
#include <search/begemot/rules/alice/binary_intent_classifier/proto/alice_binary_intent_classifier.pb.h>
#include <search/begemot/rules/alice/entity_recognizer/proto/alice_entity_recognizer.pb.h>
#include <search/begemot/rules/alice/fixlist/proto/alice_fixlist.pb.h>
#include <search/begemot/rules/alice/frame_filler/proto/alice_frame_filler.pb.h>
#include <search/begemot/rules/alice/multi_intent_classifier/proto/alice_multi_intent_classifier.pb.h>
#include <search/begemot/rules/alice/parsed_frames/proto/alice_parsed_frames.pb.h>
#include <search/begemot/rules/alice/tagger/proto/alice_tagger.pb.h>
#include <search/begemot/rules/alice/trivial_tagger/proto/alice_trivial_tagger.pb.h>
#include <search/begemot/rules/alice/word_nn/scenarios/proto/scenarios.pb.h>
#include <search/begemot/rules/alice/word_nn/toloka/proto/toloka.pb.h>
#include <search/begemot/rules/granet/proto/granet.pb.h>
#include <search/begemot/rules/wiz_detection/proto/wiz_detection.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/maybe.h>

namespace NAlice {

namespace NImpl {

THashMap<TString, double> GetOverriddenThresholds(const THashSet<TString>& experiments);

} // namespace NImpl

struct TFrameAggregatorSources {
    const NBg::NProto::TAliceActionRecognizerResult* AliceActionRecognizer = nullptr;
    const NBg::NProto::TAliceBinaryIntentClassifierResult* AliceBinaryIntentClassifier = nullptr;
    const NBg::NProto::TAliceEntityRecognizerResult* AliceEntityRecognizer = nullptr;
    const NBg::NProto::TAliceFixlistResult* AliceFixlist = nullptr;
    const NBg::NProto::TAliceFrameFillerResult* AliceFrameFiller = nullptr;
    const NBg::NProto::TAliceMultiIntentClassifierResult* AliceMultiIntentClassifier = nullptr;
    const NBg::NProto::TAliceScenariosWordLstmResult* AliceScenariosWordLstm = nullptr;
    const NBg::NProto::TAliceTolokaWordLstmResult* AliceTolokaWordLstm = nullptr;
    const NBg::NProto::TAliceTaggerResult* AliceTagger = nullptr;
    const NBg::NProto::TAliceTrivialTaggerResult* AliceTrivialTagger = nullptr;
    const NBg::NProto::TAliceWizDetectionResult* AliceWizDetection = nullptr;
    const NBg::NProto::TGranetResult* Granet = nullptr;
};

class TFrameAggregator {
public:
    TFrameAggregator(
        const TFrameAggregatorConfig* config,
        const THashSet<TString>& experiments,
        const TFrameDescriptionMap* frameDescriptionMap,
        const TFrameAggregatorSources& sources);

    void Process(NBg::NProto::TAliceParsedFramesResult* result, THashSet<TString>* processedFrames) const;

private:
    bool GetClassifierResult(const TString& frameName, const TFrameAggregatorConfig::TClassifierRule& classifierRule,
        float* confidence) const;
    TMaybe<TSemanticFrame> GetSemanticFrame(const TString& frameName, const TFrameAggregatorConfig::TTaggerRule& taggerRule) const;
    const TFrameDescription* FindFrameDescription(const TString& intent) const;

private:
    const TFrameAggregatorConfig& Config;
    const THashSet<TString>& Experiments;
    const TFrameDescriptionMap* FrameDescriptionMap = nullptr;
    const TFrameAggregatorSources Sources;

    THashMap<TString, const TSemanticFrame*> AliceActionRecognizerFrames;
    THashMap<TString, const TSemanticFrame*> AliceEntityRecognizerGranetFrames;
    THashMap<TString, const TSemanticFrame*> AliceEntityRecognizerTaggerFrames;
    THashMap<TString, const TSemanticFrame*> AliceFrameFillerFrames;
    THashMap<TString, const TSemanticFrame*> AliceTaggerFrames;
    THashMap<TString, const TSemanticFrame*> AliceTrivialTaggerFrames;
    THashMap<TString, const NBg::NProto::TModelResult*> AliceWizDetectionModelResults;
    THashMap<TString, const NBg::NProto::TGranetForm*> GranetForms;
};

} // namespace NAlice
