#pragma once

#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/music_sdk_uri_builder/music_sdk_uri_builder.h>
#include <alice/hollywood/library/music/music_resources.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

void AddWebOSLaunchAppDirective(
    const TScenarioBaseRequestWithInputWrapper& request, TNlgDataBuilder& nlgDataBuilder, TResponseBodyBuilder& bodyBuilder,
    bool isUserUnauthorizedOrWithoutSubscription, const TMaybe<TContentId>& contentId, const NAlice::TEnvironmentState* envState);

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
