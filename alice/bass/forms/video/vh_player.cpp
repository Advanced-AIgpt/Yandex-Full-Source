#include "vh_player.h"

#include <alice/library/json/json.h>

namespace NBASS::NVideo {

NHttpFetcher::THandle::TRef CreateVhPlayerRequest(const NBASS::TContext& ctx, const TStringBuf itemId) {
    NHttpFetcher::TRequestPtr request = MakeVhPlayerRequest(ctx, itemId);
    return request->Fetch();
}

TMaybe<TVhPlayerData> GetVhPlayerDataByVhPlayerRequest(const NHttpFetcher::THandle::TRef& vhRequest) {
    NHttpFetcher::TResponse::TRef frontendVhResponse = vhRequest->Wait();
    if (!frontendVhResponse->IsHttpOk()) {
        return Nothing();
    }
    NJson::TJsonValue vhResponseData;
    NJson::ReadJsonTree(frontendVhResponse->Data, &vhResponseData);
    TVhPlayerData vhPlayerData;
    if (!vhPlayerData.ParseJsonDoc(vhResponseData["content"])) {
        return Nothing();
    }
    return vhPlayerData;
}

NBASS::NVideo::TPlayVideoCommandData GetPlayCommandData(const NBASS::TContext& ctx, const TVhPlayerData& vhPlayerData) {
    NBASS::NVideo::TPlayVideoCommandData command;
    ui64 startTime = vhPlayerData.GetStartAt(ctx.Now().MilliSeconds());

    if (vhPlayerData.IsPlayableVhPlayerData()) {
        command->Item() = MakeSchemeVideoItem(vhPlayerData).Scheme();
        command->Uri() = vhPlayerData.PlayUri;
        command->StartAt() = startTime;
        command->Payload() = vhPlayerData.MakePayload(startTime);
        command->SessionToken() = vhPlayerData.SessionId;
    } else {
        command = GetPlayCommandData(ctx, vhPlayerData.GetPlayableVhPlayerData());
        if (vhPlayerData.IsTvShow()) {
            command->TvShowItem() = MakeSchemeVideoItem(vhPlayerData).Scheme();
        }
    }
    return command;
}

NBASS::NVideo::TVideoItem MakeSchemeVideoItem(const TVhPlayerData& vhPlayerData) {
    NBASS::NVideo::TVideoItem bassItem;
    bassItem->Name() = vhPlayerData.Title;
    bassItem->ProviderItemId() = vhPlayerData.Uuid;
    bassItem->Type() = ToString(vhPlayerData.VideoType);
    bassItem->Description() = vhPlayerData.Description;
    bassItem->Duration() = vhPlayerData.Duration;
    bassItem->ProviderName() = TString(PROVIDER_STRM);
    bassItem->PlayUri() = vhPlayerData.PlayUri;
    bassItem->MinAge() = vhPlayerData.RestrictionAge;
    bassItem->AgeLimit() = ToString(vhPlayerData.RestrictionAge);
    bassItem->Available() = true;

    if (!vhPlayerData.OntoId.Empty()) {
        bassItem->Entref() = "entnext=" + vhPlayerData.OntoId;
    }

    if (vhPlayerData.IsFilm() || vhPlayerData.IsTvShow() || vhPlayerData.IsTvShowEpisode()) {
        NBASS::NVideo::TLightVideoItem lightItem;
        lightItem->Type() = ToString(vhPlayerData.VideoType);
        lightItem->ProviderItemId() = vhPlayerData.Uuid;
        lightItem->ProviderName() = TString(PROVIDER_KINOPOISK);
        lightItem->Available() = vhPlayerData.HasActiveLicense;
        bassItem->ProviderName() = TString(PROVIDER_KINOPOISK);
        bassItem->Available() = vhPlayerData.HasActiveLicense;
        bassItem->Genre() = vhPlayerData.Genre;
        bassItem->Directors() = vhPlayerData.Directors;
        bassItem->Actors() = vhPlayerData.Actors;
        bassItem->Rating() = vhPlayerData.RatingKP;
        bassItem->ReleaseYear() = vhPlayerData.ReleaseYear;
        bassItem->CoverUrl16X9() = vhPlayerData.CoverUrl16X9;
        bassItem->CoverUrl2X3() = vhPlayerData.CoverUrl2X3;
        bassItem->ThumbnailUrl16X9() = vhPlayerData.ThumbnailUrl16X9;
        if (vhPlayerData.IsTvShow() || vhPlayerData.IsTvShowEpisode()) {
            bassItem->Episode() = vhPlayerData.EpisodeNumber;
            bassItem->Season() = vhPlayerData.SeasonNumber;
            lightItem->Episode() = vhPlayerData.EpisodeNumber;
            lightItem->Season() = vhPlayerData.SeasonNumber;
            bassItem->TvShowSeasonId() = vhPlayerData.TvShowSeasonId;
            lightItem->TvShowSeasonId() = vhPlayerData.TvShowSeasonId;
            bassItem->TvShowItemId() = vhPlayerData.TvShowItemId;
            lightItem->TvShowItemId() = vhPlayerData.TvShowItemId;
        }

        for (const auto& skippableFragmentProto : vhPlayerData.SkippableFragments) {
            NBASS::NVideo::TSkippableFragment skippableFragment;
            skippableFragment->StartTime() = skippableFragmentProto.starttime();
            skippableFragment->EndTime() = skippableFragmentProto.endtime();
            skippableFragment->Type() = skippableFragmentProto.type();
            bassItem->SkippableFragments().Add() = skippableFragment.Scheme();
        }
        for (const auto& audioStreamProto : vhPlayerData.AudioStreams) {
            NBASS::NVideo::TAudioStreamOrSubtitle audioStream;
            audioStream->Index() = audioStreamProto.index();
            audioStream->Language() = audioStreamProto.language();
            audioStream->Suggest() = audioStreamProto.suggest();
            audioStream->Title() = audioStreamProto.title();
            bassItem->AudioStreams().Add() = audioStream.Scheme();
        }
        for (const auto& subtitleProto : vhPlayerData.Subtitles) {
            NBASS::NVideo::TAudioStreamOrSubtitle subtitle;
            subtitle->Index() = subtitleProto.index();
            subtitle->Language() = subtitleProto.language();
            subtitle->Suggest() = subtitleProto.suggest();
            subtitle->Title() = subtitleProto.title();
            bassItem->Subtitles().Add() = subtitle.Scheme();
        }
        bassItem->PlayerRestrictionConfig().SubtitlesButtonEnable() = vhPlayerData.PlayerRestrictionConfig.subtitlesbuttonenable();
        bassItem->ProviderInfo().Add() = lightItem.Scheme();
    }
    return bassItem;
}

} // namespace NBASS::NVideo
