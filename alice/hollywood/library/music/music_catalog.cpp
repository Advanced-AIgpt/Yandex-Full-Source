#include "music_catalog.h"

#include <alice/library/music/catalog.h>

namespace NAlice::NHollywood::NMusic {

NJson::TJsonValue ParseMusicCatalogResponse(
    const TClientInfo& clientInfo,
    const NJson::TJsonValue& musicCatalogJsonResponse,
    const THashMap<TString, TMaybe<TString>>& expFlags,
    const bool supportIntentUrls,
    const bool isFairyTaleFilterGenre,
    TRTLogger& logger
) {
    const auto fairyTaleGenres = NAlice::NMusic::GetFairyTaleGenres(expFlags);

    for (auto musicCatalogAnswer : NAlice::NMusic::ParseMusicCatalogResponseToMusicCatalogAnswers(musicCatalogJsonResponse)) {
        auto musicInfo = NAlice::NMusic::ConvertMusicCatalogAnswerToMusicInfo(
            clientInfo,
            supportIntentUrls,
            /* needAutoplay = */ false,
            musicCatalogAnswer
        );

        if (musicInfo.IsNull()) {
            continue;
        }

        bool isChildContent = NAlice::NMusic::IsChildContent(musicInfo, expFlags);

        if (isFairyTaleFilterGenre && !fairyTaleGenres.empty() && !isChildContent) {
            continue;
        }

        musicInfo["is_child_content"] = isChildContent;

        LOG_DEBUG(logger) << "WEB SEARCH (music) answer: " << NAlice::JsonToString(musicInfo) << Endl;

        musicInfo["source"] = "web";

        return musicInfo;
    }

    LOG_ERROR(logger) << "WEB SEARCH (music) error: music_not_found" << Endl;
    return NJson::TJsonValue(NJson::JSON_NULL);
}

} // namespace NAlice::NHollywood::NMusic
