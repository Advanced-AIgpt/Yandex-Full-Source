#include "colored_speaker_modifier.h"

#include <alice/hollywood/library/modifiers/internal/config/proto/exact_key_groups.pb.h>
#include <alice/hollywood/library/modifiers/internal/config/proto/exact_mapping_config.pb.h>
#include <alice/library/client/protos/promo_type.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

#include <alice/library/proto/proto.h>

#include <util/stream/file.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

inline const TString MODIFIER_TYPE = "colored_speaker";

constexpr TStringBuf CONFIG_FILE_NAME = "mapping.pb.txt";
constexpr TStringBuf PHRASE_GROUPS_FILE_NAME = "key_groups.pb.txt";

bool ResponseHasVoice(const NScenarios::TLayout& layout) {
    return !layout.GetOutputSpeech().empty();
}

bool IsApplicable(const IModifierContext& ctx, const NScenarios::TLayout& layout) {
    const auto& features = ctx.GetFeatures();
    return features.GetPromoType() != NClient::PT_NO_TYPE && ResponseHasVoice(layout);
}

} // namespace

namespace NImpl {

TApplyResult TryApplyImpl(IModifierContext& ctx, TResponseBodyBuilder& responseBody,
                          TModifierAnalyticsInfoBuilder& analyticsInfo, const TExactMatcher& matcher) {
    if (!IsApplicable(ctx, responseBody.GetModifierBody().GetLayout())) {
        return TNonApply{TNonApply::EType::NotApplicable};
    }
    if (const auto* matchingsPtr =
            matcher.FindPtr(ctx.GetFeatures().GetPromoType(), ctx.GetFeatures().GetProductScenarioName(),
                            responseBody.GetModifierBody().GetLayout().GetOutputSpeech());
        matchingsPtr && !matchingsPtr->empty()) {

        const auto oldTts = responseBody.GetModifierBody().GetLayout().GetOutputSpeech(); // copy is intentional
        responseBody.SetRandomPhrase(*matchingsPtr, ctx.Rng());
        analyticsInfo.SetColoredSpeaker(oldTts, responseBody.GetModifierBody().GetLayout().GetOutputSpeech());

        return Nothing();
    }
    return TNonApply{/* reason= */ "No mapping found"};
}

} // namespace NImpl

TColoredSpeakerModifier::TColoredSpeakerModifier()
    : TBaseModifier(MODIFIER_TYPE)
{
}

void TColoredSpeakerModifier::LoadResourcesFromPath(const TFsPath& modifierResourcesBasePath) {
    TFileInput configInput(modifierResourcesBasePath / CONFIG_FILE_NAME);
    TFileInput phrasesInput(modifierResourcesBasePath / PHRASE_GROUPS_FILE_NAME);
    Matcher_ = std::make_unique<TExactMatcher>(ParseProtoText<TExactMappingConfig>(configInput.ReadAll()),
                                               ParseProtoText<TExactKeyGroups>(phrasesInput.ReadAll()));
}

TApplyResult TColoredSpeakerModifier::TryApply(TModifierApplyContext applyCtx) const {
    Y_ENSURE(Matcher_, "Matcher is not initialized");
    return NImpl::TryApplyImpl(applyCtx.ModifierContext, applyCtx.ResponseBody, applyCtx.AnalyticsInfo, *Matcher_);
}

} // namespace NAlice::NHollywood::NModifiers
