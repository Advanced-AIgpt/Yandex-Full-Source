#include "aggregator.h"

#include <alice/begemot/lib/api/experiments/flags.h>
#include <alice/begemot/lib/frame_aggregator/config.h>
#include <alice/begemot/lib/utils/config.h>
#include <alice/begemot/lib/utils/form_to_frame.h>

#include <library/cpp/iterator/enumerate.h>
#include <util/generic/algorithm.h>
#include <util/generic/yexception.h>

namespace NAlice {

namespace {

    const float DEFAULT_CONFIDENCE = 1.F;

    THashMap<TString, const TSemanticFrame*> MakeFrameMap(
        const ::google::protobuf::RepeatedPtrField<TSemanticFrame>& frames)
    {
        THashMap<TString, const TSemanticFrame*> result;
        for (const TSemanticFrame& frame : frames) {
            result[frame.GetName()] = &frame;
        }
        return result;
    }

    template<class TValue>
    const TValue* FindValue(const ::google::protobuf::Map<TProtoStringType, TValue>& map, const TProtoStringType& key) {
        const auto it = map.find(key);
        if (it == map.end()) {
            return nullptr;
        }
        return &it->second;
    }

    template<class TMapValue, class TResultValue>
    bool FindValue(const ::google::protobuf::Map<TProtoStringType, TMapValue>& map,
        const TProtoStringType& key, TResultValue* result)
    {
        Y_ENSURE(result);
        const auto it = map.find(key);
        if (it == map.end()) {
            return false;
        }
        *result = it->second;
        return true;
    }

} // namespace

namespace NImpl {

    THashMap<TString, double> GetOverriddenThresholds(const THashSet<TString>& experiments) {
        THashMap<TString, double> overrides;
        for (const auto& experiment : experiments) {
            TStringBuf payload;
            if (TStringBuf{experiment}.AfterPrefix(EXP_FRAMES_OVERRIDE_RULE_THRESHOLD_PREFIX, payload)) {
                TVector<TStringBuf> parts;
                Split(payload, ":", parts);
                if (parts.size() != 2) {
                    continue;
                }
                double threshold;
                if (TryFromString<double>(parts[1], threshold)) {
                    overrides[parts[0]] = threshold;
                }
            }
        }
        return overrides;
    }

} // namespace NImpl

TFrameAggregator::TFrameAggregator(
        const TFrameAggregatorConfig* config,
        const THashSet<TString>& experiments,
        const TFrameDescriptionMap* frameDescriptionMap,
        const TFrameAggregatorSources& sources)
    : Config(*config)
    , Experiments(experiments)
    , FrameDescriptionMap(frameDescriptionMap)
    , Sources(sources)
{
    Y_ENSURE(config);

    if (Sources.Granet != nullptr) {
        for (const NBg::NProto::TGranetForm& form : Sources.Granet->GetForms()) {
            GranetForms[form.GetName()] = &form;
        }
    }
    if (Sources.AliceActionRecognizer != nullptr) {
        AliceActionRecognizerFrames = MakeFrameMap(Sources.AliceActionRecognizer->GetFrames());
    }
    if (Sources.AliceEntityRecognizer != nullptr) {
        AliceEntityRecognizerGranetFrames = MakeFrameMap(Sources.AliceEntityRecognizer->GetGranetFrames());
        AliceEntityRecognizerTaggerFrames = MakeFrameMap(Sources.AliceEntityRecognizer->GetTaggerFrames());
    }
    if (Sources.AliceFrameFiller != nullptr) {
        AliceFrameFillerFrames = MakeFrameMap(Sources.AliceFrameFiller->GetFrames());
    }
    if (Sources.AliceWizDetection != nullptr) {
        for (const NBg::NProto::TModelResult& modelResult : Sources.AliceWizDetection->GetModelResults()) {
            AliceWizDetectionModelResults[modelResult.GetName()] = &modelResult;
        }
    }
    if (Sources.AliceTagger != nullptr) {
        AliceTaggerFrames = MakeFrameMap(Sources.AliceTagger->GetFrames());
    }
    if (Sources.AliceTrivialTagger != nullptr) {
        AliceTrivialTaggerFrames = MakeFrameMap(Sources.AliceTrivialTagger->GetFrames());
    }
}

void TFrameAggregator::Process(NBg::NProto::TAliceParsedFramesResult* result, THashSet<TString>* processedFrames) const {
    Y_ENSURE(result);
    Y_ENSURE(processedFrames);

    const auto overriddenThresholds = NImpl::GetOverriddenThresholds(Experiments);

    for (const TFrameAggregatorConfig::TFrame& frameConfig : Config.GetFrames()) {
        if (!IsEnabledByExperiments(frameConfig.GetExperiments(), Experiments)) {
            continue;
        }
        const TString& frameName = frameConfig.GetName();
        const auto [_, shouldProcess] = processedFrames->insert(frameName);
        if (!shouldProcess) {
            continue;
        }

        for (const TFrameAggregatorConfig::TRule& ruleConfig : frameConfig.GetRules()) {
            const TFrameAggregatorConfig::TClassifierRule& classifierRule = ruleConfig.GetClassifier();

            float confidence = DEFAULT_CONFIDENCE;
            bool classifierResult = GetClassifierResult(frameName, classifierRule, &confidence);

            if (confidence != DEFAULT_CONFIDENCE && classifierRule.HasLoggingThreshold() && confidence > classifierRule.GetLoggingThreshold()) {
                auto& triggeredClassifierByLoggingThreshold = *result->MutableDebugInfo()->AddTriggeredClassifiersByLoggingThreshold();
                triggeredClassifierByLoggingThreshold.SetScore(confidence);
                triggeredClassifierByLoggingThreshold.SetFrameName(frameName);
                triggeredClassifierByLoggingThreshold.MutableClassifier()->CopyFrom(classifierRule);
            }

            if (const auto* overriddenThreshold = overriddenThresholds.FindPtr(classifierRule.GetAnchor())) {
                classifierResult = confidence > *overriddenThreshold;
            }
            if (!classifierResult || !IsEnabledByExperiments(ruleConfig.GetExperiments(), Experiments)) {
                continue;
            }

            auto& matchedRule = *result->MutableDebugInfo()->AddMatchedRules();
            matchedRule.SetFrameName(frameName);
            matchedRule.MutableRule()->CopyFrom(ruleConfig);

            if (classifierRule.GetIsNegative()) {
                break;
            }
            if (classifierRule.GetConfidence() != 0) {
                confidence = classifierRule.GetConfidence();
            }

            const TFrameAggregatorConfig::TTaggerRule& taggerRule = ruleConfig.GetTagger();
            TMaybe<TSemanticFrame> frame = GetSemanticFrame(frameName, taggerRule);
            if (!frame.Defined()) {
                continue;
            }
            frame->SetName(frameName);

            *result->AddFrames() = std::move(*frame);
            result->AddSources(classifierRule.GetSource());
            result->AddConfidences(confidence);
            break;
        }
    }
}

bool TFrameAggregator::GetClassifierResult(const TString& frameName, const TFrameAggregatorConfig::TClassifierRule& classifierRule,
    float* confidence) const
{
    Y_ENSURE(confidence);

    const TString& intentName = classifierRule.GetIntent() ? classifierRule.GetIntent() : frameName;

    const EFrameAggregatorSourceName source = ReadFrameAggregatorSourceName(classifierRule.GetSource());
    switch (source) {
        case EFrameAggregatorSourceName::UNKNOWN: {
            return false;
        }
        case EFrameAggregatorSourceName::ALWAYS: {
            return true;
        }
        case EFrameAggregatorSourceName::ALICE_ACTION_RECOGNIZER: {
            return AliceActionRecognizerFrames.contains(intentName);
        }
        case EFrameAggregatorSourceName::ALICE_BINARY_INTENT_CLASSIFIER: {
            return Sources.AliceBinaryIntentClassifier != nullptr &&
                FindValue(Sources.AliceBinaryIntentClassifier->GetProbabilities(), intentName, confidence) &&
                *confidence >= classifierRule.GetThreshold();
        }
        case EFrameAggregatorSourceName::ALICE_FIXLIST: {
            if (Sources.AliceFixlist == nullptr) {
                return false;
            }
            NBg::NProto::TAliceFixlistMatches matches;
            if (!FindValue(Sources.AliceFixlist->GetMatches(), intentName, &matches)) {
                return false;
            }
            return !matches.GetIntents().empty();
        }
        case EFrameAggregatorSourceName::ALICE_FRAME_FILLER: {
            return AliceFrameFillerFrames.contains(intentName);
        }
        case EFrameAggregatorSourceName::ALICE_MULTI_INTENT_CLASSIFIER: {
            if (Sources.AliceMultiIntentClassifier == nullptr) {
                return false;
            }
            const NBg::NProto::TMultiClassifierResult* clfResult =
                FindValue(Sources.AliceMultiIntentClassifier->GetClassifiersResult(), classifierRule.GetModel());

            if (clfResult == nullptr) {
                return false;
            }

            return FindValue(clfResult->GetProbabilities(), intentName, confidence) &&
                *confidence >= classifierRule.GetThreshold();
        }
        case EFrameAggregatorSourceName::ALICE_SCENARIOS_WORD_LSTM: {
            return Sources.AliceScenariosWordLstm != nullptr &&
                FindValue(Sources.AliceScenariosWordLstm->GetProbabilities(), intentName, confidence) &&
                *confidence >= classifierRule.GetThreshold();
        }
        case EFrameAggregatorSourceName::ALICE_TOLOKA_WORD_LSTM: {
            return Sources.AliceTolokaWordLstm != nullptr &&
                FindValue(Sources.AliceTolokaWordLstm->GetProbabilities(), intentName, confidence) &&
                *confidence >= classifierRule.GetThreshold();
        }
        case EFrameAggregatorSourceName::ALICE_TAGGER: {
            return AliceTaggerFrames.contains(intentName);
        }
        case EFrameAggregatorSourceName::ALICE_TRIVIAL_TAGGER: {
            return AliceTrivialTaggerFrames.contains(intentName);
        }
        case EFrameAggregatorSourceName::ALICE_WIZ_DETECTION: {
            const NBg::NProto::TModelResult* const* modelResult = AliceWizDetectionModelResults.FindPtr(classifierRule.GetModel());
            if (modelResult == nullptr) {
                return false;
            }
            Y_ENSURE(*modelResult);
            *confidence = (*modelResult)->GetProbability();
            if (classifierRule.GetThreshold() == 0) {
                // Threshold is not defined. Use threshold from rule.
                return (*modelResult)->GetPassed();
            }
            return *confidence >= classifierRule.GetThreshold();
        }
        case EFrameAggregatorSourceName::GRANET: {
            return GranetForms.contains(intentName);
        }
        default: {
            return false;
        }
    }
}

TMaybe<TSemanticFrame> TFrameAggregator::GetSemanticFrame(const TString& frameName, const TFrameAggregatorConfig::TTaggerRule& taggerRule) const {
    const TString& intentName = taggerRule.GetIntent() ? taggerRule.GetIntent() : frameName;

    const EFrameAggregatorSourceName source = ReadFrameAggregatorSourceName(taggerRule.GetSource());
    switch (source) {
        case EFrameAggregatorSourceName::UNKNOWN: {
            return TSemanticFrame();
        }
        case EFrameAggregatorSourceName::ALWAYS: {
            return TSemanticFrame();
        }
        case EFrameAggregatorSourceName::ALICE_ACTION_RECOGNIZER: {
            if (const TSemanticFrame* const* frame = AliceActionRecognizerFrames.FindPtr(intentName)) {
                return **frame;
            }
            return Nothing();
        }
        case EFrameAggregatorSourceName::ALICE_BINARY_INTENT_CLASSIFIER: {
            return TSemanticFrame();
        }
        case EFrameAggregatorSourceName::ALICE_FIXLIST: {
            if (Sources.AliceFixlist == nullptr) {
                return Nothing();
            }
            TSemanticFrame frame;
            frame.SetName(intentName);
            if (const NBg::NProto::TAliceFixlistMatches* matches = FindValue(Sources.AliceFixlist->GetMatches(), intentName)) {
                for (const auto& intent : matches->GetIntents()) {
                    auto& slot = *frame.AddSlots();
                    slot.SetName("intent");
                    auto& slotValue = *slot.MutableTypedValue();
                    slotValue.SetType("string");
                    slotValue.SetString(intent);
                }
            }
            return frame;
        }
        case EFrameAggregatorSourceName::ALICE_FRAME_FILLER: {
            if (const TSemanticFrame* const* frame = AliceFrameFillerFrames.FindPtr(intentName)) {
                return **frame;
            }
            return Nothing();
        }
        case EFrameAggregatorSourceName::ALICE_SCENARIOS_WORD_LSTM: {
            return TSemanticFrame();
        }
        case EFrameAggregatorSourceName::ALICE_TOLOKA_WORD_LSTM: {
            return TSemanticFrame();
        }
        case EFrameAggregatorSourceName::ALICE_TAGGER: {
            if (const TSemanticFrame* const* frame = AliceEntityRecognizerTaggerFrames.FindPtr(intentName)) {
                return **frame;
            }
            if (const TSemanticFrame* const* frame = AliceTaggerFrames.FindPtr(intentName)) {
                return **frame;
            }
            return Nothing();
        }
        case EFrameAggregatorSourceName::ALICE_TRIVIAL_TAGGER: {
            if (const TSemanticFrame* const* frame = AliceTrivialTaggerFrames.FindPtr(intentName)) {
                return **frame;
            }
            return Nothing();
        }
        case EFrameAggregatorSourceName::GRANET: {
            if (const TSemanticFrame* const* frame = AliceEntityRecognizerGranetFrames.FindPtr(intentName)) {
                return **frame;
            }
            if (const NBg::NProto::TGranetForm* const* form = GranetForms.FindPtr(intentName)) {
                return NBg::ConvertFormToSemanticFrame(**form, FindFrameDescription(intentName));
            }
            return Nothing();
        }
        default: {
            return TSemanticFrame();
        }
    }
}

const TFrameDescription* TFrameAggregator::FindFrameDescription(const TString& intent) const {
    if (FrameDescriptionMap == nullptr) {
        return nullptr;
    }
    return FrameDescriptionMap->FindPtr(intent);
}

} // namespace NAlice
