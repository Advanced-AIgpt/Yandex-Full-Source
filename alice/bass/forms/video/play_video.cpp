#include "play_video.h"

#include "change_track.h"
#include "vh_player.h"

#include <alice/library/video_common/video_helper.h>

namespace NBASS::NVideo {

using namespace NAlice::NVideoCommon;

NVideo::IVideoClipsProvider::TPlayResult PlayVideo(const TCandidateToPlay& candidate,
                                                   const NVideo::IVideoClipsProvider& provider, TContext& ctx,
                                                   const TMaybe<NVideo::TPlayData>& billingData, const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest) {
    auto cur = candidate.Curr;
    FillAgeLimit(cur); // Fix for 0+ content
    auto curr = cur.Scheme();
    auto next = candidate.Next.Scheme();
    auto parent = candidate.Parent.Scheme();

    return PlayVideo(curr, next, parent, provider, ctx, billingData, vhRequest);
}

TResultValue PlayVideoAndAddAttentions(const TCandidateToPlay& candidate, const NVideo::IVideoClipsProvider& provider,
                                       TContext& ctx, const TMaybe<NVideo::TPlayData>& billingData,
                                       const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest) {
    if (const auto error = PlayVideo(candidate, provider, ctx, billingData, vhRequest))
        return NVideo::AddAttentionsOrReturnError(ctx, *error);

    ctx.AddAttention(NVideo::ATTENTION_AUTOPLAY);
    return {};
}

IVideoClipsProvider::TPlayResult PlayVideo(
    TVideoItemConstScheme curr,
    TVideoItemConstScheme next,
    TVideoItemConstScheme parent,
    const IVideoClipsProvider& provider, TContext& ctx,
    const TMaybe<TPlayData>& billingData,
    const TMaybe<NHttpFetcher::THandle::TRef>& vhRequest) {

    TPlayVideoCommandData command;
    bool isVhSuccess = false;

    if (!ctx.HasExpFlag(FLAG_VIDEO_USE_OLD_BILLING) && vhRequest.Defined()) {
        if (auto playerData = GetVhPlayerDataByVhPlayerRequest(*vhRequest)) {
            isVhSuccess = true;
            command = GetPlayCommandData(ctx, *playerData);
            if (playerData->IsTvShow() || playerData->IsTvShowEpisode()) {
                TVideoItemScheme episodeItem = command->Item();
                UpdateSeasonAndEpisodeForTvShowEpisodeFromYdb(episodeItem, ctx);
                command->Item() = episodeItem;
            }
        }
    }
    if (!isVhSuccess) {
        if (const auto error = provider.GetPlayCommandData(curr, command.Scheme(), billingData)) {
            return *error;
        }
    }

    EItemType parentItemType;
    if (!TryFromString<EItemType>(parent->Type(), parentItemType)) {
        const TString msg = TStringBuilder() << "Unknown item type: " << parent->Type();
        LOG(ERR) << msg << Endl;
        return IVideoClipsProvider::TPlayError{NVideoCommon::EPlayError::VIDEOERROR, msg};
    }

    if (parentItemType == EItemType::TvShow) {
        command->TvShowItem() = parent;
        command->NextItem() = next;

        // Check if the current episode is from the same tv-show as the episode in currently playing because of the delay
        using TState = NBassApi::TVideoCurrentlyPlaying<TSchemeTraits>::TConst;
        TState state(ctx.Meta().DeviceState().Video().CurrentlyPlaying().GetRawValue());
        if (command->Item().TvShowItemId() == state.Item().TvShowItemId()) {
            command->AudioLanguage() = state.AudioLanguage();
            command->SubtitlesLanguage() = state.SubtitlesLanguage();
        }

        if (TMaybe<TWatchedTvShowItemScheme> lastWatchedItem =
                FindTvShowInLastWatched(command->TvShowItem(), ctx)) {
            if (!isVhSuccess) {
                // Compare current episode with episode in lastWatched
                if (command->Item().ProviderItemId().Get() == lastWatchedItem->Item().ProviderItemId().Get()) {
                    command->StartAt() = NAlice::NVideoCommon::CalculateStartAt(lastWatchedItem->Item().Progress().Duration(),
                                                          lastWatchedItem->Item().Progress().Played());
                }
            }

            // Check if the current episode is from the same tv-show as the episode in last watched
            if (command->Item().TvShowItemId() == lastWatchedItem->TvShowItem().ProviderItemId() &&
                command->AudioLanguage()->empty() && command->SubtitlesLanguage()->empty())
            {
                command->AudioLanguage() = lastWatchedItem->Item().AudioLanguage();
                command->SubtitlesLanguage() = lastWatchedItem->Item().SubtitlesLanguage();
            }
        }
    } else {
        if (parentItemType == EItemType::Video && command->Item().NextItems().Size() > 0) {
            TVideoItemScheme item = command->Item();
            command->NextItem() = item.NextItems(0);
            if (!ctx.HasExpFlag(FLAG_VIDEO_DISABLE_DOC2DOC)) {
                // Change thumb url to transparent picture because with doc2doc scenario
                // next video in gallery != next video by doc2doc
                // so the next video thumb is inconsistent with next video that will be played
                // and we should hide it
                // (P.S. It's also working even with incorrect url if needed)
                command->NextItem().ThumbnailUrl16X9() = "https://yastatic.net/s3/home/station/mordovia/doc2doc_video/blank.png ";
            }
        }

        if (TMaybe<TWatchedVideoItemScheme> lastWatchedItem =
                FindVideoInLastWatched(command->Item(), ctx)) {
            if (!isVhSuccess) {
                command->StartAt() =
                    NAlice::NVideoCommon::CalculateStartAt(lastWatchedItem->Progress().Duration(), lastWatchedItem->Progress().Played());
            }
            command->AudioLanguage() = lastWatchedItem->AudioLanguage();
            command->SubtitlesLanguage() = lastWatchedItem->SubtitlesLanguage();
        }
    }

    if (!ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_CHANGE_TRACK)) {
        if (TMaybe<NVideo::TVideoSlots> slots = NVideo::TVideoSlots::TryGetFromContext(ctx)) {
            // If one of the language slots is presented then we should reset the settings
            if (slots->AudioLanguage.Defined() || slots->SubtitleLanguage.Defined()) {
                command->AudioLanguage() = "";
                command->SubtitlesLanguage() = "";
            }

            // If slot AudioLanguage is presented
            if (slots->AudioLanguage.Defined()) {
                TAudioStreamOrSubtitleArrayScheme audioStreams(command->Item().AudioStreams().GetRawValue());
                if (const auto& bestTrack = FindMostSuitableTrack(audioStreams, slots->AudioLanguage.GetString());
                        bestTrack.Defined())
                {
                    command->AudioLanguage() = (*bestTrack)->Language();
                }
            }

            // If slot SubtitleLanguage is presented
            if (slots->SubtitleLanguage.Defined()) {
                TAudioStreamOrSubtitleArrayScheme subtitles(command->Item().Subtitles().GetRawValue());
                if (const auto& bestTrack = FindMostSuitableTrack(subtitles, slots->SubtitleLanguage.GetString());
                        bestTrack.Defined())
                {
                    const TStringBuf audioLanguage = command->AudioLanguage();
                    if (command->Item().PlayerRestrictionConfig().SubtitlesButtonEnable() ||
                        audioLanguage.StartsWith(LANGUAGE_RUS) || audioLanguage.Empty())
                    {
                        command->SubtitlesLanguage() = (*bestTrack)->Language();
                    }
                }
            }
        }
    }

    // cleanup for kinopoisk items
    if (command->HasItem()) {
        ClearKinopoiskServiceInfo(command->Item());
    }
    if (command->HasTvShowItem()) {
        ClearKinopoiskServiceInfo(command->TvShowItem());
    }
    if (command->HasNextItem()) {
        ClearKinopoiskServiceInfo(command->NextItem());
    }

    AddPlayCommand(ctx, command, true /* withEasterEggs */);
    return {};
}

} // namespace NBASS::NVideo
