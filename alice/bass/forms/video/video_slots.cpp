#include "video_slots.h"

#include "video_provider.h"

#include <alice/nlu/libs/request_normalizer/request_normalizer.h>

#include <library/cpp/langs/langs.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <util/string/builder.h>

namespace NBASS {
namespace NVideo {

using namespace NAlice::NVideoCommon;

namespace {

template <class TSlot>
void SlotToSearchText(const TSlot& slot, IOutputStream* out) {
    if (!slot.Empty()) {
        *out << slot.GetSourceText();
    }
}

template <>
void SlotToSearchText(const TStringSlot& slot, IOutputStream* out) {
    if (!slot.Empty()) {
        *out << slot.GetString();
    }
}

template <>
Y_DECLARE_UNUSED
void SlotToSearchText(const TSeasonSlot& slot, IOutputStream* out) {
    if (!slot.Defined()) {
        return;
    }

    struct TVisitor {
        TString operator()(ui32 value) const {
            return ToString(value + 1);
        }

        TString operator()(ESpecialSerialNumber value) const {
            switch (value) {
            case ESpecialSerialNumber::Init:
                return TString("первоначальный");
            case ESpecialSerialNumber::Last:
                return TString("последний");
            case ESpecialSerialNumber::Prev:
                return "предыдущий";
            case ESpecialSerialNumber::Next:
                return "следующий";
            }
        }
    };

    *out << std::visit(TVisitor(), slot.GetSerialIndex()) << TStringBuf(" сезон");
}

template <>
Y_DECLARE_UNUSED
void SlotToSearchText(const TEpisodeSlot& slot, IOutputStream* out) {
    if (!slot.Defined()) {
        return;
    }

    struct TVisitor {
        TString operator()(ui32 value) {
            return ToString(value + 1);
        };

        TString operator()(ESpecialSerialNumber value) {
            switch (value) {
            case ESpecialSerialNumber::Init:
                return "первоначальная";
            case ESpecialSerialNumber::Last:
                return "последняя";
            case ESpecialSerialNumber::Prev:
                return "предыдущая";
            case ESpecialSerialNumber::Next:
                return "следующая";
            }
            Y_UNREACHABLE();
        }
    };

    *out << std::visit(TVisitor(), slot.GetSerialIndex()) << TStringBuf(" серия");
}

template <class TSlot>
void PushSlot(const TSlot& slot, TStringBuilder& b) {
    if (!slot.Defined()) {
        return;
    }
    if (b) {
        b << TStringBuf(" ");
    }
    SlotToSearchText(slot, &b.Out);
}

static void PushString(const TString& value, TStringBuilder& b) {
    if (value.empty()) {
        return;
    }
    if (b) {
        b << TStringBuf(" ");
    }
    b << value;
}

bool IsPrevOrNext(const TSlot* slot) {
    if (IsSlotEmpty(slot) || !slot->Value.IsString())
        return false;

    const auto value = slot->Value.GetString();
    return value == "next" || value == "prev";
}

TMaybe<NDatetime::TTimeZone> GetUserTimeZone(const TContext& ctx) {
    TMaybe<TString> timeZone;
    if (ctx.UserLocation()) {
        timeZone = ctx.UserLocation()->UserTimeZone();
    } else if (ctx.Meta().HasTimeZone()) {
        timeZone = TString{*ctx.Meta().TimeZone()};
    }

    if (timeZone) {
        try {
            return NDatetime::GetTimeZone(*timeZone);
        } catch (const NDatetime::TInvalidTimezone& e) {
            LOG(ERR) << e.what() << Endl;
        }
    }

    return Nothing();
}

TMaybe<ui32> GetCurrentYear(const TContext& ctx) {
    const TInstant timeNow = TInstant::Seconds(ctx.Meta().Epoch());
    if (const auto maybeTimeZone = GetUserTimeZone(ctx)) {
        const NDatetime::TSimpleTM civilTime = NDatetime::ToCivilTime(timeNow, *maybeTimeZone);
        return civilTime.RealYear();
    }
    return Nothing();
}

} // namespace anonymous

TReleaseYearSlot::TReleaseYearSlot(const TSlot* slot, const TContext& ctx)
    : TJsonSlot(slot)
{
    TStringBuf type;
    if (!IsSlotEmpty(slot)) {
        type = slot->Type;
    }
    if (type == TStringBuf("date") || type == TStringBuf("custom.year")) {
        ExactYear = slot->Value.ForceIntNumber();
    } else if (type == TStringBuf("year_adjective") || type == TStringBuf("custom.year_adjective")) {
        // Value is either in format `<from>:<to>` (e.g., `2000:2009`), or `<relative year>` (e.g., `-1`)
        i32 from, to;
        TStringBuf buf = slot->Value.GetString();
        if (TryFromString(buf.NextTok(':'), from)) {
            if (TryFromString(buf, to)) {
                if (from % 10 == 0 && from <= std::numeric_limits<i32>::max() - 9 && to == from + 9) {
                    DecadeStartYear = from;
                } else {
                    YearsRange.ConstructInPlace(std::make_pair(from, to));
                }
            } else {
                RelativeYear = from;
                if (const auto maybeCurrentYear = GetCurrentYear(ctx)) {
                    ExactYear = *maybeCurrentYear + from;
                }
            }
        }
    }
}

// TVideoSlots -----------------------------------------------------------------
TVideoSlots::TVideoSlots(const TContext& ctx)
    : SearchText(ctx.GetSlot(SLOT_SEARCH_TEXT))
    , SearchTextRaw(ctx.GetSlot(SLOT_VIDEO_TEXT_RAW))
    , SearchTextText(ctx.GetSlot(SLOT_SEARCH_TEXT_TEXT))
    , OriginalProvider(ctx.GetSlot(SLOT_PROVIDER), !ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_UNBAN_AMEDIATEKA),
                       !ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_UNBAN_IVI))
    , ProviderText(ctx.GetSlot(SLOT_PROVIDER_TEXT))
    , ContentType(ctx.GetSlot(SLOT_CONTENT_TYPE))
    , ContentTypeRaw(ctx.GetSlot(SLOT_VIDEO_TYPE_RAW))
    , ContentTypeText(ctx.GetSlot(SLOT_CONTENT_TYPE_TEXT))
    , VideoGenre(ctx.GetSlot(SLOT_GENRE))
    , VideoGenreText(ctx.GetSlot(SLOT_GENRE_TEXT))
    , Country(ctx.GetSlot(SLOT_COUNTRY))
    , CountryText(ctx.GetSlot(SLOT_COUNTRY_TEXT))
    , ReleaseDate(ctx.GetSlot(SLOT_RELEASE_DATE), ctx)
    , ReleaseDateText(ctx.GetSlot(SLOT_RELEASE_DATE_TEXT))
    , Action(ctx.GetSlot(SLOT_ACTION, SLOT_ACTION_TYPE), EVideoAction::Find)
    , NewVideo(ctx.GetSlot(SLOT_NEW, SLOT_NEW_TYPE))
    , TopVideo(ctx.GetSlot(SLOT_TOP, SLOT_TOP_TYPE))
    , FreeVideo(ctx.GetSlot(SLOT_FREE, SLOT_FREE_TYPE))
    , ProviderOverride(ctx.GetSlot(SLOT_PROVIDER_OVERRIDE))
    , SeasonIndex(ctx.GetSlot(SLOT_SEASON))
    , EpisodeIndex(ctx.GetSlot(SLOT_EPISODE))
    , GalleryNumber(ctx.GetSlot(SLOT_VIDEO_INDEX))
    , SelectionAction(ctx.GetSlot(SLOT_ACTION, SLOT_SELECTION_ACTION_TYPE))
    , ScreenName(ctx.GetSlot(SLOT_SCREEN_NAME, SLOT_SCREEN_NAME_TYPE))
    , ForbidAutoSelect(ctx.GetSlot(SLOT_FORBID_AUTOSELECT))
    , AudioLanguage(ctx.GetSlot(SLOT_AUDIO_LANGUAGE))
    , SubtitleLanguage(ctx.GetSlot(SLOT_SUBTITLE_LANGUAGE))
    , ShouldUseContentTypeForProviderSearch(
          !ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DONT_USE_CONTENT_TYPE_FOR_PROVIDER_SEARCH))
    , ShouldUseTextOnlySlotsInVideoSearch(
           !ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DONT_USE_TEXT_ONLY_SLOTS_IN_SEARCH_REQUESTS))
    , ShouldUseRawSlotsInVideoSearch(
           ctx.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_USE_RAW_SEARCH_TEXT_FRAME))
    , SilentResponse(ctx.GetSlot(SLOT_SILENT_RESPONSE))
{
    if (OriginalProvider.Defined() && !OriginalProvider.GetString().empty()) {
        if (OriginalProvider.GetType() == SLOT_PROVIDER_TYPE &&
            NVideo::IsValidProvider(OriginalProvider.GetString())) {
            FixedProvider = OriginalProvider.GetString();
            ProviderWasChanged = false;
        } else {
            FixedProvider = NVideoCommon::PROVIDER_YOUTUBE;
            ProviderWasChanged = true;
        }
    }
}

// static
TMaybe<TVideoSlots> TVideoSlots::TryGetFromContext(TContext& ctx) {
    const auto* seasonSlot = ctx.GetSlot(SLOT_SEASON);
    const auto* episodeSlot = ctx.GetSlot(SLOT_EPISODE);
    if (!IsSlotEmpty(seasonSlot) && IsPrevOrNext(episodeSlot)) {
        ctx.AddAttention(ATTENTION_PREVNEXT_EPISODE_SEASON_SET);
        return Nothing();
    }

    return TVideoSlots(ctx);
}

TString TVideoSlots::BuildSearchQueryForWeb() const {
    TStringBuilder b;

    PushSlot(TopVideo, b);
    PushSlot(NewVideo, b);
    PushSlot(FreeVideo, b);

    // NOTE (MEGAMIND-281): ContentType can be omitted intentionally because it reduces quality of web search
    // for providers.
    if (ShouldUseContentTypeForProviderSearch) {
        PushSlot(ContentType, b);
    }

    PushSlot(VideoGenre, b);
    PushSlot(Country, b);
    PushSlot(ReleaseDate, b);
    PushSlot(SearchText, b);

    return b;
}

TString TVideoSlots::BuildSearchQueryForInternetVideos() const {
    TStringBuilder b;

    if (ShouldUseRawSlotsInVideoSearch && (ContentTypeRaw.Defined() || SearchTextRaw.Defined())) {
        PushSlot(ContentTypeRaw, b);
        if (SearchTextRaw.Defined()) {
            PushString(NNlu::TRequestNormalizer::Normalize(ELanguage::LANG_RUS, SearchTextRaw.GetString()), b);
        }
        if (b) {
            return b;
        }
    }

    PushSlot(TopVideo, b);
    PushSlot(NewVideo, b);
    PushSlot(FreeVideo, b);

    // NOTE (MEGAMIND-281): ContentType is always pushed for video providers.
    PushSlot(ContentType, b);
    if (ShouldUseTextOnlySlotsInVideoSearch) {
        PushSlot(ContentTypeText, b);
    }

    PushSlot(VideoGenre, b);
    if (ShouldUseTextOnlySlotsInVideoSearch) {
        PushSlot(VideoGenreText, b);
    }
    PushSlot(Country, b);
    if (ShouldUseTextOnlySlotsInVideoSearch) {
        PushSlot(CountryText, b);
    }
    PushSlot(ReleaseDate, b);
    if (ShouldUseTextOnlySlotsInVideoSearch) {
        PushSlot(ReleaseDateText, b);
    }
    PushSlot(SearchText, b);
    if (ShouldUseTextOnlySlotsInVideoSearch) {
        PushSlot(SearchTextText, b);
        PushSlot(ProviderText, b);

    }

    // Season and Episode indexes are only pushed for video search because videos don't have serial descriptors.
    PushSlot(SeasonIndex, b);
    PushSlot(EpisodeIndex, b);

    return b;
}

} // namespace NVideo
} // namespace NBASS
