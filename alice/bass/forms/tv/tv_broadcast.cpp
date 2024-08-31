#include "tv_broadcast.h"
#include "defs.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/search/search.h>
#include <alice/bass/forms/tv/tv_helper.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/avatars/avatars.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>
#include <alice/library/datetime/datetime.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/video_common/restreamed_data/restreamed_data.h>

#include <library/cpp/scheme/scheme.h>
#include <library/cpp/timezone_conversion/convert.h>

#include <search/idl/meta.pb.h>
#include <search/session/compression/report.h>

#include <util/datetime/base.h>
#include <util/generic/hash.h>
#include <util/generic/ptr.h>
#include <util/string/builder.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/quote/quote.h>

using NAlice::TDateTime;
using NAlice::TDateTimeList;

namespace NBASS {

namespace {
constexpr TStringBuf TV_BROADCAST = "personal_assistant.scenarios.tv_broadcast";
constexpr TStringBuf TV_BROADCAST_ELLIPSIS = "personal_assistant.scenarios.tv_broadcast__ellipsis";
constexpr TStringBuf TV_STREAM = "personal_assistant.scenarios.tv_stream";
constexpr TStringBuf TV_STREAM_ELLIPSIS = "personal_assistant.scenarios.tv_stream__ellipsis";

constexpr TStringBuf SLOT_NAME_ANSWER = "answer";
constexpr TStringBuf SLOT_NAME_BROADCAST_LOCATION = "broadcast_location";
constexpr TStringBuf SLOT_NAME_CHANNEL = "channel";
constexpr TStringBuf SLOT_NAME_DAY_PART = "day_part";
constexpr TStringBuf SLOT_NAME_GENRE = "genre";
constexpr TStringBuf SLOT_NAME_PROGRAM = "program";
constexpr TStringBuf SLOT_NAME_SCHEDULE_MARKER = "schedule_marker";
constexpr TStringBuf SLOT_NAME_WHEN = "when";
constexpr TStringBuf SLOT_NAME_WHERE = "where";
constexpr TStringBuf SLOT_TYPE_ANSWER = "broadcast";
constexpr TStringBuf SLOT_TYPE_BROADCAST_LOCATION = "geo";
constexpr TStringBuf SLOT_TYPE_CHANNEL = "string";
constexpr TStringBuf SLOT_TYPE_CHANNEL_FIXLIST = "tv_channel_fixlist";
constexpr TStringBuf SLOT_TYPE_CHANNEL_RESTREAMED = "tv_channel_restreamed";
constexpr TStringBuf SLOT_TYPE_CHANNEL_SUGGEST = "tv_channel_suggest";
constexpr TStringBuf SLOT_TYPE_DAY_PART = "day_part";
constexpr TStringBuf SLOT_TYPE_WHEN = "datetime_range_raw";
constexpr TStringBuf SLOT_TYPE_WHERE = "string";
constexpr TStringBuf SLOT_VALUE_TV_SCHEDULE = "tv_schedule";

const char DATETIME_FORMAT[] = "%Y-%m-%dT%H:%M:%S%Ez";

constexpr TStringBuf CHANNELS_LIMIT = "20";
constexpr TStringBuf PROGRAMS_LIMIT = "10";
constexpr TStringBuf EVENTS_LIMIT = "5";

constexpr TStringBuf COMMON_STREAM_URL = "https://yandex.ru/efir?channels_category=all&stream_active=channels-list&from_block=alice_online";
constexpr TStringBuf CHANNEL_STREAM_URL = "https://tv.yandex.ru/channel/";
constexpr TStringBuf CHANNEL_STREAM_FROM_BLOCK = "/stream?from_block=alice_online";
constexpr TStringBuf SCHEDULE_URL = "https://tv.yandex.ru/";
constexpr TStringBuf PROGRAM_URL = "https://tv.yandex.ru/program/";
constexpr TStringBuf PROGRAM_UTM = "&utm_source=alice&utm_content=tvprogram";

constexpr TDuration SUFFICIENT_DURATION = TDuration::Hours(12); // если нет сложного сочетания слотов, расписания на 12 часов достаточно
constexpr TDuration DAY_PART_DURATION = TDuration::Hours(6);
constexpr TDuration MAX_AVAILABLE_DURATION = TDuration::Days(11); // в четверг появляется расписание на следующую неделю

// suggests
constexpr TStringBuf CHANNEL_PLAY_SUGGEST = "tv_channel_play";
constexpr TStringBuf CHANNEL_SUGGEST = "tv_channel";
constexpr TStringBuf DAY_PART_SUGGEST = "tv_day_part";
constexpr TStringBuf TV_BROADCAST_SUGGEST = "tv_broadcast";
constexpr TStringBuf TV_STREAM_SUGGEST = "tv_stream";

// Hack for marketing RK: DIALOG-4085
constexpr TStringBuf EXP_FLAG_OHOTNIK_AND_RYBOLOV = "tv_rybolov_rk";
constexpr TStringBuf CHANNEL_OHOTNIK_AND_RYBOLOV_OLD = "Охотник и Рыболов";
constexpr TStringBuf CHANNEL_OHOTNIK_AND_RYBOLOV_NEW = "Охотник и Рыболов Int";

constexpr TStringBuf PERSONAL_TV_CHANNEL_NAME = "yachan";
constexpr TStringBuf PERSONAL_CHANNEL_FALLBACK_URL = "https://yandex.ru/efir?stream_channel=1550142789&from_block=alicepp";

constexpr TStringBuf NHL_TV_CHANNEL_NAME = "yanhl";
constexpr TStringBuf NHL_FALLBACK_URL = "https://nhl.yandex.ru?from_block=alicepp";

constexpr TStringBuf NY2020_TV_CHANNEL_NAME = "ny2020";
constexpr TStringBuf NY2020_TV_CHANNEL_TITLE = "Новогодний 2020";
constexpr ui64 NY2020_TV_CHANNEL_ID = 1544590981;

constexpr TStringBuf CONCERTS_TV_CHANNEL_NAME = "concerts";
constexpr TStringBuf CONCERTS_TV_CHANNEL_TITLE = "Концерты";
constexpr ui64 CONCERTS_TV_CHANNEL_ID = 1585050586;

constexpr TStringBuf THEATERS_TV_CHANNEL_NAME = "theaters";
constexpr TStringBuf THEATERS_TV_CHANNEL_TITLE = "Театры";
constexpr ui64 THEATERS_TV_CHANNEL_ID = 1585050710;

const std::pair<TStringBuf, TStringBuf> SUGGEST_CHANNELS[] = {
        {TStringBuf("МУЗ-ТВ"), TStringBuf("55")},
        {TStringBuf("Неизвестная Планета"), TStringBuf("100")},
        {TStringBuf("Любимое.ТВ"), TStringBuf("119")},
        {TStringBuf("РБК"), TStringBuf("18")},
        {TStringBuf("Ю"), TStringBuf("40")},
        {TStringBuf("Старт"), TStringBuf("103")}
};

const TStringBuf DAY_PARTS[] = {
        TStringBuf("evening"),
        TStringBuf("morning"),
        TStringBuf("day"),
        TStringBuf("night")
};

TStringBuf VinsToTvGenreId(const TStringBuf genre) {
    static const THashMap<TStringBuf, TStringBuf> mapping = {
        { "films", "5" },
        { "series", "4" },
        { "sport", "7" },
        { "children", "3" },
    };
    auto it = mapping.find(genre);
    return (it != mapping.end()) ? it->second : TStringBuf("0");
}

TStringBuf VinsToTvGenre(const TStringBuf genre) {
    static const THashMap<TStringBuf, TStringBuf> mapping = {
        { "films", "films" },
        { "series", "series" },
        { "sport", "sport" },
        { "children", "for-children" },
    };
    auto it = mapping.find(genre);
    return (it != mapping.end()) ? it->second : TStringBuf("0");
}

using TChannel = TChannelsInfo::TChannel;
NSc::TValue GetPersonalChannelInfo() {
    TChannel::TConstPtr personalChannel = TTvChannelsHelper::GetPersonalTvChannelInfo();
    NSc::TValue result;
    if (personalChannel) {
        result["title"] = personalChannel->GetName();
        result["channelId"] = personalChannel->GetChannelId();
        result["familyId"] = personalChannel->GetFamilyId();
    }
    return result;
}

} // namespace

class TTvBroadcastRequestImpl {
public:
    explicit TTvBroadcastRequestImpl(TRequestHandler& r)
        : Ctx(r.Ctx())
        , RequestedCity(Ctx, SLOT_NAME_WHERE)
        , RespondedGeoId(NGeobase::UNKNOWN_REGION) {
    }

    TResultValue Do();

private:
    TResultValue ProcessTvGeo();
    TResultValue ProcessDateTime(const TDateTime& userTime);

    TResultValue PersonalTvChannelScenario();
    TResultValue NHLTvChannelsScenario();

    TResultValue TvBroadcastCommonScenario(const TDateTime& userTime);
    TResultValue TvStreamStationScenario();

    void FinishUpGeo();
    void AddDivCards();
    // writes error and all suggests which comes with the given error
    void WriteError(const TError& error);
    void WriteSuggests();

    bool HasScheduleInRequest();

private:
    void CreateCommonRequest(const NHttpFetcher::TRequestPtr& request);
    TResultValue RequestTvGeo();

    TMaybe<NSc::TValue> CheckChannelStream(const TStringBuf familyId, const TStringBuf yacFamilyId);
    NSc::TValue ChooseThumb(const NSc::TValue& images);

    NHttpFetcher::THandle::TRef CreateTvStreamRequest(NHttpFetcher::IMultiRequest::TRef& multiRequest);
    TResultValue ParseTvStreamResponse(NHttpFetcher::TResponse::TRef handler);
    NHttpFetcher::THandle::TRef CreateTvIdRequest(const TStringBuf type, NHttpFetcher::IMultiRequest::TRef& multiRequest);
    NHttpFetcher::THandle::TRef CreateTvIdSuggestRequest(const TStringBuf type, NHttpFetcher::IMultiRequest::TRef& multiRequest);
    TResultValue ParseTvIdResponse(const TStringBuf type, NHttpFetcher::TResponse::TRef handler);
    TResultValue RequestTvIds();

    NHttpFetcher::THandle::TRef CreateTvScheduleRequest(
        const TDateTime& userTime,
        const TDateTimeList& dtl,
        const bool ignoreChannel,
        const bool ignoreTime,
        NHttpFetcher::IMultiRequest::TRef& multiRequest
    );
    TResultValue ParseTvScheduleResponse(NHttpFetcher::TResponse::TRef handler);
    TResultValue RequestTvSchedule(const TDateTime& userTime, const TDateTimeList& dtl);

    TResultValue GetTvUrl(const TDateTimeList& dtl);

private:
    TContext& Ctx;

    TRequestedGeo RequestedCity;
    // Geo which comes from the answer of tv back
    NGeobase::TId RespondedGeoId;

    std::unique_ptr<TDateTimeList> DateTime;

    NSc::TValue Result;
    NSc::TValue StreamList;
};

TResultValue TTvBroadcastRequestImpl::ProcessTvGeo() {
    if (RequestedCity.HasError()) {
        TContext::TSlot* slot = Ctx.GetSlot(SLOT_NAME_WHERE);
        if (IsSlotEmpty(slot)) {
            Ctx.CreateSlot(SLOT_NAME_WHERE, SLOT_TYPE_WHERE, /* optional */ false);
        }
        return RequestedCity.GetError();
    }

    if (const auto error = RequestTvGeo()) {
        return error;
    }

    return TResultValue();
}

TResultValue TTvBroadcastRequestImpl::ProcessDateTime(const TDateTime& userTime) {
    // Time and date and ranges
    TContext::TSlot* whenSlot = Ctx.GetSlot(SLOT_NAME_WHEN);
    TContext::TSlot* dayPartSlot = Ctx.GetSlot(SLOT_NAME_DAY_PART);

    try {
        DateTime = TDateTimeList::CreateFromSlot(
                whenSlot,
                dayPartSlot,
                userTime,
                { 11, true } // max schedule duration in days, LookForward
        );
    } catch (const yexception& e) {
        return TError(TError::EType::INVALIDPARAM, e.what());
    }

    Y_ASSERT(DateTime);
    auto dateTimeBegin = DateTime->cbegin();

    if (!DateTime->IsNow() && dateTimeBegin->OffsetWidth(userTime) && IsSlotEmpty(dayPartSlot)) {
        TContext::TSlot* dayPartSlotNew = Ctx.CreateSlot("day_part_changed", "day_part", /* optional */ true, "morning");
        Result["day_part_changed"] = "morning";
        try {
            DateTime = TDateTimeList::CreateFromSlot(
                    whenSlot,
                    dayPartSlotNew,
                    userTime,
                    { 11, true } // max schedule duration in days, LookForward
            );
        } catch (const yexception& e) {
            return TError(TError::EType::INVALIDPARAM, e.what());
        }
    }

    Y_ASSERT(DateTime);
    dateTimeBegin = DateTime->cbegin();

    if (!IsSlotEmpty(whenSlot)
        && (TStringBuf("datetime_range") == whenSlot->Type || TStringBuf("datetime_range_raw") == whenSlot->Type)
    ) {
        if (IsSlotEmpty(dayPartSlot)) {
            Result["date_type"] = "range";
        } else {
            Result["date_type"] = "range_day_part";
        }

    } else if (DateTime->IsNow() || (!dateTimeBegin->OffsetWidth(userTime) && IsSlotEmpty(dayPartSlot))) {
        Result["date_type"] = "now";
    } else {
        if (IsSlotEmpty(dayPartSlot) || dayPartSlot->Value.GetString() == "whole_day") {
            Result["date_type"] = "one_day";
        } else {
            Result["date_type"] = "day_part";
        }
    }

    if (DateTime->IsNow()
        || (!dateTimeBegin->OffsetWidth(userTime) && IsSlotEmpty(dayPartSlot))
        || ((dateTimeBegin->DayPart() == userTime.DayPart())
            && (dateTimeBegin->SplitTime().ToString("%F") == userTime.SplitTime().ToString("%F")))
    ) {
        Result["start_now"] = true;
    }
    Result["user_day_part"] = userTime.DayPartAsString();

    Result["tz"] = Ctx.GlobalCtx().GeobaseLookup().GetTimezoneName(RespondedGeoId);
    Result["usertz"] = Ctx.UserTimeZone();
    Result["epoch"] = Ctx.Meta().Epoch();
    Result["user_time"] = userTime.SplitTime().ToString(DATETIME_FORMAT);
    if (!Result.Has("start_now")) {
        Result["start_time"] = dateTimeBegin->SplitTime().ToString(DATETIME_FORMAT);
    }

    return TResultValue();
}

TResultValue TTvBroadcastRequestImpl::TvStreamStationScenario() {
    TTvChannelsHelper tvHelper(Ctx);
    // user specifies channel and we've found it
    const NSc::TValue& channel = Result.TrySelect("channel");
    if (!channel.IsNull()) {
        ui64 channelId = channel.TrySelect("channelId").ForceIntNumber();
        ui64 familyId = channel.Has("yacFamilyId") ?
                        channel.TrySelect("yacFamilyId").ForceIntNumber() :
                        channel.TrySelect("familyId").ForceIntNumber();
        if (Ctx.HasExpFlag(NAlice::NExperiments::EXPERIMENTAL_FLAG_PLAY_CHANNEL_BY_NAME)) {
            Ctx.AddIrrelevantAttention(
                /* relevantIntent= */ TStringBuf("personal_assistant.scenarios.tv_stream"),
                /* reason= */ TStringBuf("https://st.yandex-team.ru/VIDEOFUNC-473"));
            return {};
        }
        return tvHelper.PlayCurrentTvEpisode(channelId, familyId);
    }
    if (Ctx.HasExpFlag(EXPERIMENTAL_FLAG_SHOW_TV_CHANNELS_GALLERY_IS_ACTIVE)) {
        return TResultValue();
    }
    // general case [what's on tv now], [turn on tv]
    // + [show <tv_program> on tv], [show <genre> on tv]
    return tvHelper.ShowTvChannelsGallery();
}

TResultValue TTvBroadcastRequestImpl::PersonalTvChannelScenario() {
    Ctx.AddAttention(TStringBuf("personal_tv_channel_in_request"));

    // Can show personal channel == can talk about it's schedule
    // response with a stub otherwise
    if (!TTvChannelsHelper::CanShowTvStreamOnClient(Ctx)) {
        Ctx.AddAttention(TStringBuf("personal_tv_channel_not_supported"));
        if (Ctx.ClientFeatures().SupportsOpenLink()) {
            NSc::TValue data;
            data["uri"] = PERSONAL_CHANNEL_FALLBACK_URL;
            Ctx.AddSuggest(TStringBuf("personal_tv_channel_fallback_to_service"), data);
        } else {
            Ctx.AddAttention(TStringBuf("cant_find_channel"));
        }

        return TResultValue();
    }

    NSc::TValue personalChannel = GetPersonalChannelInfo();
    if (personalChannel.IsNull()) {
        LOG(ERR) << "No personal channel found in TV-cache" << Endl;
        return TError(TError::EType::TVERROR, TStringBuf("Failed to get personal TV channel info"));
    }

    Result[SLOT_NAME_CHANNEL] = personalChannel;

    // user is not interested in schedule or has no schedule exp-flag
    if (!HasScheduleInRequest() || !Ctx.HasExpFlag("personal_tv_schedule")) {
        return TvStreamStationScenario();
    }

    NSc::TValue personalSchedule;
    personalSchedule["channel"] = GetPersonalChannelInfo();
    TTvChannelsHelper tvHelper = TTvChannelsHelper(Ctx);
    if (TResultValue err = tvHelper.GetPersonalChannelSchedule(personalSchedule)) {
        return err;
    }

    if (!personalSchedule["events"].ArrayEmpty()) {
        Result["schedule"].GetArrayMutable().push_back(personalSchedule);
    }

    // user specified time and it was not current day part
    if (!Result.TrySelect("start_now").GetBool(false)) {
        Ctx.AddAttention(TStringBuf("personal_tv_schedule_future"));
    }

    return TResultValue();
}

TResultValue TTvBroadcastRequestImpl::NHLTvChannelsScenario() {
    Ctx.AddAttention(TStringBuf("nhl_translation_in_request"));

    // response with a stub otherwise
    if (!TTvChannelsHelper::CanShowTvStreamOnClient(Ctx)) {
        Ctx.AddAttention(TStringBuf("nhl_translation_not_supported"));
        NSc::TValue data;
        data["uri"] = NHL_FALLBACK_URL;
        Ctx.AddSuggest(TStringBuf("nhl_fallback_to_service"), data);
        return TResultValue();
    }

    TStringBuf project = Ctx.HasExpFlag(EXPERIMENTAL_FLAG_TV_SPECIAL_PROJECT_GALLERY) ?
                              NHL_TV_CHANNEL_NAME : TStringBuf("");

    return TTvChannelsHelper(Ctx).ShowTvChannelsGallery(project);
}

TResultValue TTvBroadcastRequestImpl::TvBroadcastCommonScenario(const TDateTime& userTime) {
    // Channel and program ids and stream info
    if (Ctx.ClientFeatures().SupportsDivCards() || !IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_CHANNEL)) ||
        !IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_PROGRAM))) {
        if (const auto error = RequestTvIds()) {
            return error;
        }
    }

    // show TV-gallery or play TV-stream on smart-speakers-with-screen
    if (TTvChannelsHelper::CanShowTvStreamOnClient(Ctx) && !HasScheduleInRequest()) {
        return TvStreamStationScenario();
    }

    // переходим в поиск, если просят включить канал без эфира
    if (Ctx.ClientFeatures().SupportsDivCards()
        && Ctx.FormName().StartsWith(TV_STREAM)
        && Result.Has("channel")
        && !Result["channel"].Has("stream_info"))
    {
        if (Ctx.HasExpFlag("tv_disable_search_change_form")) {
            Ctx.AddAttention("no_stream_for_channel");
            return TResultValue();
        }
        if (TSearchFormHandler::SetAsResponse(Ctx, false)) {
            return Ctx.RunResponseFormHandler();
        }
        return TError(TError::EType::TVERROR, TStringBuf("tv channel has no stream"));
    }

    if (Ctx.ClientFeatures().SupportsDivCards()
        && Ctx.FormName().StartsWith(TV_STREAM)
        && (!Result.Has("channel") || Result["channel"].Has("stream_info")))
    {
        Result["shouldAddTvStreamCard"] = true;
    }

    // Schedule
    if (!Result.Has("shouldAddTvStreamCard")) {
        if (IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_PROGRAM))) {
            if (const auto error = RequestTvSchedule(userTime, *DateTime)) {
                return error;
            }
        } else {
            if (!Result.Has("program") || RequestTvSchedule(userTime, *DateTime) || Result["schedule"].ArrayEmpty()) {
                // передача не нашлась или расписание пустое:
                // уходим в поиск

                if (Ctx.ClientFeatures().SupportsTvOpenSearchScreenDirective()) {
                    LOG(INFO) << "Send tv_open_search_screen directive to perform video search" << Endl;
                    NSc::TValue payload;
                    payload["search_query"] = Ctx.Meta().Utterance();
                    Ctx.AddCommand<TTvOpenSearchScreenDirective>(NAlice::NVideoCommon::COMMAND_TV_OPEN_SEARCH_SCREEN, payload);
                    return TResultValue();
                }

                if (!Ctx.ClientFeatures().SupportsOpenLink() && Ctx.MetaClientInfo().IsTvDevice()) {
                    LOG(INFO) << "This TV device does not supports open link feature, sending show_gallery directive" << Endl;
                    NSc::TValue payload;
                    payload["debug_info"]["ya_video_request"] = Ctx.Meta().Utterance();
                    // Empty array for analytics
                    payload["items"].SetArray();
                    Ctx.AddCommand<TShowGalleryDirective>(NAlice::NVideoCommon::COMMAND_SHOW_GALLERY, payload);
                    return TResultValue();
                }
                if (Ctx.HasExpFlag("tv_broadcast_disable_search_change_form")) {
                    Ctx.AddAttention("no_program_found"); // Attention
                    return TResultValue();
                }
                if (TSearchFormHandler::SetAsResponse(Ctx, false)) {
                    return Ctx.RunResponseFormHandler();
                }
                return TError(TError::EType::TVERROR, TStringBuf("tv program not found"));
            }
        }
    }

    if (StreamList.ArrayEmpty()) {
        Ctx.AddAttention(TStringBuf("empty_stream_list"));
    }

    // TV url
    if (const auto error = GetTvUrl(*DateTime)) {
        return error;
    }

    FinishUpGeo();
    AddDivCards();
    WriteSuggests();

    return TResultValue();
}

void TTvBroadcastRequestImpl::FinishUpGeo() {
    // Finish Up Geo
    NSc::TValue broadcastLocation;
    broadcastLocation["geoid"].SetIntNumber(RespondedGeoId);

    if (RequestedCity.GetId() != RespondedGeoId) {
        RequestedCity.ConvertTo(RespondedGeoId);
    }

    if (!RequestedCity.IsSame()) {
        const TContext::TSlot* slot = Ctx.GetSlot(SLOT_NAME_WHERE);
        if (!IsSlotEmpty(slot)) {
            Ctx.AddAttention(TStringBuf("geo_changed"));
        }
    }

    RequestedCity.AddAllCaseForms(Ctx, &broadcastLocation, true /* wantObsolete */);
    Ctx.CreateSlot(SLOT_NAME_BROADCAST_LOCATION, SLOT_TYPE_BROADCAST_LOCATION, /* optional */ true,
                   std::move(broadcastLocation));
}

void TTvBroadcastRequestImpl::AddDivCards() {
    // Cards
    if (Ctx.ClientFeatures().SupportsDivCards()) {
        if (Result.Has("shouldAddTvStreamCard")) {
            if (Result.Has("channel")) {
                Ctx.AddTextCardBlock("tv_stream_channel");
                Ctx.AddDivCardBlock("stream_channel_card", NSc::Null());
            } else {
                Ctx.AddTextCardBlock("tv_stream_gallery");
                if (!StreamList.ArrayEmpty()) {
                    Ctx.AddDivCardBlock("stream_gallery_card", NSc::Null());
                    Result["stream_list"] = StreamList;
                }
            }
        } else {
            if (Result.Has("program")) {
                Ctx.AddDivCardBlock("program_card", NSc::Null());
            } else if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) {
                Ctx.AddDivCardBlock("gallery_card", NSc::Null());
            } else if (Result.Has("channel") && !Result.Has("ignore_channel")) {
                Ctx.AddDivCardBlock("channel_card", NSc::Null());
            } else {
                Ctx.AddDivCardBlock("schedule_card", NSc::Null());
            }
            Ctx.AddTextCardBlock("tv_broadcast");
        }
    } else {
        Ctx.AddAttention(TStringBuf("no_div_cards"));
    }
}

void TTvBroadcastRequestImpl::WriteError(const TError& error) {
    WriteSuggests();
    Ctx.AddErrorBlock(error);
}

void TTvBroadcastRequestImpl::WriteSuggests() {
    if (!Result.Has("shouldAddTvStreamCard")) {
        if (Result.Has("ignore_channel")) {
            Ctx.AddSuggest(CHANNEL_SUGGEST, Result.TrySelect("channel/title").GetString());
        }
        if (!Result.Has("program") && !RequestedCity.HasError()) {
            TStringBuf slotDayPart = IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_DAY_PART)) ? "" : Ctx.GetSlot(
                    SLOT_NAME_DAY_PART)->Value.GetString();
            TStringBuf userDayPart = Result.TrySelect("user_day_part").GetString();
            for (auto dayPart : DAY_PARTS) {
                if (slotDayPart != dayPart && userDayPart != dayPart) {
                    Ctx.AddSuggest(DAY_PART_SUGGEST, dayPart);
                    break;
                }
            }
        }
        if ((Result.Has("channel") && !Result.Has("ignore_channel"))
            || Result.Has("program")
            || !IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) {
            Ctx.AddSuggest(TV_BROADCAST_SUGGEST);
        }
        if (!StreamList.ArrayEmpty() &&
            (Ctx.ClientFeatures().SupportsDivCards() || TTvChannelsHelper::CanShowTvStreamOnClient(Ctx)))
        {
            Ctx.AddSuggest(TV_STREAM_SUGGEST);
        }
    } else {
        Ctx.AddSuggest(TV_BROADCAST_SUGGEST);

        static_assert(Y_ARRAY_SIZE(SUGGEST_CHANNELS) > 0, "Revisit SUGGEST_CHANNELS");
        size_t num = Ctx.GetRng().RandomInteger(Y_ARRAY_SIZE(SUGGEST_CHANNELS));
        // чтобы не писать в саджесте тот же канал, про который был запрос
        if (SUGGEST_CHANNELS[num].second == Result.TrySelect("channel/familyId").ForceString()) {
            num = (num + 1) % Y_ARRAY_SIZE(SUGGEST_CHANNELS);
        }
        Ctx.AddSuggest(CHANNEL_PLAY_SUGGEST, SUGGEST_CHANNELS[num].first);
    }
}

namespace {
NSc::TValue CreateFixlistedChannel(TStringBuf channelName) {
    NSc::TValue resultChannel;
    if (channelName == NY2020_TV_CHANNEL_NAME) {
        resultChannel["title"] = NY2020_TV_CHANNEL_TITLE;
        resultChannel["channelId"] = NY2020_TV_CHANNEL_ID;
        resultChannel["familyId"] = NY2020_TV_CHANNEL_ID;
    } else if (channelName == CONCERTS_TV_CHANNEL_NAME) {
        resultChannel["title"] = CONCERTS_TV_CHANNEL_TITLE;
        resultChannel["channelId"] = CONCERTS_TV_CHANNEL_ID;
        resultChannel["familyId"] = CONCERTS_TV_CHANNEL_ID;
    } else if (channelName == THEATERS_TV_CHANNEL_NAME) {
        resultChannel["title"] = THEATERS_TV_CHANNEL_TITLE;
        resultChannel["channelId"] = THEATERS_TV_CHANNEL_ID;
        resultChannel["familyId"] = THEATERS_TV_CHANNEL_ID;
    }
    return resultChannel;
}

} // namespace


TResultValue TTvBroadcastRequestImpl::Do() {
    // Geo
    if (const auto error = ProcessTvGeo()) {
        WriteError(*error);
        return TResultValue();
    }

    const auto& timeZone = Ctx.GlobalCtx().GeobaseLookup().GetTimezoneName(RespondedGeoId);
    const TDateTime userTime(TDateTime::TSplitTime(NDatetime::GetTimeZone(timeZone), Ctx.Meta().Epoch()));

    if (const auto error = ProcessDateTime(userTime)) {
        WriteError(*error);
        return TResultValue();
    }

    TResultValue error;
    // some custom logic for personal channel and NHL project
    TContext::TSlot* channelSlot = Ctx.GetOrCreateSlot(SLOT_NAME_CHANNEL, SLOT_TYPE_CHANNEL);
    TStringBuf channelName = channelSlot->Value.GetString();
    bool isFixlistedChannel = !IsSlotEmpty(channelSlot) && channelSlot->Type == SLOT_TYPE_CHANNEL_FIXLIST;
    bool isRestreamedChannel = !IsSlotEmpty(channelSlot) && channelSlot->Type == SLOT_TYPE_CHANNEL_RESTREAMED;

    // will actually play fixed restreamed channels only on smart-speaker-with-tv devices under exp-flag
    // otherwise will perform channel search as usual
    if (isRestreamedChannel && !TTvChannelsHelper::CanShowTvStreamOnClient(Ctx)) {
        channelSlot->Type = SLOT_TYPE_CHANNEL_SUGGEST;
        isRestreamedChannel = false;
    }

    if (isFixlistedChannel && channelName == PERSONAL_TV_CHANNEL_NAME) { // special logic for personal channel
        error = PersonalTvChannelScenario();
    } else if (isFixlistedChannel && channelName == NHL_TV_CHANNEL_NAME) { // special logic for NHL tv-gallery
        error = NHLTvChannelsScenario();
    } else { // common scenario in any other cases (with channel info fixing if needed)
        NSc::TValue resultChannel;
        if (isFixlistedChannel) {
            // TODO remove channel fixlisting after https://st.yandex-team.ru/VAD-43
            resultChannel = CreateFixlistedChannel(channelName);
        } else if (isRestreamedChannel) {
            TMaybe<NSc::TValue> restreamedChannel =
                    NAlice::NVideoCommon::TRestreamedChannelsData::Instance().GetRestreamedChannelInfo(channelName);
            if (restreamedChannel.Defined()) {
                resultChannel["title"] = (*restreamedChannel)["title"];
                resultChannel["channelId"] = (*restreamedChannel)["channel_id"];
                resultChannel["familyId"] = (*restreamedChannel)["family_id"];
            }
        }

        if (!resultChannel.IsNull()) {
            Result[SLOT_NAME_CHANNEL] = std::move(resultChannel);
        }

        error = TvBroadcastCommonScenario(userTime);
    }

    Ctx.CreateSlot(SLOT_NAME_ANSWER, SLOT_TYPE_ANSWER, /* optional */ true, std::move(Result));
    if (Result.Has("ignore_time")) {
        Ctx.CreateSlot(SLOT_NAME_WHEN, SLOT_TYPE_WHEN);
        Ctx.CreateSlot(SLOT_NAME_DAY_PART, SLOT_TYPE_DAY_PART);
    }

    if (error) {
        WriteError(*error);
    }

    return TResultValue();
}

TMaybe<NSc::TValue> TTvBroadcastRequestImpl::CheckChannelStream(const TStringBuf familyId, const TStringBuf yacFamilyId) {
    if (!Ctx.ClientFeatures().SupportsDivCards()) {
        return Nothing();
    }
    for (const auto& it : StreamList.GetArray()) {
        if ((yacFamilyId != "" && it.TrySelect("channel/yacFamilyId").ForceString() == yacFamilyId) ||
            (familyId != "" && it.TrySelect("channel/familyId").ForceString() == familyId)) {
            return it;
        }
    }
    return Nothing();
}

NSc::TValue TTvBroadcastRequestImpl::ChooseThumb(const NSc::TValue& images) {
    if (!images.Has("426")) {
        return images["400"]["src"];
    } else {
        return images["426"]["src"];
    }
}

void TTvBroadcastRequestImpl::CreateCommonRequest(const NHttpFetcher::TRequestPtr& request) {
    request->AddHeader("X-Uid", Ctx.GlobalCtx().Secrets().TVUid);
    request->AddHeader("X-Client-Id", Ctx.GlobalCtx().Secrets().TVClientId);
    request->AddCgiParam(TStringBuf("lang"), Ctx.MetaLocale().Lang);
}

TResultValue TTvBroadcastRequestImpl::RequestTvGeo() {
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().TVGeo("/regionInfo.json").Request();
    CreateCommonRequest(request);
    request->AddCgiParam(TStringBuf("regionId"), ToString(RequestedCity.GetId()));

    NHttpFetcher::TResponse::TRef handler = request->Fetch()->Wait();
    if (handler->IsError()) {
        return TError(
            TError::EType::TVERROR,
            TStringBuilder() << TStringBuf("Fetching from TV-geo error: ")
                             << handler->GetErrorText()
        );
    }

    NSc::TValue respJson = NSc::TValue::FromJson(handler->Data);
    if (respJson.IsNull()) {
        return TError(TError::EType::TVERROR, "parsing TV-geo answer error");
    }

    RespondedGeoId = respJson["schedule"]["scheduleId"].ForceIntNumber(NGeobase::UNKNOWN_REGION);

    return TResultValue();
}

NHttpFetcher::THandle::TRef TTvBroadcastRequestImpl::CreateTvStreamRequest(NHttpFetcher::IMultiRequest::TRef& multiRequest) {
    TStringBuilder path;
    path << "regions/"
        << RespondedGeoId
        << "/vhOnAir.json";
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().TVSearch(path).MakeOrAttachRequest(multiRequest);
    CreateCommonRequest(request);
    return request->Fetch();
}

TResultValue TTvBroadcastRequestImpl::ParseTvStreamResponse(NHttpFetcher::TResponse::TRef handler) {
    if (handler->IsError()) {
        return TError(
            TError::EType::TVERROR,
            TStringBuilder() << TStringBuf("Fetching from TV-stream error: ")
                             << handler->GetErrorText()
        );
    }

    NSc::TValue respJson = NSc::TValue::FromJson(handler->Data);
    if (respJson.IsNull()) {
        return TError(TError::EType::TVERROR, "parsing TV-stream answer error");
    }

    NSc::TValue result;
    NSc::TArray& resultStreamList = result.GetArrayMutable();
    for (const auto& streamIt : respJson["on_air_channels"].GetArray()) {
        NSc::TValue stream;
        stream["program"]["programId"] = streamIt["broadcastedProgramTo"]["program"]["id"];
        stream["program"]["eventId"] = streamIt["broadcastedProgramTo"]["id"];
        stream["program"]["title"] = streamIt["broadcastedProgramTo"]["program"]["title"];
        stream["program"]["thumb"] = ChooseThumb(streamIt["broadcastedProgramTo"]["program"]["images"][0]["sizes"]);
        stream["program"]["start"] = streamIt["broadcastedProgramTo"]["start"];
        stream["program"]["finish"] = streamIt["broadcastedProgramTo"]["finish"];
        stream["channel"]["title"] = streamIt["channelTo"]["title"];
        stream["channel"]["channelId"] = streamIt["channelTo"]["id"];
        stream["channel"]["familyId"] = streamIt["channelTo"]["familyId"];
        if (streamIt["channelTo"].Has("yacFamilyId")) {
            stream["channel"]["yacFamilyId"] = streamIt["channelTo"]["yacFamilyId"];
        }
        stream["channel"]["stream_url"] = TStringBuilder()
            << CHANNEL_STREAM_URL
            << stream["channel"]["familyId"].ForceString()
            << CHANNEL_STREAM_FROM_BLOCK;
        if (stream["channel"]["familyId"].ForceString() != "" || stream["channel"].Has("yacFamilyId")) {
            resultStreamList.push_back(std::move(stream));
        }
    }

    StreamList = result;
    return TResultValue();
}

NHttpFetcher::THandle::TRef TTvBroadcastRequestImpl::CreateTvIdRequest(const TStringBuf type, NHttpFetcher::IMultiRequest::TRef& multiRequest) {
    TStringBuilder path;
    path << "regions/"
        << RespondedGeoId
        << "/search/"
        << (type == SLOT_NAME_PROGRAM ? "episodes" : "channels")
        << ".json";
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().TVSearch(path).MakeOrAttachRequest(multiRequest);
    CreateCommonRequest(request);
    request->AddCgiParam(TStringBuf("limit"), "1");
    request->AddCgiParam(TStringBuf("offset"), "0");
    request->AddCgiParam(TStringBuf("searchEngine"), "BigXml");
    request->AddCgiParam(TStringBuf("text"), Ctx.GetSlot(type)->Value.GetString());
    return request->Fetch();
}

NHttpFetcher::THandle::TRef TTvBroadcastRequestImpl::CreateTvIdSuggestRequest(const TStringBuf type, NHttpFetcher::IMultiRequest::TRef& multiRequest) {
    TStringBuilder path;
    path << "regions/"
        << RespondedGeoId
        << "/suggest-search.json";
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().TVSearch(path).MakeOrAttachRequest(multiRequest);
    CreateCommonRequest(request);
    request->AddCgiParam(TStringBuf("channel_count"), type == SLOT_NAME_CHANNEL ? TStringBuf("1") : TStringBuf("0"));
    request->AddCgiParam(TStringBuf("event_count"), type == SLOT_NAME_PROGRAM ? TStringBuf("1") : TStringBuf("0"));
    TStringBuf channelName = Ctx.GetSlot(type)->Value.GetString();
    if (channelName == CHANNEL_OHOTNIK_AND_RYBOLOV_OLD && Ctx.HasExpFlag(EXP_FLAG_OHOTNIK_AND_RYBOLOV)) {
        channelName = CHANNEL_OHOTNIK_AND_RYBOLOV_NEW;
    }
    request->AddCgiParam(TStringBuf("text"), channelName);
    return request->Fetch();
}

TResultValue TTvBroadcastRequestImpl::ParseTvIdResponse(const TStringBuf type, NHttpFetcher::TResponse::TRef handler) {
    if (handler->IsError()) {
        return TError(
            TError::EType::TVERROR,
            TStringBuilder() << TStringBuf("Fetching from TV-search error: ")
                             << handler->GetErrorText()
        );
    }

    NSc::TValue respJson = NSc::TValue::FromJson(handler->Data);
    if (respJson.IsNull()) {
        return TError(TError::EType::TVERROR, "parsing TV-search answer error");
    }

    NSc::TValue result;
    if ((type == SLOT_NAME_CHANNEL) && !respJson["channels"].ArrayEmpty()) {
        const NSc::TValue& firstChannel = respJson["channels"][0];
        result["channelId"] = firstChannel["id"];
        result["title"] = firstChannel["title"];
        result["familyId"] = firstChannel["familyId"];
        if (firstChannel.Has("yacFamilyId")) {
            result["yacFamilyId"] = firstChannel["yacFamilyId"];
        }
        if (const auto streamInfo = CheckChannelStream(result.TrySelect("familyId").ForceString(), result.TrySelect("yacFamilyId").ForceString())) {
            result["stream_info"] = *streamInfo;
        }
    } else if ((type == SLOT_NAME_PROGRAM) && !respJson["events"].ArrayEmpty()) {
        const NSc::TValue& firstProgram = respJson["events"][0]["program"];
        result["programId"] = firstProgram["id"];
        result["title"] = firstProgram["title"];
        result["description"] = firstProgram["description"];
        result["type"] = firstProgram["type"]["name"];
        result["ageRestriction"] = firstProgram["ageRestriction"];
        result["logo"] = firstProgram["images"][0]["sizes"]["80"]["src"];
    }

    if (result.Has("title")) {
        Result[type] = result;
    } else {
        Ctx.AddAttention(TStringBuilder() << "cant_find_" << type);
    }

    return TResultValue();
}

TResultValue TTvBroadcastRequestImpl::RequestTvIds() {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
    NHttpFetcher::THandle::TRef streamRequest;
    NHttpFetcher::THandle::TRef channelRequest;
    NHttpFetcher::THandle::TRef programRequest;
    if (Ctx.ClientFeatures().SupportsDivCards()) {
        streamRequest = CreateTvStreamRequest(multiRequest);
    }

    TContext::TSlot* channelSlot = Ctx.GetSlot(SLOT_NAME_CHANNEL);
    if (!IsSlotEmpty(channelSlot)) {
        if (channelSlot->Type == SLOT_TYPE_CHANNEL) {
            channelRequest = CreateTvIdRequest(SLOT_NAME_CHANNEL, multiRequest);
        } else if (channelSlot->Type == SLOT_TYPE_CHANNEL_SUGGEST) {
            channelRequest = CreateTvIdSuggestRequest(SLOT_NAME_CHANNEL, multiRequest);
        }
    }

    if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_PROGRAM))) {
        programRequest = CreateTvIdRequest(SLOT_NAME_PROGRAM, multiRequest);
    }

    if (!streamRequest && !channelRequest && !programRequest) {
        return TResultValue();
    }

    if (streamRequest) {
        if (const auto error = ParseTvStreamResponse(streamRequest->Wait())) {
            return error;
        }
    }
    if (channelRequest) {
        if (const auto error = ParseTvIdResponse(SLOT_NAME_CHANNEL, channelRequest->Wait())) {
            return error;
        }
    }
    if (programRequest) {
        if (const auto error = ParseTvIdResponse(SLOT_NAME_PROGRAM, programRequest->Wait())) {
            return error;
        }
    }
    return TResultValue();
}

NHttpFetcher::THandle::TRef TTvBroadcastRequestImpl::CreateTvScheduleRequest(
    const TDateTime& userTime,
    const TDateTimeList& dtl,
    const bool ignoreChannel,
    const bool ignoreTime,
    NHttpFetcher::IMultiRequest::TRef& multiRequest)
{
    TStringBuilder path;
    if (Result.Has("program")) {
        path << "alice/program-schedule.json";
    } else if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) {
        path << "alice/program-genres-schedule.json";
    } else {
        path << "regions/" << RespondedGeoId << "/schedule.json";
    }
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().TVSchedule(path).MakeOrAttachRequest(multiRequest);
    CreateCommonRequest(request);

    auto time = (Result.Has("start_now") || ignoreTime) ? &userTime.SplitTime() : &dtl.cbegin()->SplitTime();
    TString timeString = time->ToString(DATETIME_FORMAT);
    request->AddCgiParam(TStringBuf("start"), timeString);

    TDuration duration = SUFFICIENT_DURATION;
    if ((Result.Has("channel") && !ignoreChannel && !IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) || Result.Has("program")) {
        // TODO special logic for range_day_part
        if (Result["date_type"].GetString() == TStringBuf("now") || ignoreTime) {
            duration = MAX_AVAILABLE_DURATION;
        } else if (Result["date_type"].GetString() == TStringBuf("day_part")) {
            duration = DAY_PART_DURATION;
        } else { // range or range_day_part or one_day
            duration = TDuration::Days(dtl.TotalDays());
        }
    }
    request->AddCgiParam(TStringBuf("duration"), ToString(duration.Seconds()));

    if (Result.Has("channel") && !ignoreChannel) {
        request->AddCgiParam(TStringBuf("channelIds"), Result["channel"]["channelId"].ForceString());
    }

    if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE)) && !Result.Has("program")) {
        request->AddCgiParam(TStringBuf("programTypeIds"), VinsToTvGenreId(Ctx.GetSlot(SLOT_NAME_GENRE)->Value.GetString()));
    }

    if (Result.Has("program")) {
        request->AddCgiParam(TStringBuf("programId"), Result["program"]["programId"].ForceString());
        request->AddCgiParam(TStringBuf("eventsLimit"), EVENTS_LIMIT);
        request->AddCgiParam(TStringBuf("regionId"), ToString(RespondedGeoId));
    } else if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) {
        request->AddCgiParam(TStringBuf("programTypeIds"), VinsToTvGenreId(Ctx.GetSlot(SLOT_NAME_GENRE)->Value.GetString()));
        request->AddCgiParam(TStringBuf("eventsLimit"), CHANNELS_LIMIT);
        request->AddCgiParam(TStringBuf("regionId"), ToString(RespondedGeoId));
    } else {
        request->AddCgiParam(TStringBuf("channelProgramsLimit"), PROGRAMS_LIMIT);
        request->AddCgiParam(TStringBuf("channelLimit"), CHANNELS_LIMIT);
        request->AddCgiParam(TStringBuf("channelOffset"), "0");
    }
    return request->Fetch();
}

TResultValue TTvBroadcastRequestImpl::ParseTvScheduleResponse(NHttpFetcher::TResponse::TRef handler) {
    if (handler->IsError()) {
        return TError(
            TError::EType::TVERROR,
            TStringBuilder() << TStringBuf("Fetching from TV-schedule error: ")
                             << handler->GetErrorText()
        );
    }

    NSc::TValue respJson = NSc::TValue::FromJson(handler->Data);
    if (respJson.IsNull()) {
        return TError(TError::EType::TVERROR, "parsing TV-schedule answer error");
    }

    NSc::TValue result;
    NSc::TArray& resultSchedule = result.GetArrayMutable();
    for (const auto& channelIt : respJson["schedules"].GetArray()) {
        NSc::TValue channel;
        NSc::TArray& channelEvents = channel["events"].GetArrayMutable();
        for (const auto& eventIt : channelIt["events"].GetArray()) {
            NSc::TValue event;
            event["title"] = eventIt["program"]["title"];
            event["episodeTitle"] = eventIt["episode"].Has("title") ? eventIt["episode"]["title"] : eventIt["title"];
            event["seasonNumber"] = eventIt["episode"].Has("seasonNumber") ? eventIt["episode"]["seasonNumber"] : eventIt["seasonNumber"];
            event["seasonName"] = eventIt["episode"].Has("seasonName") ? eventIt["episode"]["seasonName"] : eventIt["seasonName"];
            event["start"] = eventIt["start"];
            event["finish"] = eventIt["finish"];
            event["thumb"] = ChooseThumb(eventIt["program"]["images"][0]["sizes"]);
            event["programId"] = eventIt["program"]["id"];
            event["episodeId"] = eventIt["episode"]["id"];
            event["eventId"] = eventIt["id"];
            event["schedule_url"] = TStringBuilder() << PROGRAM_URL << event["programId"].ForceString() << "?eventId=" << event["eventId"].ForceString() << PROGRAM_UTM;
            channelEvents.push_back(std::move(event));
        }
        channel["channel"]["title"] = channelIt["channel"]["title"];
        channel["channel"]["channelId"] = channelIt["channel"]["id"];
        channel["channel"]["logo"] = channelIt["channel"]["logo"]["sizes"]["64"]["src"];
        channel["channel"]["familyId"] = channelIt["channel"]["familyId"];
        if (channelIt["channel"].Has("yacFamilyId")) {
            channel["channel"]["yacFamilyId"] = channelIt["channel"]["yacFamilyId"];
        }
        if (const auto streamInfo = CheckChannelStream(channel.TrySelect("channel/familyId").ForceString(), channel.TrySelect("channel/yacFamilyId").ForceString())) {
            channel["channel"]["stream_info"] = *streamInfo;
        }
        if (!channel["events"].ArrayEmpty()) {
            resultSchedule.push_back(std::move(channel));
        }
    }

    if (!Result.Has("schedule") && !result.ArrayEmpty()) {
        Result["schedule"] = result;
    }
    return TResultValue();
}

TResultValue TTvBroadcastRequestImpl::RequestTvSchedule(const TDateTime& userTime, const TDateTimeList& dtl) {
    NHttpFetcher::IMultiRequest::TRef multiRequest = NHttpFetcher::MultiRequest();
    NHttpFetcher::THandle::TRef commonRequest = CreateTvScheduleRequest(userTime, dtl, /*ignoreChannel*/ false, /*ignoreTime*/ false, multiRequest);
    NHttpFetcher::THandle::TRef ignoreChannelRequest;
    NHttpFetcher::THandle::TRef ignoreTimeRequest;
    if (Result.Has("channel") && (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE)) || Result.Has("program"))) {
        ignoreChannelRequest = CreateTvScheduleRequest(userTime, dtl, /*ignoreChannel*/ true, /*ignoreTime*/ false, multiRequest);
    }
    // TODO тут можно проверять что старт не попадает в гарантированные 4 дня (доступное расписание в худшем случае)
    if (Result["date_type"].GetString() != TStringBuf("now")) {
        ignoreTimeRequest = CreateTvScheduleRequest(userTime, dtl, /*ignoreChannel*/ bool(ignoreChannelRequest), /*ignoreTime*/ true, multiRequest);
    }

    if (const auto error = ParseTvScheduleResponse(commonRequest->Wait())) {
        return error;
    }
    if (Result.Has("schedule")) {
        return TResultValue();
    }

    if (ignoreChannelRequest) {
        Ctx.AddAttention(TStringBuf("ignore_channel"));
        Result["ignore_channel"] = true;
        if (const auto error = ParseTvScheduleResponse(ignoreChannelRequest->Wait())) {
            return error;
        }
        if (Result.Has("schedule")) {
            return TResultValue();
        }
    }

    if (ignoreTimeRequest) {
        Ctx.AddAttention(TStringBuf("ignore_time"));
        Result["ignore_time"] = true;
        if (const auto error = ParseTvScheduleResponse(ignoreTimeRequest->Wait())) {
            return error;
        }
        if (Result.Has("schedule")) {
            return TResultValue();
        }

    }

    return TError(TError::EType::TVERROR, "empty tv schedule");
}

TResultValue TTvBroadcastRequestImpl::GetTvUrl(const TDateTimeList& dtl) {
    TStringBuilder url;
    TCgiParameters cgi;
    url << SCHEDULE_URL;
    cgi.InsertUnescaped("utm_source", "alice");
    if (Result.Has("program")) {
        url << "program/" << Result["program"]["programId"].ForceString();
        cgi.InsertUnescaped("utm_content", "tvprogram");
    } else {
        if (Result.Has("channel") && !Result.Has("ignore_channel")) {
            url << "channels/" << Result["channel"]["channelId"].ForceString();
            cgi.InsertUnescaped("utm_content", "tvchannel");
        } else {
            cgi.InsertUnescaped("utm_content", "tvmain");
        }
        if (!IsSlotEmpty(Ctx.GetSlot(SLOT_NAME_GENRE))) {
            cgi.InsertUnescaped("genre", VinsToTvGenre(Ctx.GetSlot(SLOT_NAME_GENRE)->Value.GetString()));
        }
        if (Result["date_type"].GetString() != TStringBuf("now") && !Result.Has("start_now") && !Result.Has("ignore_time")) {
            TString date = dtl.cbegin()->SplitTime().ToString("%F");
            cgi.InsertUnescaped("date", date);
            cgi.InsertUnescaped("period", "all-day");
        }
    }
    url << "?" << cgi.Print();
    Result["schedule_url"] = url;
    Result["common_stream_url"] = COMMON_STREAM_URL;
    return TResultValue();
}

bool TTvBroadcastRequestImpl::HasScheduleInRequest() {
    TSlot* scheduleMarker = Ctx.GetSlot(SLOT_NAME_SCHEDULE_MARKER);
    return scheduleMarker && (scheduleMarker->Value == SLOT_VALUE_TV_SCHEDULE)
           || !Result.TrySelect("start_now").GetBool(false);
}

TResultValue TTvBroadcastFormHandler::Do(TRequestHandler& r) {
    r.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(r.Ctx().FormName().StartsWith(TV_STREAM)
                                                                 ? NAlice::NProductScenarios::TV_STREAM
                                                                 : NAlice::NProductScenarios::TV_BROADCAST);
    TTvBroadcastRequestImpl impl(r);
    return impl.Do();
}

// static
void TTvBroadcastFormHandler::Register(THandlersMap* handlers) {
    auto cbTvBroadcastForm = []() {
        return MakeHolder<TTvBroadcastFormHandler>();
    };
    handlers->RegisterFormHandler(TV_BROADCAST, cbTvBroadcastForm);
    handlers->RegisterFormHandler(TV_BROADCAST_ELLIPSIS, cbTvBroadcastForm);
    handlers->RegisterFormHandler(TV_STREAM, cbTvBroadcastForm);
    handlers->RegisterFormHandler(TV_STREAM_ELLIPSIS, cbTvBroadcastForm);
}

} // namespace NBASS
