#include "create_search_request.h"

#include <alice/library/music/common_special_playlists.h>
#include <alice/library/music/defs.h>

#include <alice/hollywood/library/frame/slot.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>

#include <alice/library/websearch/prepare_search_request.h>

namespace {

constexpr TStringBuf FLAG_ANALYTICS_MUSIC_WEB_RESPONSES = "analytics.music.add_web_responses";

const TString DUMMY_RTLOG_TOKEN = "dummy_rtlog_token";
const TString HAMSTER_QUOTA = "bass-hamster";
const TString SLOT_SPECIAL_PLAYLIST = "special_playlist";

const THashMap<TStringBuf, NJson::TJsonValue> PERSONAL_SPECIAL_PLAYLISTS_DATA = {
    {
        TStringBuf("playlist_of_the_day"),
        NAlice::JsonFromString("{\"title\":\"Плейлист дня\",\"special_playlist\":\"daily\"}")
    },
    {
        TStringBuf("recent_tracks"),
        NAlice::JsonFromString("{\"title\":\"Премьера\",\"special_playlist\":\"premiere\"}")
    },
    {
        TStringBuf("missed_likes"),
        NAlice::JsonFromString("{\"title\":\"Тайник\",\"special_playlist\":\"secret\"}")
    },
    {
        TStringBuf("never_heard"),
        NAlice::JsonFromString("{\"title\":\"Дежавю\",\"special_playlist\":\"dejavu\"}")
    },
    {
        TStringBuf("year_top_2019"),
        NAlice::JsonFromString("{\"title\":\"Перемотка\'19\",\"special_playlist\":\"rewind\"}")
    },
    {
        TStringBuf("kids_rewind"),
        NAlice::JsonFromString("{\"title\":\"Детская перемотка\",\"special_playlist\":\"kidsRewind\"}")
    },
    {
        TStringBuf("summer_top"),
        NAlice::JsonFromString("{\"title\":\"Летняя перемотка\",\"special_playlist\":\"summerTop\"}")
    },
    {
        TStringBuf("rewind10"),
        NAlice::JsonFromString("{\"title\":\"Большая перемотка\",\"special_playlist\":\"rewind10\"}")
    },
    {
        TStringBuf("rewind20"),
        NAlice::JsonFromString("{\"title\":\"Мой 2020\",\"special_playlist\":\"rewind20\"}")
    },
    {
        TStringBuf("kinopoisk"),
        NAlice::JsonFromString("{\"title\":\"Киноплейлист\",\"special_playlist\":\"kinopoisk\"}")
    },
    {
        TStringBuf("origin"),
        NAlice::JsonFromString("{\"title\":\"Плейлист с Алисой\",\"special_playlist\":\"origin\"}")
    },
    {
        TStringBuf("podcasts"),
        NAlice::JsonFromString("{\"title\":\"Подкасты недели\",\"special_playlist\":\"podcasts\"}")
    },
    {
        TStringBuf("family"),
        NAlice::JsonFromString("{\"title\":\"Семейный плейлист\",\"special_playlist\":\"family\"}")
    },
    {
        TStringBuf("morningShow"),
        NAlice::JsonFromString("{\"title\":\"Утреннее шоу Алисы\",\"special_playlist\":\"morningShow\"}")
    },
};

TString CreateTextQuery(
    const NAlice::NHollywood::NMusic::TMusicResources& musicResources,
    const NAlice::NHollywood::TFrame& frame,
    const NAlice::NHollywood::TScenarioRunRequestWrapper& runRequest,
    const NAlice::NScenarios::TInterfaces& interfaces
) {
    if (auto slotPtr = frame.FindSlot(SLOT_SPECIAL_PLAYLIST)) {
        auto spName = slotPtr->Value.AsString();

        if (spName == "alice" && runRequest.ClientInfo().IsSmartSpeaker() && interfaces.GetHasMusicPlayerShots()) {
            spName = "origin";
        }

        if (!PERSONAL_SPECIAL_PLAYLISTS_DATA.contains(spName)) {
            if (const auto* playlist = NAlice::NMusic::GetCommonSpecialPlaylists().FindPtr(spName)) {
                return playlist->Title.Data();
            }
        }
    }

    return NAlice::NHollywood::NMusic::MergeTextFromSlots(
        musicResources,
        frame
    );
}

void TryAddSpace(TStringBuilder& sb) {
    if (!sb.Empty() && sb.back() != ' ') {
        sb << " ";
    }
}

void TryAddText(TStringBuilder& sb, const TStringBuf buf) {
    if (!buf.Empty()) {
        sb << buf;
    }
}

TString FlatSearchText(const NAlice::NHollywood::TPtrWrapper<NAlice::NHollywood::TSlot>& searchTextSlot) {
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_provider.cpp?rev=r8671477#L257
    if (!searchTextSlot) {
        return {};
    }
    const TStringBuf value = searchTextSlot->Value.AsString();
    NJson::TJsonValue jsonObj;
    if (NJson::ReadJsonFastTree(value, &jsonObj, /* throwOnError= */ false, /* notClosedBracketIsError= */ true)) {
        if (jsonObj.IsArray()) {
            TStringBuilder sb;
            for (const auto& t : jsonObj.GetArray()) {
                TryAddSpace(sb);
                TryAddText(sb, t.GetString());
            }
            return sb;
        } else {
            return jsonObj.GetString();
        }
    } else {
        return TString{value};
    }
}

} // namespace

namespace NAlice::NHollywood::NMusic {

TString MergeTextFromSlots(const TMusicResources& musicResources, const TFrame& frame) {
    // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_provider.cpp?rev=r8671477#L275
    static const TVector<TStringBuf> SLOTS_ORDER_FOR_TEXT = {
        NAlice::NMusic::SLOT_NOVELTY,
        NAlice::NMusic::SLOT_NEED_SIMILAR,
        NAlice::NMusic::SLOT_MOOD,
        NAlice::NMusic::SLOT_GENRE,
        NAlice::NMusic::SLOT_ARTIST,
        NAlice::NMusic::SLOT_ALBUM,
        NAlice::NMusic::SLOT_TRACK,
        NAlice::NMusic::SLOT_PLAYLIST,
        NAlice::NMusic::SLOT_ACTIVITY,
        NAlice::NMusic::SLOT_EPOCH,
        NAlice::NMusic::SLOT_LANGUAGE,
        NAlice::NMusic::SLOT_VOCAL,
        "target_type", // only used for alarms, not needed here?..
    };

    TStringBuilder sb;
    const auto reverseMappingProvider = SlotToTextProvider(musicResources);

    for (const auto slotName : SLOTS_ORDER_FOR_TEXT) {
        if (const auto slot = frame.FindSlot(slotName)) {
            const TStringBuf key = slot->Type;
            const TString& value = slot->Value.AsString();
            const TStringBuf reversedValue = reverseMappingProvider(key, value);

            TryAddSpace(sb);
            TryAddText(sb, reversedValue);
        }
    }

    const auto searchTextSlot = frame.FindSlot(NAlice::NMusic::SLOT_SEARCH_TEXT);
    if (const auto text = FlatSearchText(searchTextSlot); !text.Empty()) {
        TryAddSpace(sb);
        TryAddText(sb, text);
    }

    return sb;
}

bool HaveHttpResponse(const TScenarioHandleContext& ctx, const TStringBuf itemName) {
    return ctx.ServiceCtx.HasProtobufItem(itemName);
}

TString GetRawHttpResponse(const TScenarioHandleContext& ctx, const TStringBuf itemName) {
    auto maybeResponse = GetMaybeOnlyProto<NAppHostHttp::THttpResponse>(ctx.ServiceCtx, itemName);

    // This rtlogToken is fake, it should be fixed in the whole music scenario
    TBassRequestRTLogToken rtlogToken;
    rtlogToken.SetRTLogToken(DUMMY_RTLOG_TOKEN);
    return RetireResponse(std::move(maybeResponse), rtlogToken, ctx.Ctx.Logger());
}

TMaybe<TString> GetRawHttpResponseMaybe(TScenarioHandleContext& ctx, const TStringBuf itemName) {
    if (HaveHttpResponse(ctx, itemName)) {
        return GetRawHttpResponse(ctx, itemName);
    }
    return Nothing();
}

TEvent CreatePartialEvent(const TScenarioInputWrapper& input) {
    TEvent event;
    const auto& inputProto = input.Proto();

    switch (inputProto.GetEventCase()) {
        case NScenarios::TInput::EventCase::kText: {
            event.SetType(EEventType::text_input);
            event.SetText(inputProto.GetText().GetRawUtterance());
            break;
        }
        case NScenarios::TInput::EventCase::kVoice: {
            event.SetType(EEventType::voice_input);

            const auto& voice = inputProto.GetVoice();
            *event.MutableAsrResult() = voice.GetAsrData();

            // XXX(a-square): add a HasBiometryScoring check if necessary,
            // not doing it right now to preserve the tests
            *event.MutableBiometryScoring() = voice.GetBiometryScoring();

            if (voice.HasBiometryClassification()) {
                *event.MutableBiometryClassification() = voice.GetBiometryClassification();
            }

            break;
        }
        case NScenarios::TInput::EventCase::kImage: {
            event.SetType(EEventType::image_input);
            break;
        }
        case NScenarios::TInput::EventCase::kCallback: {
            event.SetType(EEventType::server_action);
            *event.MutablePayload() = inputProto.GetCallback().GetPayload();
            event.SetName(inputProto.GetCallback().GetName());
            event.SetIgnoreAnswer(inputProto.GetCallback().GetIgnoreAnswer());
            break;
        }
        default: {
            break;
        }
    }
    return event;
}

NAlice::TWebSearchBuilder ConstructSearchRequest(
    const NAlice::NHollywood::NMusic::TMusicResources& musicResources,
    const TFrame& frame,
    const TScenarioRunRequestWrapper& runRequest,
    const NScenarios::TInterfaces& interfaces,
    const NScenarios::TRequestMeta& requestMeta,
    TRTLogger& rtLogger,
    TString& encodedAliceMeta,
    bool waitAll
) {
    auto searchText = TString::Join(
        CreateTextQuery(musicResources, frame, runRequest, interfaces),
        TStringBuf(" host:music.yandex.ru")
    );

    TCgiParameters cgi;
    if (runRequest.HasExpFlag(FLAG_ANALYTICS_MUSIC_WEB_RESPONSES)) {
        cgi.InsertUnescaped("flags", "alice_full_tmpl_data");
    }

    cgi.InsertEscaped("request_type", "music");
    cgi.InsertEscaped("device_locale", runRequest.ClientInfo().Lang);

    const TBlackBoxUserInfo* userInfo = GetUserInfoProto(runRequest);
    Y_ENSURE(userInfo);

    return PrepareSearchRequest(
        searchText,
        runRequest.ClientInfo(),
        ExpFlagsFromProto(runRequest.Proto().GetBaseRequest().GetExperiments()),
        interfaces.GetCanOpenLink(),
        /* speechKitEvent = */ CreatePartialEvent(runRequest.Input()),
        runRequest.Proto().GetBaseRequest().GetOptions().GetUserAgent(),
        GetContentRestrictionLevel(runRequest.ContentRestrictionLevel()),
        frame.Name(),
        runRequest.ClientInfo().Lang,
        cgi,
        runRequest.Proto().GetBaseRequest().GetRequestId(),
        runRequest.ClientInfo().Uuid,
        /* userTicket = */ requestMeta.GetUserTicket(),
        /* yandexUid = */ userInfo->GetUid(),
        runRequest.Proto().GetBaseRequest().GetOptions().GetClientIP(),
        /* cookies = */ {},
        NAlice::TWebSearchBuilder::EService::Megamind,
        /* megamindCgi = */ {},
        /* processId = */ Nothing(),
        ToString(runRequest.Proto().GetBaseRequest().GetRandomSeed()),
        runRequest.Proto().GetBaseRequest().GetOptions().GetUserDefinedRegionId(),
        /* imageSearch = */ false,
        /* hamsterQuota = */ "bass-hamster",
        waitAll,
        /* encodedAliceMeta = */ encodedAliceMeta,
        [&rtLogger](const TStringBuf msg) {
            LOG_WARNING(rtLogger) << msg << Endl;
        }
    );
}

} // namespace NAlice::NHollywood::NMusic
