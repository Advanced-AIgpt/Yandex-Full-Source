#pragma once

#include <alice/bass/forms/context/fwd.h>

#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/bass/libs/globalctx/fwd.h>
#include <alice/bass/libs/source_request/source_request.h>

#include <alice/bass/util/error.h>

#include <alice/library/geo/geodb.h>

#include <util/datetime/base.h>
#include <util/generic/ptr.h>
#include <util/system/spinlock.h>

namespace NBASS {

class TChannelsInfo : public TThrRefBase {
public:
    using TPtr = TIntrusivePtr<TChannelsInfo>;

    class TChannel : public TSimpleRefCount<TChannel> {
    public:
        using TPtr = TIntrusivePtr<TChannel>;
        using TConstPtr = TIntrusiveConstPtr<TChannel>;

        explicit TChannel(const NSc::TValue& channelJson);
        NSc::TValue ToVideoItemJson() const;

        bool IsRegionSupported(const NGeobase::TLookup& geobase, NGeobase::TId userRegion) const;
        bool IsChannelStatusAllowed() const;
        bool IsPersonal() const;
        bool IsRestreamedChannel() const;
        bool IsSubscriptionChannel() const;

        const TString& GetContentId() const;
        ui64 GetChannelId() const;
        ui64 GetFamilyId() const;

        const TString& GetName() const;

        const TString& GetProviderName() const;

    private:
        TChannel() = default;

    private:
        TString ContentId;
        ui64 ChannelId;
        ui64 FamilyId;
        TString Name;
        TString Description;
        TString ProviderName;
        TString Thumbnail;
        TString Type;
        bool IsDeepHD;
        bool IsStatusAllowed;
        bool IsSubscriptionNeeded;
        ui64 Weight;
        TVector<NGeobase::TId> Regions;
    };

    TResultValue RequestChannelsInfo(const TSourceRequestFactory& sourceRequestFactory, TStringBuf serviceId, const TString& providerNameFilter);

    const TChannel::TConstPtr GetInfoByChannelId(ui64 channelId) const {
        const auto channel = ChannelIdToInfo.find(channelId);
        return channel ? channel->second : nullptr;
    }

    const TChannel::TConstPtr GetInfoByFamilyId(ui64 familyId) const {
        const auto channel = FamilyIdToInfo.find(familyId);
        return channel ? channel->second : nullptr;
    }

    const TChannel::TConstPtr GetInfoByUuid(TStringBuf contentId) const {
        const auto channel = UuidToInfo.find(contentId);
        return channel ? channel->second : nullptr;
    }

    const TString& GetAvailableChannels() const {
        return AvailableChannels;
    };

    const TString& GetPersonalChannels() const {
        return PersonalChannels;
    }

    const TString& GetRestreamedChannels() const {
        return RestreamedChannels;
    }

    const TString GetChannelsByProjectAlias(TStringBuf projectAlias) const {
        return SpecialProjects.contains(projectAlias) ? SpecialProjects.at(projectAlias) : "";
    }

    size_t ChannelsCount() const {
        return UuidToInfo.size();
    }

private:
    THashMap <TString, TChannel::TPtr> UuidToInfo;
    THashMap <ui64, TChannel::TPtr> ChannelIdToInfo;
    THashMap <ui64, TChannel::TPtr> FamilyIdToInfo;

    TString AvailableChannels;
    TString PersonalChannels;
    TString RestreamedChannels;

    THashMap <TString, TString> SpecialProjects;
};

class TTvChannelsInfoCache : NNonCopyable::TNonCopyable {
public:
    TChannelsInfo::TPtr Data();
    bool IsExpired() const;

    // This object will be available after init is done.
    static TTvChannelsInfoCache* Instance();

    // Runs in application init stage.
    static void Init(IGlobalContext& globalCtx);

private:
    TTvChannelsInfoCache(IGlobalContext& globalCtx);

    TDuration DoUpdate();

private:
    TDummySourcesRegistryDelegate SourcesRegistryDelegate;
    TSourceRequestFactory SourceRequestFactory;
    const TDuration RefreshTime;
    TInstant LastUpdate;

    TAdaptiveLock Lock;
    TChannelsInfo::TPtr SavedData;

    static THolder<TTvChannelsInfoCache> Self;
};

} // namespace NBASS
