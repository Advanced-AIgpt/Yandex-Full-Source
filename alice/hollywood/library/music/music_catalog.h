#pragma once

#include <alice/library/client/client_info.h>
#include <alice/library/json/json.h>
#include <alice/library/logger/logger.h>

namespace NAlice::NHollywood::NMusic {

constexpr TStringBuf MUSIC_CATALOG_HTTP_RESPONSE_ITEM = "music_catalog_http_response";

NJson::TJsonValue ParseMusicCatalogResponse(
    const TClientInfo& clientInfo,
    const NJson::TJsonValue& musicCatalogJsonResponse,
    const THashMap<TString, TMaybe<TString>>& expFlags,
    const bool supportIntentUrls,
    const bool isFairyTaleFilterGenre,
    TRTLogger& logger
);

} // namespace NAlice::NHollywood::NMusic
