#pragma once

#include "channels_info.h"

#include <alice/bass/forms/context/fwd.h>
#include <alice/bass/forms/video/defs.h>

#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/util/error.h>

#include <alice/library/blackbox/blackbox.h>
#include <alice/library/geo/user_location.h>

namespace NBASS {

class TTvChannelsHelper {
public:
    explicit TTvChannelsHelper(TContext& ctx);

    static bool CanShowTvStreamOnClient(const TContext& ctx);
    static TChannelsInfo::TChannel::TConstPtr GetPersonalTvChannelInfo();

    TResultValue PlayCurrentTvEpisode(NVideo::TVideoItemConstScheme item);
    TResultValue PlayCurrentTvEpisode(ui64 channelId, ui64 familyId);
    TResultValue PlayCurrentTvEpisode(const TString& contentId);
    TResultValue PlayVodEpisode(const TString& contentId, TMaybe<NVideo::TVideoItemConstScheme> optionalItem = Nothing());
    TResultValue PlayVodEpisode(NVideo::TVideoItemConstScheme item);

    TResultValue PlayNextTvChannel(NVideo::TVideoItemConstScheme currentItem);
    TResultValue PlayPrevTvChannel(NVideo::TVideoItemConstScheme currentItem);

    /** Sends user's reaction to UGC database and restarts current episode in case of negative reaction
    * @param Item - the video user reacts to
    * @param Reaction - for example "Like", "Dislike", "Skip". First letter is CAPITAL
    */
    TResultValue HandleUserReaction(NVideo::TVideoItemConstScheme item, TStringBuf reaction);
    TResultValue GetPersonalChannelSchedule(NSc::TValue& result);

    TResultValue ShowTvChannelsGallery(TStringBuf projectAlias = "");

private:
    /** Adds User-Agent header, Authorization header, and 'from' param to videohosting request
     */
    void AddClientParamsAndHeaders(NHttpFetcher::TRequestPtr& request) const;
    /** Adds 'service' param to videohosting request
     */
    void AddServiceParamsAndHeaders(NHttpFetcher::TRequestPtr& request) const;

    /** Adds all of the service and client params (see above) to videohosting request
     */
    void PrepareStreamsRequest(NHttpFetcher::TRequestPtr& request) const;

    /** Adds available client codecs into videohosting and billing request
     */
    void AddCodecHeadersIntoStreamRequest(NHttpFetcher::TRequestPtr& request) const;

    /** Adds user identification params to personal channel request. Runs TTvChannelsHelper::PrepareVideohostingRequest()
     */
    bool PreparePersonalChannelRequest(NHttpFetcher::TRequestPtr& request);

    /** Creates request for episodes that start before current time and end after current time
     * @param[in] channels - comma-separated channels IDs
     * @param[in] currentTime - timestamp
     */
    NHttpFetcher::TRequestPtr CreateAllStreamsRequest(TStringBuf channels, ui64 currentTime, NHttpFetcher::IMultiRequest::TRef multirequest = nullptr);

    /** Creates request for limited amount of episodes that end after current time
     * @param[in] channel - channel ID
     * @param[in] currentTime - timestamp
     * @param[in] limit - will request no more than <limit> episodes
     */
    NHttpFetcher::TRequestPtr CreateChannelStreamsRequest(TStringBuf channel, ui64 currentTime, ui16 limit);

    /** Creates request for personal tv-channel episodes
     * that start before current time and end after current time.
     * @param[in] channel - personal channel ID(s)
     * @param[in] currentTime - timestamp
     * @param[in] limit - will request no more than <limit> episodes
     */
    NHttpFetcher::TRequestPtr CreatePersonalChannelStreamsRequest(TStringBuf channel, ui64 currentTime, ui16 limit);
    NHttpFetcher::TRequestPtr CreatePersonalChannelStreamsRequest(TStringBuf channel, ui64 currentTime, NHttpFetcher::IMultiRequest::TRef& multirequest);

    /** Creates request for VOD stream info by given episode id
     * @param episode vh episode content_id or deprecated_content_id
     */
    NHttpFetcher::TRequestPtr CreatePlayerRequest(TStringBuf episode);

    void CollectGalleryItemsFromEpisodesResponse(ui64 currentTime, const NSc::TValue& responseJson, TStringBuf projectAlias, NSc::TValue& galleryItems);
    void CollectGalleryItemsFromEpisodesResponse(ui64 currentTime, const NSc::TValue& responseJson, NSc::TValue& galleryItems);

    /** Builds sorted gallery of available at current time channels
     */
    TResultValue BuildFullTvChannelsGalleryFromCache(NSc::TValue &result);
    TResultValue BuildSpecialTvChannelsGalleryFromCache(TStringBuf projectAlias, NSc::TValue& result);
    TResultValue BuildTvChannelsGallery(NSc::TValue& result, TStringBuf projectAlias = "");

    /** Prepares item and command data
     * @param[in] stream - stream json from /episodes handle response
     * @param[in] currentTime - timestamp to calculate view progress and start offset
     * @param[out] item - channel video item (comes from PlayCurrentTvEpisode method)
     * @param[out] command - command to prepare
     */
    void PreparePlayCommand(const NSc::TValue& stream, ui64 currentTime,
        NVideo::TVideoItemConstScheme& constItem, NVideo::TPlayVideoCommandData& command) const;

    void AddPlayCommand(const NSc::TValue& stream, ui64 currentTime, NVideo::TVideoItemConstScheme& constItem);

    TResultValue PlayOffsetTvChannel(TStringBuf providerItemId, i64 offset, TStringBuf projectAlias);

    /** Sends user's reaction to UGC database
    * @param Item - the video user reacts to
    * @param Reaction - for example "Like", "Dislike", "Skip". First letter is CAPITAL
    */
    void SendReactionToUGCDB(NVideo::TVideoItemConstScheme item, TStringBuf reaction);

    ui64 GetPersonalTvScheduleRequestHistory(NSc::TValue& history);
    void UpdatePersonalTvScheduleRequestHistory(NSc::TValue& history);

private:
    /** Returns TError::EType::SYSTEM if ChannelsInfo is null
     */
    TResultValue CheckChannelsInfoAvailability() const;

    /**
     * Makes request to Blackbox if PassportUID and UserHasYaPlus have not been defined yet
     * @return true if PassportUID and UserHasYaPlus are defined
     */
    bool TryGetUserInfo();

    /**
     * @return true if we can show this channel on this device to this user
     */
    bool CanPlayChannelOnClient(TChannelsInfo::TChannel::TConstPtr channel);


private:
    TContext& Ctx;
    TChannelsInfo::TPtr ChannelsInfo;
    TMaybe<TString> PassportUID;
    TMaybe<bool> UserHasYaPlus;
};

} // namespace NBASS
