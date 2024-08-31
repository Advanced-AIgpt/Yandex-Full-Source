#pragma once

#include <alice/bass/libs/video_common/defs.h>
#include <alice/bass/libs/video_common/video.sc.h>

#include <alice/bass/util/error.h>

#include <alice/library/video_common/defs.h>

#include <library/cpp/scheme/domscheme_traits.h>
#include <library/cpp/scheme/scheme.h>
#include <library/cpp/scheme/util/scheme_holder.h>

#include <util/generic/flags.h>
#include <util/generic/maybe.h>
#include <util/generic/strbuf.h>
#include <util/generic/string.h>
#include <util/generic/variant.h>
#include <util/generic/vector.h>

namespace NBASS {
namespace NVideo {

using TVideoGalleryScheme = NBassApi::TVideoGallery<TSchemeTraits>;
using TVideoGalleryConstScheme = TVideoGalleryScheme::TConst;
using TVideoGallery = TSchemeHolder<TVideoGalleryScheme>;
using TVideoGalleryDebugInfo = TSchemeHolder<NBassApi::TVideoGalleryDebugInfo<TSchemeTraits>>;

using TAvatarMdsImage = NVideoCommon::TAvatarMdsImage;
using TAvatarMdsImageConstScheme = NVideoCommon::TAvatarMdsImageConstScheme;
using TAvatarMdsImageScheme = NVideoCommon::TAvatarMdsImageScheme;

using TVideoItem = NVideoCommon::TVideoItem;
using TVideoItemConstScheme = NVideoCommon::TVideoItemConstScheme;
using TVideoItemScheme = NVideoCommon::TVideoItemScheme;

using TPersonItem = NVideoCommon::TPersonItem;
using TPersonItemConstScheme = NVideoCommon::TPersonItemConstScheme;
using TPersonItemScheme = NVideoCommon::TPersonItemScheme;

using TCollectionItem = NVideoCommon::TCollectionItem;
using TCollectionItemConstScheme = NVideoCommon::TCollectionItemConstScheme;
using TCollectionItemScheme = NVideoCommon::TCollectionItemScheme;

using TLightVideoItemScheme = NBassApi::TLightVideoItem<TSchemeTraits>;
using TLightVideoItemConstScheme = TLightVideoItemScheme::TConst;
using TLightVideoItem = TSchemeHolder<TLightVideoItemScheme>;

using TShowPayScreenCommandDataScheme = NBassApi::TShowPayScreenCommandData<TSchemeTraits>;
using TShowPayScreenCommandDataConstScheme = TShowPayScreenCommandDataScheme::TConst;
using TShowPayScreenCommandData = TSchemeHolder<TShowPayScreenCommandDataScheme>;

using TRequestContentPayloadScheme = NBassApi::TRequestContentPayload<TSchemeTraits>;
using TRequestContentPayloadConstScheme = TRequestContentPayloadScheme::TConst;
using TRequestContentPayload = TSchemeHolder<TRequestContentPayloadScheme>;

using TPlayVideoCommandDataScheme = NBassApi::TPlayVideoCommandData<TSchemeTraits>;
using TPlayVideoCommandDataConstScheme = TPlayVideoCommandDataScheme::TConst;
using TPlayVideoCommandData = TSchemeHolder<TPlayVideoCommandDataScheme>;

using TSeasonsItemScheme = NBassApi::TSeasonsItem<TSchemeTraits>;
using TSeasonsItemConstScheme = TSeasonsItemScheme::TConst;
using TSeasonsItem = TSchemeHolder<TSeasonsItemScheme>;

using TWatchedVideoItemScheme = NBassApi::TWatchedVideoItem<TSchemeTraits>::TConst;
using TWatchedTvShowItemScheme = NBassApi::TWatchedTvShowItem<TSchemeTraits>::TConst;
using TLastWatchedStateScheme = NBassApi::TLastWatchedState<TSchemeTraits>::TConst;

using TShowVideoDescriptionCommandData = TSchemeHolder<NBassApi::TShowVideoDescriptionCommandData<TSchemeTraits>>;

using TSkippableFragmentScheme = NBassApi::TSkippableFragment<TSchemeTraits>;
using TSkippableFragment = TSchemeHolder<TSkippableFragmentScheme>;
using TSkippableFragmentsDeprScheme = NBassApi::TSkippableFragmentsDepr<TSchemeTraits>;
using TSkippableFragmentsDepr = TSchemeHolder<TSkippableFragmentsDeprScheme>;

using TAudioStreamOrSubtitleScheme = NBassApi::TAudioStreamOrSubtitle<TSchemeTraits>;
using TAudioStreamOrSubtitle = TSchemeHolder<TAudioStreamOrSubtitleScheme>;
using TAudioStreamOrSubtitleArrayScheme = NDomSchemeRuntime::TConstArray<TSchemeTraits, NBassApi::TAudioStreamOrSubtitleConst<TSchemeTraits>>;

using NAlice::NVideoCommon::ATTENTION_ALL_RESULTS_FILTERED;
using NAlice::NVideoCommon::ATTENTION_AUTOPLAY;
using NAlice::NVideoCommon::ATTENTION_AUTOSELECT;
using NAlice::NVideoCommon::ATTENTION_DETAILED_DESCRIPTION;
using NAlice::NVideoCommon::ATTENTION_EMPTY_SEARCH_GALLERY;
using NAlice::NVideoCommon::ATTENTION_NON_AUTHORIZED_USER;
using NAlice::NVideoCommon::ATTENTION_NO_GOOD_RESULT;
using NAlice::NVideoCommon::ATTENTION_NO_SUCH_EPISODE;
using NAlice::NVideoCommon::ATTENTION_NO_SUCH_SEASON;
using NAlice::NVideoCommon::ATTENTION_PAID_CONTENT;

inline constexpr TStringBuf ATTENTION_ALREADY_AVAILABLE = "video_already_available";
inline constexpr TStringBuf ATTENTION_BROWSER_GALLERY = "browser_video_gallery";
inline constexpr TStringBuf ATTENTION_CANNOT_REWIND_TV_STREAM = "cannot_rewind_tv_stream";
inline constexpr TStringBuf ATTENTION_CANNOT_REWIND_CAMERA_STREAM = "cannot_rewind_camera_stream";
inline constexpr TStringBuf ATTENTION_FEATURE_NOT_SUPPORTED = "feature_not_supported";
inline constexpr TStringBuf ATTENTION_GALLERY = "video_gallery";
inline constexpr TStringBuf ATTENTION_INDEX_OUT_OF_RANGE = "video_index_out_of_range";
inline constexpr TStringBuf ATTENTION_NO_NEXT_VIDEO = "no_next_video";
inline constexpr TStringBuf ATTENTION_NO_PREV_VIDEO = "no_previous_video";
inline constexpr TStringBuf ATTENTION_NO_TV_IS_PLUGGED_IN = "no_tv_is_plugged_in";
inline constexpr TStringBuf ATTENTION_NO_VIDEO_TO_CONTINUE = "no_video_to_continue";
inline constexpr TStringBuf ATTENTION_NOTHING_IS_PLAYING = "nothing_is_playing";
inline constexpr TStringBuf ATTENTION_PREVNEXT_EPISODE_SEASON_SET = "video_prevnext_episode_season_set";
inline constexpr TStringBuf ATTENTION_REWIND_AFTER_END = "rewind_position_after_end";
inline constexpr TStringBuf ATTENTION_REWIND_BEFORE_BEGIN = "rewind_position_before_begin";
inline constexpr TStringBuf ATTENTION_SEASON_COMING_SOON = "video_season_coming_soon";
inline constexpr TStringBuf ATTENTION_SEASON_GALLERY = "video_season_gallery";
inline constexpr TStringBuf ATTENTION_SEND_PAY_PUSH_DONE = "sent_buy_video_push";
inline constexpr TStringBuf ATTENTION_SEND_PAY_PUSH_FAIL = "video_failed_to_send_push";
inline constexpr TStringBuf ATTENTION_TV_PAYMENT_WITHOUT_PUSH = "video_tv_payment_without_push";
inline constexpr TStringBuf ATTENTION_LEGATUS_PAYMENT_WITHOUT_PUSH = "video_legatus_payment_without_push";
inline constexpr TStringBuf ATTENTION_VIDEO_BOTH_TRACK_TYPES = "video_both_track_types";
inline constexpr TStringBuf ATTENTION_VIDEO_CANNOT_TURN_OFF_SUBTITLES = "video_cannot_turn_off_subtitles";
inline constexpr TStringBuf ATTENTION_VIDEO_COMING_SOON = "video_coming_soon";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_GEO_CONSTRAINT_VIOLATION = "video_error_geo_constraint_violation";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_LICENSES_NOT_FOUND = "video_error_licenses_not_found";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_PURCHASE_EXPIRED = "video_error_purchase_expired";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_PURCHASE_NOT_FOUND = "video_error_purchase_not_found";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_SERVICE_CONSTRAINT_VIOLATION = "video_error_service_constraint_violation";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_SUBSCRIPTION_NOT_FOUND = "video_error_subscription_not_found";
inline constexpr TStringBuf ATTENTION_VIDEO_ERROR_SUPPORTED_STREAMS_NOT_FOUND = "video_error_supported_streams_not_found";
inline constexpr TStringBuf ATTENTION_VIDEO_HAS_SIMILAR_AUDIO_STREAMS = "video_has_similar_audio_streams";
inline constexpr TStringBuf ATTENTION_VIDEO_HAS_SIMILAR_SUBTITLES = "video_has_similar_subtitles";
inline constexpr TStringBuf ATTENTION_VIDEO_IRRELEVANT_NUMBER = "video_irrelevant_number";
inline constexpr TStringBuf ATTENTION_VIDEO_IRRELEVANT_PROVIDER = "video_irrelevant_provider";
inline constexpr TStringBuf ATTENTION_VIDEO_IRRELEVANT_SCREEN_FOR_CHANGE_TRACK = "video_irrelevant_screen_for_change_track";
inline constexpr TStringBuf ATTENTION_VIDEO_IRRELEVANT_CLIENT_FOR_CHANGE_TRACK = "video_irrelevant_client_for_change_track";
inline constexpr TStringBuf ATTENTION_VIDEO_NOT_SKIPPABLE_FRAGMENT = "video_not_skippable_fragment";
inline constexpr TStringBuf ATTENTION_VIDEO_NOT_SUPPORTED = "video_not_supported";
inline constexpr TStringBuf ATTENTION_VIDEO_NO_SUCH_AUDIO_STREAM = "video_no_such_audio_stream";
inline constexpr TStringBuf ATTENTION_VIDEO_NO_SUCH_SUBTITLE = "video_no_such_subtitle";
inline constexpr TStringBuf ATTENTION_VIDEO_NO_ANY_SUBTITLES = "video_no_any_subtitles";
inline constexpr TStringBuf ATTENTION_VIDEO_HOW_LONG_IS_NOT_SUPPORTED = "video_how_long_is_not_supported";
inline constexpr TStringBuf ATTENTION_VIDEO_SHOW_VIDEO_SETTINGS_IS_NOT_SUPPORTED = "video_show_video_settings_is_not_supported";
inline constexpr TStringBuf ATTENTION_VIDEO_SKIP_FRAGMENT_IS_NOT_SUPPORTED = "video_skip_fragment_is_not_supported";

inline const TString YOUTUBE_API_KEY("GOOGLEAPIS_KEY");

inline constexpr TStringBuf PLAYER_RESTRICTION_CONFIG = "playerRestrictionConfig";
inline constexpr TStringBuf SUBTITLES_BUTTON_ENABLE = "subtitlesButtonEnable";

inline constexpr ui32 CHANGE_TRACK_ADDITIONAL_THRESHOLD = 2;
inline constexpr ui32 MAX_FAMILY_SEARCH_AGE = 15;

using EItemType = NVideoCommon::EItemType;
using EVideoGenre = NVideoCommon::EVideoGenre;
using EContentType = NVideoCommon::EContentType;

using TContentTypeFlags = TFlags<EContentType>;
inline constexpr TContentTypeFlags ALL_CONTENT_TYPES = ~TContentTypeFlags();
inline constexpr TContentTypeFlags DEFAULT_CONTENT_TYPES =
    TContentTypeFlags(EContentType::Movie) | TContentTypeFlags(EContentType::TvShow);
inline constexpr TContentTypeFlags VIDEO_CONTENT_TYPES =
    TContentTypeFlags(EContentType::Video) | TContentTypeFlags(EContentType::MusicVideo);
inline constexpr TContentTypeFlags RECOMMENDABLE_CONTENT_TYPES =
    TContentTypeFlags(EContentType::Movie) | TContentTypeFlags(EContentType::Cartoon);

using TSeasonDescriptor = NVideoCommon::TSeasonDescriptor;
using TSerialDescriptor = NVideoCommon::TSerialDescriptor;

enum class EBadArgument { Season, Episode, NoPrevEpisode, NoNextEpisode };

enum class ESpecialSerialNumber {
    Init /* "init" */,
    Last /* "last" */,
    Prev /* "prev" */,
    Next /* "next" */
};

using TSerialIndex = std::variant<ui32, ESpecialSerialNumber>;

inline TMaybe<TSerialIndex> SerialIndexFromNumber(i64 num) {
    return num > 0 ? TMaybe<TSerialIndex>(TSerialIndex(static_cast<ui32>(num - 1))) : Nothing();
}

inline bool IsPrevOrNext(const TSerialIndex& index) {
    return index == TSerialIndex(ESpecialSerialNumber::Prev) || index == TSerialIndex(ESpecialSerialNumber::Next);
}

struct TEpisodeIndex {
    NVideo::TSerialIndex Season;
    NVideo::TSerialIndex Episode;
};

using EScreenId = NAlice::NVideoCommon::EScreenId;
using EVideoAction = NAlice::NVideoCommon::EVideoAction;
using ESelectionAction = NAlice::NVideoCommon::ESelectionAction;

enum ENewVideo {
    NewVideo /* "new_video" */,
};

enum ETopVideo {
    TopVideo /* "top_video" */,
};

enum EFreeVideo {
    FreeVideo /* "free_video" */,
};

using EScreenName = NAlice::NVideoCommon::EScreenName;

using TItemTypeFlags = TFlags<EItemType>;
inline constexpr TItemTypeFlags ALL_ITEM_TYPES = ~TItemTypeFlags();
inline constexpr TItemTypeFlags DEFAULT_ITEM_TYPES =
    TItemTypeFlags(EItemType::TvShow) | TItemTypeFlags(EItemType::Movie);

enum class ESendPayPushMode {
    DontSend /* "dont_send" */,
    SendImmediately /* "send_immediately" */,
    DelaySending /* "delay_sending" */
};

} // namespace NVideo
} // namespace NBASS
