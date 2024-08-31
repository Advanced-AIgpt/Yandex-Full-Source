#include "whisper_modifier.h"

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/contextual_data.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

#include <util/generic/algorithm.h>

namespace NAlice::NHollywood::NModifiers {

namespace {

inline const TString MODIFIER_TYPE = "whisper";
const TString TAG_WHISPER_ON = "<speaker is_whisper=\"true\"> ";
const TString TAG_SIL_100MS = "sil<[100]> ";
const TString SOUND_SET_LEVEL = "sound_set_level";
const uint WHISPERING_SOUND_LEVEL = 3;

inline constexpr TStringBuf EXP_ENABLE_FORCED_APPLY_WHISPER_MODIFIER = "mm_enable_forced_apply_whisper_modifier";

bool HasAnySoundLevelDirective(const TResponseBodyBuilder& responseBody) {
    return AnyOf(responseBody.GetModifierBody().GetLayout().GetDirectives(), [](const auto& directive) {
        return directive.HasSoundSetLevelDirective() || directive.HasSoundQuiterDirective() ||
               directive.HasSoundLouderDirective() || directive.HasSoundMuteDirective() ||
               directive.HasSoundUnmuteDirective();
    });
}

bool HasTtsPlayPlaceholder(const TResponseBodyBuilder& responseBody) {
    return AnyOf(responseBody.GetModifierBody().GetLayout().GetDirectives(), [](const auto& directive) {
        return directive.HasTtsPlayPlaceholderDirective();
    });
}

bool ShouldChangeSoundLevel(IModifierContext& ctx, const TResponseBodyBuilder& responseBody,
                            const uint minSoundLevel) {
    const bool hasAnySoundDirectives = HasAnySoundLevelDirective(responseBody);

    const bool isPreviousRequestWhisper = ctx.GetFeatures().GetSoundSettings().GetIsPreviousRequestWhisper();

    const bool soundLevelExceedsMinimal = !ctx.GetFeatures().GetSoundSettings().HasSoundLevel() ||
                                          ctx.GetFeatures().GetSoundSettings().GetSoundLevel() > minSoundLevel;

    return !hasAnySoundDirectives && !isPreviousRequestWhisper && soundLevelExceedsMinimal;
}

void AddSoundSetLevelDirective(TResponseBodyBuilder& responseBody, const uint soundLevel,
                               const NMegamind::TModifierFeatures::TSoundSettings& soundSettings,
                               const bool supportsTtsPlayPlaceholder = false) {
    NScenarios::TSoundSetLevelDirective soundSetLevelDirective;
    soundSetLevelDirective.SetNewLevel(soundLevel);
    soundSetLevelDirective.SetName(SOUND_SET_LEVEL);
    if (soundSettings.HasMultiroomSessionId()) {
        soundSetLevelDirective.SetMultiroomSessionId(soundSettings.GetMultiroomSessionId());
    }
    NScenarios::TDirective directive;
    *directive.MutableSoundSetLevelDirective() = std::move(soundSetLevelDirective);
    if (supportsTtsPlayPlaceholder && !HasTtsPlayPlaceholder(responseBody)) {
        NScenarios::TDirective placeholderDirective;
        *placeholderDirective.MutableTtsPlayPlaceholderDirective() = {};
        responseBody.AddDirectivesToFront({std::move(directive), std::move(placeholderDirective)});
    } else {
        responseBody.AddDirectiveToFront(std::move(directive));
    }
}

} // namespace

namespace NImpl {

bool IsApplicable(IModifierContext& ctx, const TResponseBodyBuilder& responseBody) {
    const bool enabledByExp = ctx.HasExpFlag(EXP_ENABLE_FORCED_APPLY_WHISPER_MODIFIER);
    const auto& soundSettings = ctx.GetFeatures().GetSoundSettings();
    const bool enabledByWhisperRequest = soundSettings.GetIsWhisper();
    const bool hasSpeech = !responseBody.GetModifierBody().GetLayout().GetOutputSpeech().empty();

    if (enabledByExp) {
        return true;
    }

    if (!hasSpeech) {
        return false;
    }

    switch (ctx.GetFeatures().GetContextualData().GetWhisper().GetHint()) {
        case NData::TContextualData::TWhisper::ForcedDisable:
            return false;
        case NData::TContextualData::TWhisper::ForcedEnable:
            return true;
        default:
            return enabledByWhisperRequest;
    }
}

TApplyResult TryApplyImpl(IModifierContext& ctx, TResponseBodyBuilder& responseBody,
                          TModifierAnalyticsInfoBuilder& analyticsInfo) {
    if (!IsApplicable(ctx, responseBody)) {
        return TNonApply{TNonApply::EType::NotApplicable};
    }
    const auto& soundSettings = ctx.GetFeatures().GetSoundSettings();
    const bool shouldApplyWisperTag = !soundSettings.GetIsWhisperTagDisabled();
    if (shouldApplyWisperTag) {
        responseBody.PrependVoice(TAG_WHISPER_ON);
        LOG_INFO(ctx.Logger()) << "Applied whisper tag";
    }
    const bool shouldApplySoundSetLevelDirective = ShouldChangeSoundLevel(ctx, responseBody, WHISPERING_SOUND_LEVEL);
    if (shouldApplySoundSetLevelDirective) {
        AddSoundSetLevelDirective(responseBody, WHISPERING_SOUND_LEVEL, ctx.GetFeatures().GetSoundSettings(),
                                  ctx.GetBaseRequest().GetInterfaces().GetTtsPlayPlaceholder());
        responseBody.PrependVoice(TAG_SIL_100MS);
        LOG_INFO(ctx.Logger()) << "Added SoundSetLevelDirective";
    }

    analyticsInfo.SetWhisper(shouldApplyWisperTag, shouldApplySoundSetLevelDirective);

    return Nothing();
}

} // namespace NImpl

TWhisperModifier::TWhisperModifier()
    : TBaseModifier(MODIFIER_TYPE) {
}

TApplyResult TWhisperModifier::TryApply(TModifierApplyContext applyCtx) const {
    return NImpl::TryApplyImpl(applyCtx.ModifierContext, applyCtx.ResponseBody, applyCtx.AnalyticsInfo);
}

} // namespace NAlice::NHollywood::NModifiers
