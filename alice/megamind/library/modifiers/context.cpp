#include "context.h"

#include <alice/library/json/json.h>

namespace NAlice::NMegamind {

TMaybe<NSc::TValue> ParsePersonalData(const TStringBuf json) {
    if (json) {
        return NSc::TValue::FromJsonThrow(json);
    }
    return Nothing();
}

TResponseModifierContext::TResponseModifierContext(const IContext& ctx,
                                                   TModifiersStorage& modifiersStorage,
                                                   TModifiersInfo& modifiersInfo,
                                                   TProactivityLogStorage& proactivityLogStorage,
                                                   const TVector<TSemanticFrame>& semanticFrames,
                                                   const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                                                   TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                   ui64 randomSeed,
                                                   const TProactivityAnswer& proactivity)
    : Ctx(ctx)
    , SpeechKitRequest_(ctx.SpeechKitRequest())
    , Session_(ctx.Session())
    , PersonalData_(ParsePersonalData(ctx.SpeechKitRequest().Proto().GetRequest().GetRawPersonalData()))
    , RecognizedActionEffectFrames_(recognizedActionEffectFrames)
    , Responses_(ctx.Responses())
    , ModifiersStorage_(modifiersStorage)
    , ModifiersInfo_(modifiersInfo)
    , Rng_(randomSeed)
    , Proactivity_(proactivity)
    , ProactivityLogStorage_(proactivityLogStorage)
    , SemanticFrames_(semanticFrames)
    , UserConfigs_(ctx.MementoData().GetUserConfigs())
    , MegamindAnalyticsInfoBuilder_(analyticsInfoBuilder)
    , Sensors_(ctx.Sensors())
{
}

TResponseModifierContext::TResponseModifierContext(const IContext& ctx,
                                                   TModifiersStorage& modifiersStorage,
                                                   TModifiersInfo& modifiersInfo,
                                                   TProactivityLogStorage& proactivityLogStorage,
                                                   const TVector<TSemanticFrame>& semanticFrames,
                                                   const TVector<TSemanticFrame>& recognizedActionEffectFrames,
                                                   TMegamindAnalyticsInfoBuilder& analyticsInfoBuilder,
                                                   const TProactivityAnswer& proactivity)
    : TResponseModifierContext(ctx,
                               modifiersStorage,
                               modifiersInfo,
                               proactivityLogStorage,
                               semanticFrames,
                               recognizedActionEffectFrames,
                               analyticsInfoBuilder,
                               ctx.SpeechKitRequest().GetSeed(),
                               proactivity)
{
}

} // namespace NAlice::NMegamind
