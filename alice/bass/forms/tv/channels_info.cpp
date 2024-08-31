#include "channels_info.h"
#include "defs.h"

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/scheduler/scheduler.h>
#include <alice/bass/libs/video_common/defs.h>
#include <alice/library/video_common/video_helper.h>

#include <alice/library/video_common/restreamed_data/restreamed_data.h>

namespace NBASS {

namespace {

using TChannel = TChannelsInfo::TChannel;

constexpr TStringBuf PERSONAL_TV_CHANNEL_CONTENT_ID = "4461546c4debdcffbab506fd75246e19";
constexpr TStringBuf PERSONAL_TV_CHANNEL_TYPE = "personal";
constexpr ui64 PERSONAL_TV_CHANNEL_WEIGHT = 100500;

bool IsChannelStatusAllowed(const NSc::TValue& channel) {
    static const THashSet<TStringBuf> disabledStatuses =
        {TStringBuf("deleted"), TStringBuf("disabled"), TStringBuf("hidden")};
    static const TStringBuf publishedStatus = "published";

    bool isDisabled = false;
    bool isPublished = false;

    for (const auto& status : channel["status"].GetArray()) {
        TStringBuf channelStatus = status.GetString();
        if (disabledStatuses.contains(channelStatus)) {
            isDisabled = true;
            break;
        }
        if (channelStatus == publishedStatus) {
            isPublished = true;
        }
    }

    return !isDisabled && isPublished;
}

bool IsDeepHDStream(const NSc::TValue& streamJson) {
    const NSc::TArray& streamOptions = streamJson.TrySelect("options").GetArray();
    for (size_t i = 0; i < streamOptions.size(); ++i) {
        if (streamOptions[i].GetString() == "hires")
            return true;
    }
    return false;
}

bool IsDeepHDChannel(const NSc::TValue& channelJson) {
    const NSc::TArray& streams = channelJson.TrySelect("streams").GetArray();
    for (size_t i = 0; i < streams.size(); ++i) {
        if (IsDeepHDStream(streams[i]))
            return true;
    }
    return false;
}

void AddContentIdToList(TStringBuf contentId, TString& list) {
    if (list) {
        list += ',';
    }
    list += contentId;
}

} // namespace

TChannel::TChannel(const NSc::TValue& channelJson) {
    ContentId = channelJson["content_id"].GetString();
    ChannelId = channelJson["channel_id"].GetIntNumber();
    FamilyId = channelJson["family_id"].GetIntNumber();

    Name = channelJson["title"];
    Description = channelJson["description"];
    Thumbnail = NAlice::NVideoCommon::BuildThumbnailUri(channelJson["thumbnail"]);

    Type = channelJson["channel_type"];
    Weight = channelJson.TrySelect("meta/weight").GetIntNumber();

    IsDeepHD = IsDeepHDChannel(channelJson);
    IsStatusAllowed = NBASS::IsChannelStatusAllowed(channelJson);

    for (const auto& region : channelJson["region_ids"].GetArray()) {
        Regions.push_back(static_cast<NGeobase::TId>(region.GetIntNumber()));
    }

    // personal channel
    // TODO remove this logic after VH-9124,VH-8861
    if (ContentId == PERSONAL_TV_CHANNEL_CONTENT_ID) {
        Weight = PERSONAL_TV_CHANNEL_WEIGHT;
    }

    IsSubscriptionNeeded = !channelJson.TrySelect("ya_plus").GetArray().empty();

    ProviderName = channelJson.TrySelect("auto_fields/channel_provider").GetString();
    if (ProviderName.empty()) {
        ProviderName = ToString(IsRestreamedChannel() ? NAlice::NVideoCommon::PROVIDER_YAVIDEO_PROXY : NAlice::NVideoCommon::PROVIDER_STRM);
    }
}

NSc::TValue TChannel::ToVideoItemJson() const {
    NSc::TValue item;
    item["provider_item_id"] = ContentId;
    item["provider_name"] = GetProviderName();
    item["type"] = ToString(NVideoCommon::EItemType::TvStream);

    // TODO remove when possible (see ASSISTANT-2850). Still here for tests
    item["channel_type"] = Type;
    item["tv_stream_info"]["channel_type"] = Type;
    item["tv_stream_info"]["deep_hd"] = IsDeepHD;
    item["tv_stream_info"]["is_personal"] = IsPersonal();

    item["name"] = Name;
    item["description"] = Description;

    item["thumbnail_url_16x9_small"] = Thumbnail;
    item["thumbnail_url_16x9"] = Thumbnail;

    item["relevance"] = Weight;

    return item;
}

bool TChannel::IsRegionSupported(const NGeobase::TLookup& geobase, NGeobase::TId userRegion) const {
    if (!NAlice::IsValidId(userRegion)) {
        return false;
    }
    for (const auto& region : Regions) {
        if (geobase.IsIdInRegion(userRegion, region)) {
            return true;
        }
    }

    return false;
}

bool TChannel::IsChannelStatusAllowed() const {
    return IsStatusAllowed;
}

bool TChannel::IsPersonal() const {
    return Type == PERSONAL_TV_CHANNEL_TYPE;
}

bool TChannel::IsRestreamedChannel() const {
    return Type == NTvCommon::QUASAR_PROXY_CHANNEL_TYPE;
}

bool TChannel::IsSubscriptionChannel() const {
    return IsSubscriptionNeeded;
}

const TString& TChannel::GetContentId() const {
    return ContentId;
}

ui64 TChannel::GetChannelId() const {
    return ChannelId;
}

ui64 TChannel::GetFamilyId() const {
    return FamilyId;
}

const TString& TChannel::GetName() const {
    return Name;
}

const TString& TChannel::GetProviderName() const {
    return ProviderName;
}

TResultValue TChannelsInfo::RequestChannelsInfo(const TSourceRequestFactory& sourceRequestFactory,
                                                TStringBuf serviceId = NTvCommon::QUASAR_SERVICE_ID,
                                                const TString& providerNameFilter = "")
{
    AvailableChannels.clear();
    UuidToInfo.clear();
    ChannelIdToInfo.clear();
    FamilyIdToInfo.clear();

    NHttpFetcher::TRequestPtr channelsRequest = sourceRequestFactory.Request();
    channelsRequest->AddCgiParam(TStringBuf("service"), serviceId);

    NHttpFetcher::TResponse::TRef response = channelsRequest->Fetch()->Wait();
    if (response->IsError()) {
        return TError(TError::EType::VIDEOERROR, response->GetErrorText());
    }

    const NSc::TValue json = NSc::TValue::FromJson(response->Data);
    for (const auto& channelJson : json["set"].GetArray()) {
        TChannel::TPtr channel = MakeIntrusive<TChannel>(channelJson);

        TStringBuf contentId = channelJson.TrySelect("content_id").GetString();
        if (!providerNameFilter.empty() && channel->GetProviderName() != providerNameFilter) {
            continue;
        }
        TString& channelsList = channel->IsPersonal() ? PersonalChannels : AvailableChannels;
        AddContentIdToList(contentId, channelsList);

        UuidToInfo.emplace(contentId, channel);

        const NSc::TValue& channelId = channelJson.TrySelect("channel_id");
        if (!channelId.IsNull()) {
            ChannelIdToInfo.emplace(channelId.GetIntNumber(), channel);
        }

        const NSc::TValue& familyId = channelJson.TrySelect("family_id");
        if (!familyId.IsNull()) {
            FamilyIdToInfo.emplace(familyId.GetIntNumber(), channel);
        }
    }

    static const TVector<TStringBuf> specialProjectsAliases = {TStringBuf("yanhl")};

    for (const auto& project: specialProjectsAliases) {
        NHttpFetcher::TRequestPtr channelsRequest = sourceRequestFactory.Request();
        channelsRequest->AddCgiParam(TStringBuf("service"), serviceId);
        channelsRequest->AddCgiParam(TStringBuf("project_alias"), project);

        NHttpFetcher::TResponse::TRef response = channelsRequest->Fetch()->Wait();
        if (response->IsError()) {
            return TError(TError::EType::VIDEOERROR, response->GetErrorText());
        }

        TString& projectChannels = SpecialProjects[project];
        const NSc::TValue json = NSc::TValue::FromJson(response->Data);
        for (const auto &channelJson : json["set"].GetArray()) {
            TStringBuf contentId = channelJson.TrySelect("content_id").GetString();
            AddContentIdToList(contentId, projectChannels);
        }
    }

    const auto& restreamedChannelsMap = NAlice::NVideoCommon::TRestreamedChannelsData::Instance().GetRestreamedChannelsMap();
    for (const auto& channelInfo : restreamedChannelsMap) {
        TChannel::TPtr channel = MakeIntrusive<TChannel>(channelInfo.second);

        UuidToInfo.emplace(channel->GetContentId(), channel);
        ChannelIdToInfo.emplace(channel->GetChannelId(), channel);
        FamilyIdToInfo.emplace(channel->GetFamilyId(), channel);

        AddContentIdToList(channel->GetContentId(), RestreamedChannels);
    }
    LOG(DEBUG) << "Restreamed channels info updated in cache: " << RestreamedChannels << Endl;

    return TResultValue();
}

THolder<TTvChannelsInfoCache> TTvChannelsInfoCache::Self;

TTvChannelsInfoCache::TTvChannelsInfoCache(IGlobalContext& globalCtx)
    : SourceRequestFactory(globalCtx.Sources().VideoHostingTvChannels, globalCtx.Config(), /* path= */ Nothing(),
                           SourcesRegistryDelegate)
    , RefreshTime(globalCtx.Config().CacheUpdatePeriod())
{
}

TIntrusivePtr<TChannelsInfo> TTvChannelsInfoCache::Data() {
    with_lock (Lock)
    {
        return SavedData;
    }
}

bool TTvChannelsInfoCache::IsExpired() const {
    return (TInstant::Now() - LastUpdate) > (RefreshTime + TDuration::Minutes(1));
}

// static
TTvChannelsInfoCache* TTvChannelsInfoCache::Instance() {
    Y_ENSURE(Self, "TTvChannelsInfoCache::Init has not been called");
    return Self.Get();
}

// static
void TTvChannelsInfoCache::Init(IGlobalContext& globalCtx) {
    Self.Reset(new TTvChannelsInfoCache(globalCtx));
    auto onUpdate = []() {
        Y_ASSERT(Self);
        return Self->DoUpdate();
    };
    globalCtx.Scheduler().Schedule(onUpdate);
}


TDuration TTvChannelsInfoCache::DoUpdate() {
    TIntrusivePtr<TChannelsInfo> newChannelsInfo = MakeIntrusive<TChannelsInfo>();
    if (TResultValue error = newChannelsInfo->RequestChannelsInfo(SourceRequestFactory)) {
        LOG(ERR) << "Failed to update TV-channels cache: " << error->Msg << Endl;
        return RefreshTime;
    }

    LastUpdate = TInstant::Now();
    with_lock(Lock) {
        SavedData = std::move(newChannelsInfo);
    }

    LOG(DEBUG) << "TV-channels info updated in cache: " << SavedData->ChannelsCount()  << " channels total" << Endl;

    return RefreshTime;
}


} // namespace NBASS
