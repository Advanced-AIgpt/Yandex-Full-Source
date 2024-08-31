#include "unauthorized_user_directives.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/consts.h>
#include <alice/hollywood/library/scenarios/music/proto/music_arguments.pb.h>

namespace NAlice::NHollywood::NMusic {

namespace {

constexpr TStringBuf UNAUTHORIZED_ERROR_CODE = "music_authorization_problem";  // user is not authorized

template<typename TScenarioRequestWrapper>
bool DeviceAllowsDirectives(const TScenarioRequestWrapper& request) {
    return request.Interfaces().GetSupportsUnauthorizedMusicDirectives();
}

bool BassStateHasError(const NJson::TJsonValue& bassStateJson, const TStringBuf errorCode) {
    const auto& blocks = bassStateJson["blocks"].GetArray();
    return AnyOf(blocks, [errorCode](const auto& block) {
        return block["data"]["code"].GetString() == errorCode;
    });
}

}

bool TryAddUnauthorizedUserDirectives(
    const bool loggedIn,
    const bool userHasSubscription,
    const bool hasPromo,
    TResponseBodyBuilder& bodyBuilder
) {
    NScenarios::TDirective directive;
    if (!loggedIn) {
        directive.MutableShowLoginDirective();
    } else if (hasPromo) {
        directive.MutableShowPlusPromoDirective();
    } else if (!userHasSubscription) {
        // if a user has Plus, he has the Music subscription,
        // but some users without Plus have Music subscription (via legacy way),
        // now the only way to get a new Music subscription is to get Plus.
        directive.MutableShowPlusPurchaseDirective();
    } else {
        return false;
    }

    bodyBuilder.AddDirective(std::move(directive));
    return true;
}

bool TryAddUnauthorizedUserDirectivesForThinClient(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NMusic::TMusicContext musicContext,
    TResponseBodyBuilder& bodyBuilder
) {
    if (!DeviceAllowsDirectives(applyRequest)) {
        return false;
    }

    const bool userIsLoggedIn = !musicContext.GetAccountStatus().GetUid().Empty();
    const bool userHasSubscription = musicContext.GetAccountStatus().GetHasMusicSubscription();
    const bool userHasPromo = musicContext.GetAccountStatus().HasPromo();

    return TryAddUnauthorizedUserDirectives(userIsLoggedIn, userHasSubscription, userHasPromo, bodyBuilder);
}

bool TryAddUnauthorizedUserDirectivesForThickClient(
    const NHollywood::TScenarioApplyRequestWrapper& applyRequest,
    const NJson::TJsonValue& bassStateJson,
    TResponseBodyBuilder& bodyBuilder
) {
    if (!DeviceAllowsDirectives(applyRequest)) {
        return false;
    }
    const auto& applyArgs = applyRequest.UnpackArgumentsAndGetRef<TMusicArguments>();

    const bool userIsLoggedIn = !applyArgs.GetAccountStatus().GetUid().Empty();
    const bool userHasSubscription = !BassStateHasError(bassStateJson, PAYMENT_REQUIRED_ERROR_CODE);
    const bool userHasPromo = BassStateHasError(bassStateJson, PROMO_AVAILABLE_ERROR_CODE); // absense of "!" is important

    return TryAddUnauthorizedUserDirectives(userIsLoggedIn, userHasSubscription, userHasPromo, bodyBuilder);
}

bool TryAddUnauthorizedUserDirectivesForVinsRunResponse(
    const NHollywood::TScenarioRunRequestWrapper& runRequest,
    const NJson::TJsonValue& bassStateJson,
    TResponseBodyBuilder& bodyBuilder
) {
    if (!DeviceAllowsDirectives(runRequest)) {
        return false;
    }

    const bool userIsLoggedIn = !BassStateHasError(bassStateJson, UNAUTHORIZED_ERROR_CODE);
    const bool userHasSubscription = !BassStateHasError(bassStateJson, PAYMENT_REQUIRED_ERROR_CODE);
    const bool userHasPromo = BassStateHasError(bassStateJson, PROMO_AVAILABLE_ERROR_CODE); // absense of "!" is important

    return TryAddUnauthorizedUserDirectives(userIsLoggedIn, userHasSubscription, userHasPromo, bodyBuilder);
}

} // namespace NAlice::NHollywood::NMusic
