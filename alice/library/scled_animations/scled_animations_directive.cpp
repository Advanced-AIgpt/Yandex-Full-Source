#include "scled_animations_directive.h"
#include "scled_animations_builder.h"

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/megamind/protos/scenarios/response.pb.h>

namespace NAlice::NScledAnimation {

///
/// Add standard SCLED animation
///
void AddStandardScled(NAlice::NScenarios::TScenarioResponseBody& response, EScledAnimations anim) {
    TScledAnimationBuilder b;
    TScledAnimationOptions options;
    //
    // TODO [DD] Probably need to move these operations into external resources: like.bin, dislike.bin, etc...
    //
    switch (anim) {
        case EScledAnimations::SCLED_ANIMATION_LIKE:
            {
                constexpr TScledAnimationBuilder::TPattern a = TScledAnimationBuilder::MakeDigit(/* a= */ true, /* b= */ false, /* c= */ false, /* d= */ false, /* e= */ false, /* f= */ false, /* g= */ false);
                constexpr TScledAnimationBuilder::TPattern d = TScledAnimationBuilder::MakeDigit(false, false, false, true, false, false, false);
                constexpr TScledAnimationBuilder::TPattern g = TScledAnimationBuilder::MakeDigit(true, false, false, false, false, false, true);

                TScledAnimationBuilder::TFullPattern a34 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, a, a);
                TScledAnimationBuilder::TFullPattern d34 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, d, d);
                TScledAnimationBuilder::TFullPattern g34 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, g, g);

                b.SetDraw(d34, 255, 0, 180);
                b.SetAnim(d34, 255, 0, 180, 500, TScledAnimationBuilder::AnimModeFade);
                b.SetDraw(g34, 255, 200, 340);
                b.SetAnim(g34, 255, 0, 340, 660, TScledAnimationBuilder::AnimModeFade);
                b.SetDraw(a34, 255, 360, 500);
                b.SetAnim(a34, 255, 0, 500, 780, TScledAnimationBuilder::AnimModeFade);
            }
            break;
        case EScledAnimations::SCLED_ANIMATION_DISLIKE:
            {
                constexpr TScledAnimationBuilder::TPattern a = TScledAnimationBuilder::MakeDigit(true, false, false, false, false, false, false);
                constexpr TScledAnimationBuilder::TPattern d = TScledAnimationBuilder::MakeDigit(false, false, false, true, false, false, false);
                constexpr TScledAnimationBuilder::TPattern g = TScledAnimationBuilder::MakeDigit(true, false, false, false, false, false, true);

                TScledAnimationBuilder::TFullPattern a12 = TScledAnimationBuilder::MakeFullPattern(a, a, TScledAnimationBuilder::SpecialNothing, 0, 0);
                TScledAnimationBuilder::TFullPattern d12 = TScledAnimationBuilder::MakeFullPattern(d, d, TScledAnimationBuilder::SpecialNothing, 0, 0);
                TScledAnimationBuilder::TFullPattern g12 = TScledAnimationBuilder::MakeFullPattern(g, g, TScledAnimationBuilder::SpecialNothing, 0, 0);

                b.SetDraw(a12, 255, 0, 180);
                b.SetAnim(a12, 255, 0, 180, 500, TScledAnimationBuilder::AnimModeFade);
                b.SetDraw(g12, 255, 200, 340);
                b.SetAnim(g12, 255, 0, 340, 660, TScledAnimationBuilder::AnimModeFade);
                b.SetDraw(d12, 255, 360, 500);
                b.SetAnim(d12, 255, 0, 500, 780, TScledAnimationBuilder::AnimModeFade);
            }
            break;
        case EScledAnimations::SCLED_ANIMATION_NEXT:
            {
                constexpr TScledAnimationBuilder::TPattern bc = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);
                constexpr TScledAnimationBuilder::TPattern ef = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);

                TScledAnimationBuilder::TFullPattern bc3 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, bc, 0);
                TScledAnimationBuilder::TFullPattern bc4 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, 0, bc);
                TScledAnimationBuilder::TFullPattern ef3 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, ef, 0);
                TScledAnimationBuilder::TFullPattern ef4 = TScledAnimationBuilder::MakeFullPattern(0, 0, TScledAnimationBuilder::SpecialNothing, 0, ef);

                b.SetAnim(ef3, 255, 0, 0, 520, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(bc3, 255, 0, 200, 680, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(ef4, 255, 0, 360, 840, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(bc4, 255, 0, 520, 1000, TScledAnimationBuilder::AnimModeFade);
            }
            break;
        case EScledAnimations::SCLED_ANIMATION_PREVIOUS:
            {
                constexpr TScledAnimationBuilder::TPattern bc = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);
                constexpr TScledAnimationBuilder::TPattern ef = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);

                TScledAnimationBuilder::TFullPattern bc1 = TScledAnimationBuilder::MakeFullPattern(bc, 0, TScledAnimationBuilder::SpecialNothing, 0, 0);
                TScledAnimationBuilder::TFullPattern bc2 = TScledAnimationBuilder::MakeFullPattern(0, bc, TScledAnimationBuilder::SpecialNothing, 0, 0);
                TScledAnimationBuilder::TFullPattern ef1 = TScledAnimationBuilder::MakeFullPattern(ef, 0, TScledAnimationBuilder::SpecialNothing, 0, 0);
                TScledAnimationBuilder::TFullPattern ef2 = TScledAnimationBuilder::MakeFullPattern(0, ef, TScledAnimationBuilder::SpecialNothing, 0, 0);

                b.SetAnim(bc2, 255, 0, 0, 520, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(ef2, 255, 0, 200, 680, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(bc1, 255, 0, 360, 840, TScledAnimationBuilder::AnimModeFade);
                b.SetAnim(ef1, 255, 0, 520, 1000, TScledAnimationBuilder::AnimModeFade);
            }
            break;
        case EScledAnimations::SCLED_ANIMATION_PAUSE:
            {
                constexpr TScledAnimationBuilder::TPattern bc = TScledAnimationBuilder::MakeDigit(false, true, true, false, false, false, false);
                constexpr TScledAnimationBuilder::TPattern ef = TScledAnimationBuilder::MakeDigit(false, false, false, false, true, true, false);

                TScledAnimationBuilder::TFullPattern pause = TScledAnimationBuilder::MakeFullPattern(0, bc, TScledAnimationBuilder::SpecialNothing, ef, 0);

                b.SetAnim(pause, 0, 255, 0, 300, TScledAnimationBuilder::AnimModeFade);
                b.SetDraw(pause, 255, 300, 2000);
                b.SetAnim(pause, 255, 0, 2000, 2300, TScledAnimationBuilder::AnimModeFade);
            }
            break;
    }
    AddDrawScled(response, b, options);
}

///
/// Add SCLED animation from animation builder
///
void AddAnimatedScled(NAlice::NScenarios::TScenarioResponseBody& response, const TString& pattern) {
    TScledAnimationBuilder scled;

    scled.AddAnim(pattern, 0, 255, 1000, TScledAnimationBuilder::AnimModeFromRight | TScledAnimationBuilder::AnimModeSpeedSmooth);
    scled.AddDraw(pattern, 255, 1000);
    scled.AddAnim(pattern, 255, 0, 1000, TScledAnimationBuilder::AnimModeFromLeft | TScledAnimationBuilder::AnimModeSpeedSmooth);

    AddDrawScled(response, scled);

    NScenarios::TDirective directive;
    auto& ttsPlayPlaceholderDirective = *directive.MutableTtsPlayPlaceholderDirective();
    ttsPlayPlaceholderDirective.SetName("tts_play_placeholder");
    *response.MutableLayout()->AddDirectives() = std::move(directive);
}

///
/// Add SCLED animation from animation builder
///
void AddDrawScled(NAlice::NScenarios::TScenarioResponseBody& response, TScledAnimationBuilder& scledBuilder, 
                         TScledAnimationOptions options /*= TScledAnimationOptions{}*/) {
    AddCustomScled(response, scledBuilder.PrepareBinary().c_str(), options);
}

///
/// Add custom SCLED animation created on your own side 
///
void AddCustomScled(NAlice::NScenarios::TScenarioResponseBody& response, const TStringBuf& anim, 
                         TScledAnimationOptions options /*= TScledAnimationOptions{}*/) {
    NScenarios::TDirective directive;
    auto& setScledDirective = *directive.MutableDrawScledAnimationsDirective();
    setScledDirective.SetName("draw_segment_animations");

    auto* animationsDirective = setScledDirective.AddAnimations();

    animationsDirective->SetName("animation_1");

    animationsDirective->SetBase64EncodedValue(anim.Data());

    //
    // Current officially supported format is GZIP
    //
    animationsDirective->SetCompression(NScenarios::TDrawScledAnimationsDirective_TAnimation_ECompressionType_Gzip);

    if (options.RepeatFlag) {
        setScledDirective.SetAnimationStopPolicy(options.TillEndOfTTSFlag ?
            NScenarios::TDrawScledAnimationsDirective_EAnimationStopPolicy_RepeatLastTillEndOfTTS : 
            NScenarios::TDrawScledAnimationsDirective_EAnimationStopPolicy_RepeatLastTillNextDirective);
    } else {
        setScledDirective.SetAnimationStopPolicy(options.TillEndOfTTSFlag ?
            NScenarios::TDrawScledAnimationsDirective_EAnimationStopPolicy_PlayOnceTillEndOfTTS : 
            NScenarios::TDrawScledAnimationsDirective_EAnimationStopPolicy_PlayOnce);
    } 
    if (options.AddSpeakingAnimationFlag) {
        // Default mode: add speaking animation after SCLED if TTS is longer
        setScledDirective.SetSpeakingAnimationPolicy(NScenarios::TDrawScledAnimationsDirective_ESpeakingAnimationPolicy_PlaySpeakingEndOfTts);
    } else {
        // Special mode (for animations like Alarms: ignore speaking animation if TTS is longer)
        setScledDirective.SetSpeakingAnimationPolicy(NScenarios::TDrawScledAnimationsDirective_ESpeakingAnimationPolicy_ShowClockImmediately);
    }
    *response.MutableLayout()->AddDirectives() = std::move(directive);

    if (options.AddTtslayPlaceholder) {
        NScenarios::TDirective ttsDirective;
        auto& ttsPlayPlaceholderDirective = *ttsDirective.MutableTtsPlayPlaceholderDirective();
        ttsPlayPlaceholderDirective.SetName("tts_play_placeholder");
        *response.MutableLayout()->AddDirectives() = std::move(ttsDirective);
    }
}

} // namespace NAlice::NScledAnimation
