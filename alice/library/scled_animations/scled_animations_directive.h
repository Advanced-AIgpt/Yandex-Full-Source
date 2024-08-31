#pragma once

#include "scled_animations_builder.h"

//
// Forward declarations
//
namespace NAlice::NScenarios {
    
class TScenarioResponseBody;

}


namespace NAlice {

class TScledAnimationBuilder;

namespace NScledAnimation {

enum class EScledAnimations {
    SCLED_ANIMATION_LIKE,
    SCLED_ANIMATION_DISLIKE,
    SCLED_ANIMATION_NEXT,
    SCLED_ANIMATION_PREVIOUS,
    SCLED_ANIMATION_PAUSE
};

/**
 * IMPORTANT NOTICE!!!
 * 
 * To play segment animations in the same time with TTS you have to add tts_play_placeholder directive into response!
 * 
 */
struct TScledAnimationOptions {
    bool RepeatFlag = false;
    bool TillEndOfTTSFlag = false;
    bool AddSpeakingAnimationFlag = true;
    bool AddTtslayPlaceholder = false;
};

// Direct access for NAlice::NScenarios::TScenarioResponseBody
extern void AddStandardScled(NScenarios::TScenarioResponseBody& response, EScledAnimations anim);
extern void AddAnimatedScled(NScenarios::TScenarioResponseBody& response, const TString& pattern);
extern void AddDrawScled(NScenarios::TScenarioResponseBody& response, TScledAnimationBuilder& scledBuilder, 
                         TScledAnimationOptions options = TScledAnimationOptions{});
extern void AddCustomScled(NScenarios::TScenarioResponseBody& response, const TStringBuf& anim, 
                         TScledAnimationOptions options = TScledAnimationOptions{});


} // namespace NScledAnimation

} // namespace NAlice
