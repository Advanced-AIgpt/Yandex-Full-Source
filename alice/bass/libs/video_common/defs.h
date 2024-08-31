#pragma once

#include <alice/bass/libs/video_content/protos/rows.pb.h>

#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/library/video_common/defs.h>
#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/datetime/base.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>
#include <util/system/compiler.h>
#include <util/system/types.h>

namespace NVideoCommon {

struct TSeasonKey;
struct TSerialKey;

// Тип элемента галереи (влияет на проигрывание и отрисовку в галерее)
enum class EItemType {
    Null = 0 /* "" */,
    Movie = (1ULL << 0) /* "movie" */,
    TvShow = (1ULL << 1) /* "tv_show" */,
    TvShowEpisode = (1ULL << 2) /* "tv_show_episode" */,
    Video = (1ULL << 3) /* "video" */,
    TvStream = (1ULL << 4) /* "tv_stream" */,
    CameraStream = (1ULL << 5) /* "camera_stream" */,
};

using EVideoGenre = NAlice::NVideoCommon::EVideoGenre;
using EContentType = NAlice::NVideoCommon::EContentType;
using EUserContentType = EContentType;

// Inspired by https://wiki.yandex-team.ru/kinopoisk/mobile/kp1fsd/online/watchingrejectionreason/
enum class EPlayError {
    PURCHASE_NOT_FOUND /* "purchase_not_found" */,
    PURCHASE_EXPIRED /* "purchase_expired" */,
    SUBSCRIPTION_NOT_FOUND /* "subscription_not_found" */,
    GEO_CONSTRAINT_VIOLATION /* "geo_constraint_violation" */,
    LICENSES_NOT_FOUND /* "licenses_not_found" */,
    SERVICE_CONSTRAINT_VIOLATION /* "service_constraint_violation" */,
    SUPPORTED_STREAMS_NOT_FOUND /* "supported_streams_not_found" */,
    UNEXPLAINABLE /* "unexplainable" */,
    PRODUCT_CONSTRAINT_VIOLATION /* "product_constraint_violation" */,
    STREAMS_NOT_FOUND /* "streams_not_found" */,
    MONETIZATION_MODEL_CONSTRAINT_VIOLATION /* "monetization_model_constraint_violation" */,
    AUTH_TOKEN_SIGNATURE_FAILED /* "auth_token_signature_failed" */,
    INTERSECTION_BETWEEN_LICENSE_AND_STREAMS_NOT_FOUND /* "intersection_between_license_and_streams_not_found" */,
    UNAUTHORIZED /* "unauthorized" */,
    VIDEOERROR /* "videoerror" */
};

using TAvatarMdsImageScheme = NBassApi::TAvatarMdsImage<TSchemeTraits>;
using TAvatarMdsImageConstScheme = TAvatarMdsImageScheme::TConst;
using TAvatarMdsImage = TSchemeHolder<TAvatarMdsImageScheme>;

using TVideoItemScheme = NBassApi::TVideoItem<TSchemeTraits>;
using TVideoItemConstScheme = TVideoItemScheme::TConst;
using TVideoItem = TSchemeHolder<TVideoItemScheme>;

using TPersonItemScheme = NBassApi::TPersonItem<TSchemeTraits>;
using TPersonItemConstScheme = TPersonItemScheme::TConst;
using TPersonItem = TSchemeHolder<TPersonItemScheme>;

using TCollectionItemScheme = NBassApi::TCollectionItem<TSchemeTraits>;
using TCollectionItemConstScheme = TCollectionItemScheme::TConst;
using TCollectionItem = TSchemeHolder<TCollectionItemScheme>;

using TVideoGalleryScheme = NBassApi::TVideoGallery<TSchemeTraits>;
using TVideoGalleryConstScheme = TVideoGalleryScheme::TConst;
using TVideoGallery = TSchemeHolder<TVideoGalleryScheme>;

using TLightVideoItemScheme = NBassApi::TLightVideoItem<TSchemeTraits>;
using TLightVideoItemConstScheme = TLightVideoItemScheme::TConst;
using TLightVideoItem = TSchemeHolder<TLightVideoItemScheme>;

using NAlice::NVideoCommon::PROVIDER_AMEDIATEKA;
using NAlice::NVideoCommon::PROVIDER_IVI;
using NAlice::NVideoCommon::PROVIDER_KINOPOISK;
using NAlice::NVideoCommon::PROVIDER_OKKO;
using NAlice::NVideoCommon::PROVIDER_STRM;
using NAlice::NVideoCommon::PROVIDER_YAVIDEO;
using NAlice::NVideoCommon::PROVIDER_YAVIDEO_PROXY;
using NAlice::NVideoCommon::PROVIDER_YOUTUBE;

inline constexpr TStringBuf ALL_VIDEO_PROVIDERS[] = {PROVIDER_AMEDIATEKA, PROVIDER_IVI,     PROVIDER_KINOPOISK,
                                                     PROVIDER_OKKO,       PROVIDER_STRM,    PROVIDER_YAVIDEO, PROVIDER_YAVIDEO_PROXY,
                                                     PROVIDER_YOUTUBE};

inline constexpr TStringBuf IVI_APP_VERSION = "7540";

inline constexpr TStringBuf AMEDIATEKA_HOST = "https://www.amediateka.ru";
inline constexpr TStringBuf IVI_HOST = "https://www.ivi.ru";

inline const TString KINOPOISK_DEFAULT_SVOD_TABLE("kinopoisk_svod");

TVector<TStringBuf> GetSupportedVideoProviders();
bool DoesProviderHaveUniqueIdsForItems(TStringBuf providerName);

struct TSeasonDescriptor {
    void Ser(NVideoContent::NProtos::TSeasonDescriptor& s) const;
    [[nodiscard]] bool Des(const NVideoContent::NProtos::TSeasonDescriptor& s);

    [[nodiscard]] bool Ser(const TSeasonKey& key, NVideoContent::NProtos::TSeasonDescriptorRow& row) const;

    bool operator==(const TSeasonDescriptor& rhs) const {
        if (SerialId != rhs.SerialId || Id != rhs.Id || EpisodesCount != rhs.EpisodesCount ||
            EpisodeIds != rhs.EpisodeIds || Index != rhs.Index || ProviderNumber != rhs.ProviderNumber ||
            Soon != rhs.Soon) {
            return false;
        }
        if (EpisodeItems.size() != rhs.EpisodeItems.size())
            return false;

        for (size_t i = 0; i < EpisodeItems.size(); ++i) {
            if (*EpisodeItems[i]->GetRawValue() != *rhs.EpisodeItems[i]->GetRawValue())
                return false;
        }
        return true;
    }

    // Don't forget to update Ser/Des methods when adding/removing
    // fields.
    TString SerialId;
    TMaybe<TString> Id;
    ui32 EpisodesCount = 0;
    TVector<TString> EpisodeIds;
    TVector<TVideoItem> EpisodeItems;
    ui32 Index = 0;
    ui64 ProviderNumber = 0; /* some providers have series only partially */
    bool Soon = false; /* A season not issued yet */

    TMaybe<TInstant> DownloadedAt;
    TMaybe<TInstant> UpdateAt;
};

struct TSerialDescriptor : public TSimpleRefCount<TSerialDescriptor> {
    void Ser(NVideoContent::NProtos::TSerialDescriptor& s) const;
    [[nodiscard]] bool Des(const NVideoContent::NProtos::TSerialDescriptor& s);

    [[nodiscard]] bool Ser(const TSerialKey& key, NVideoContent::NProtos::TSerialDescriptorRow& row) const;

    bool operator==(const TSerialDescriptor& rhs) const {
        return Id == rhs.Id && Seasons == rhs.Seasons && TotalEpisodesCount == rhs.TotalEpisodesCount &&
               MinAge == rhs.MinAge;
    }

    // Don't forget to update Ser/Des methods when adding/removing
    // fields.
    TString Id;
    TVector<TSeasonDescriptor> Seasons;
    ui32 TotalEpisodesCount = 0;
    TMaybe<ui32> MinAge;
};
} // namespace NVideoCommon
