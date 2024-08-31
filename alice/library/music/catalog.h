#pragma once

#include <alice/megamind/protos/common/frame.pb.h>

#include <library/cpp/json/writer/json_value.h>

namespace NAlice {

struct TClientInfo;

} // namespace NAlice

namespace NAlice::NMusic {

TVector<NJson::TJsonValue> ParseMusicCatalogResponseToMusicCatalogAnswers(
    const NJson::TJsonValue& musicCatalogJsonResponse
);

NJson::TJsonValue ConvertMusicCatalogAnswerToMusicInfo(
    const TClientInfo& clientInfo,
    const bool supportsIntentUrls,
    const bool needAutoplay,
    const NJson::TJsonValue& musicCatalogAnswer
);

bool IsChildContent(
    const NJson::TJsonValue& musicInfo,
    const THashMap<TString, TMaybe<TString>>& expFlags
);

TVector<TStringBuf> GetFairyTaleGenres(
    const THashMap<TString, TMaybe<TString>>& expFlags
);

NAlice::TMusicPlaySemanticFrame ConstructMusicPlaySemanticFrame(
    const NJson::TJsonValue& musicInfo,
    const bool repeat
);

} // namespace NAlice::NMusic
