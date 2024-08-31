#pragma once

#include <alice/hollywood/library/scenarios/onboarding/proto/onboarding.pb.h>
#include <dj/services/alisa_skills/server/proto/client/onboarding_response.pb.h>
#include <dj/services/alisa_skills/server/proto/client/response.pb.h>

#include <util/generic/string.h>

namespace NAlice::NHollywoodFw::NOnboarding {

    const TString GREETINGS_DIV_LAYER_CONTENT("content");
    const TString GREETINGS_DIV_LAYER_FOOTER("footer");
    const TString SCREEN_ID("cloud_ui");

    constexpr TStringBuf GREETINGS_SCENE_NAME("greetings");
    constexpr TStringBuf GREETINGS_CARD_NAME("get_greetings");
    constexpr TStringBuf GREETINGS_REQUEST_KEY("onboarding_greetings_request");
    constexpr TStringBuf GREETINGS_RESPONSE_KEY("onboarding_greetings_response");
    
    struct TResponseItem {
        TString Id;
        TString Activation;
        TString LogoUrl;
        TString LogoAvatarId;
    };

    struct TPromoSkillItem {
        TString Title;
        TString Activation;
        TString DivActionId;
        TString ImageUrl;
    };

    NDJ::NAS::TServiceResponse GetOldResponseFallbackValues();
    NDJ::NAS::TOnboardingResponse GetNewResponseFallbackValues();
    
    TGreetingsRenderProto GetPromoSkillsStub(std::size_t id, const bool isSearchApp);
    
}
