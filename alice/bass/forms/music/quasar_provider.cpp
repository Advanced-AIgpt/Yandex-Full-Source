#include "converter.h"
#include "providers.h"
#include "push_message.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>
#include <alice/bass/forms/common/biometry_delegate.h>
#include <alice/bass/forms/common/blackbox_api.h>
#include <alice/bass/forms/common/data_sync_api.h>
#include <alice/bass/forms/video/billing.h>

#include <alice/bass/libs/client/client_info.h>
#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/config/config.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/push_notification/create_callback_data.h>
#include <alice/bass/libs/push_notification/request.h>
#include <alice/bass/libs/source_request/handle.h>
#include <alice/bass/setup/setup.h>

#include <alice/library/billing/defs.h>
#include <alice/library/biometry/biometry.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/music/defs.h>
#include <alice/library/music/fairytale_linear_albums.h>
#include <alice/library/util/system_time.h>

#include <alice/megamind/protos/common/atm.pb.h>
#include <alice/megamind/protos/scenarios/frame.pb.h>
#include <alice/megamind/protos/scenarios/stack_engine.pb.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/containers/stack_vector/stack_vec.h>
#include <library/cpp/svnversion/svnversion.h>

#include <util/charset/utf8.h>
#include <util/datetime/base.h>
#include <util/string/join.h>
#include <util/random/shuffle.h>

namespace NBASS {

namespace NMusic {

namespace {

constexpr TStringBuf FILTERS = "filters";
constexpr TStringBuf SEARCH = "search";
constexpr TStringBuf OBJECT = "object";

constexpr TStringBuf ATTENTIONS = "attentions";
constexpr TStringBuf ERRORS = "errors";

constexpr TStringBuf EXPERIMENT_RU_EXPLICIT = "medium_ru_explicit_content";
constexpr TStringBuf MULTIROOM_ALL_ROOMS = "__all__";

constexpr TStringBuf DEFAULT_FAIRY_TALE_URI = "https://music.yandex.ru/users/970829816/playlists/1039/?from=alice&mob=0";
constexpr TStringBuf DEFAULT_AMBIENT_SOUND_URI = "https://music.yandex.ru/users/103372440/playlists/1919/?from=alice&mob=0";
constexpr TStringBuf DEFAULT_PODCAST_URI = "https://music.yandex.ru/users/414787002/playlists/1104/?from=alice&mob=0";

constexpr TStringBuf FAIRY_TALES = "fairytales";
constexpr TStringBuf NATURE_SOUNDS = "naturesounds";
constexpr TStringBuf PODCASTS = "podcasts";

constexpr TStringBuf ERROR_RESTRICTED_BY_CHILD_CONTENT_SETTINGS = "music_restricted_by_child_content_settings";
constexpr TStringBuf ATTENTION_FORBIDDEN_PODCAST = "forbidden_podcast";

const int MORNING_SHOW_CHILD_CONFIG_ID = 1000;
const int RGB_COLOR_LEN = 7;

const THashMap<TStringBuf, TVector<TStringBuf>> SUPPORTED_MUSIC_TYPES = {
    { "yamusic", {"artist","track","album","playlist","radio","chart","metatag"} },
    { "navigator", {"album", "artist", "playlist", "track", "radio"} }
};

constexpr std::array<TStringBuf, 10> COMMON_SPECIAL_PLAYLISTS = {
    TStringBuf("ambient_sounds_default"),
    TStringBuf("sea_sounds"),
    TStringBuf("rain_sounds"),
    TStringBuf("bird_sounds"),
    TStringBuf("bonfire_sounds"),
    TStringBuf("wind_sounds"),
    TStringBuf("forest_sounds"),
    TStringBuf("fairy_tales_default"),
    TStringBuf("podcasts_default"),
    TStringBuf("podcasts_child"),
};

TStringBuf GetClientType(const TClientFeatures& clientFeatures) {
    if (clientFeatures.IsYaMusic()) {
        return TStringBuf("yamusic");
    } else if (clientFeatures.SupportsMusicQuasarClient() || clientFeatures.IsSmartSpeaker()) {
        return TStringBuf("quasar");
    } else if (clientFeatures.IsNavigator()) {
        return TStringBuf("navigator");
    }

    return TStringBuf("searchapp");
}

NSc::TValue MakeSupportedMusicTypesArray(const TClientFeatures& clientFeatures) {
    TStringBuf clientType = GetClientType(clientFeatures);

    NSc::TValue value;
    value.SetArray();

    if (SUPPORTED_MUSIC_TYPES.contains(clientType)) {
        for (const auto& t : SUPPORTED_MUSIC_TYPES.find(clientType)->second) {
            value.Push(t);
        }
    }

    return value;
}

// NOTE(a-square): trying to emulate what the NLG template for music_play does, to some extent
TString CreateHumanReadableTitle(const NSc::TValue& track) {
    const TStringBuf title = track[TStringBuf("title")].GetString();

    if (track[TStringBuf("subtype")].GetString() == TStringBuf("fairy-tale")) {
        return TString::Join("Сказка \"", title, '"');
    }

    if (track[TStringBuf("subtype")].GetString() == TStringBuf("audiobook")) {
        return TString::Join("Аудиокнига \"", title, '"');
    }

    if (track[TStringBuf("subtype")].GetString() == TStringBuf("podcast-episode")) {
        if (const auto& album = track[TStringBuf("album")][TStringBuf("title")].GetString()) {
            return TString::Join("Подкаст \"", album, "\", выпуск \"", title, '"');
        }
        return TString::Join("Выпуск \"", title, '"');
    }

    TStackVec<TStringBuf, 2> parts;
    if (auto& artists = track[TStringBuf("artists")].GetArray(); !artists.empty()) {
        if (const auto& name = artists[0][TStringBuf("name")].GetString()) {
            parts.push_back(name);
        }
    }
    parts.push_back(title);
    return JoinSeq(TStringBuf(", "), parts);
}

bool ShouldShowFirstTrack(const NAlice::TClientFeatures& clientFeatures, const TQuasarProvider::ERequestType result) {
    if (clientFeatures.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_FORCE_SHOW_FIRST_TRACK)) {
        return true;
    }

    if (clientFeatures.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_DONT_SHOW_FIRST_TRACK)) {
        return false;
    }

    if (!clientFeatures.IsSmartSpeaker()) {
        return false;
    }

    using ERequestType = TQuasarProvider::ERequestType;

    switch (result) {
        case ERequestType::Error:
        case ERequestType::Filters:
        case ERequestType::General:
        case ERequestType::Playlist:
            return false;
        case ERequestType::Answer:
        case ERequestType::Text:
            return true;
    }
}

TStringBuf GetFirstTrackGenre(const NSc::TValue& result) {
    const auto firstTrackGenre = result["first_track"]["album"]["genre"].GetString();

    if (firstTrackGenre.empty()) {
        const auto uri = result["uri"].GetString();
        if (uri == DEFAULT_FAIRY_TALE_URI) {
            return FAIRY_TALES;
        } else if (uri == DEFAULT_AMBIENT_SOUND_URI) {
            return NATURE_SOUNDS;
        } else if (uri == DEFAULT_PODCAST_URI) {
            return PODCASTS;
        }
    }

    return firstTrackGenre;
}

bool IsFirstTrackPodcast(const NSc::TValue& musicAnswer) {
    const auto trackType = musicAnswer.TrySelect("result/firstTrack/type");
    return trackType == "podcast" || trackType == "podcast-episode";
}

bool TryParseMorningshowConfigId(const TStringBuf expFlag, const TStringBuf expFlagPrefix, NSc::TValue& requestJson) {
    if (!expFlag.StartsWith(expFlagPrefix)) {
        return false;
    }
    int flagVal;
    const bool flagParsed = TryFromString<int>(expFlag.SubStr(expFlagPrefix.size()), flagVal);
    if (flagParsed) {
        requestJson["experiments"]["morningShowConfigId"].SetIntNumber(flagVal);
    } else {
        LOG(ERR) << "Failed to parse morningShowConfigId from: " << expFlag << Endl;
    }
    return flagParsed;
}

void AddStartMultiroom(TContext& context, const NSc::TValue& result, NSc::TValue& commandData) {
    NSc::TValue multiroomPayload;

    // forward old field room_id
    const TStringBuf roomId = result[MULTIROOM_ROOM].GetString();
    const TStringBuf fixedRoomId = (roomId == "everywhere") ? MULTIROOM_ALL_ROOMS : roomId;
    multiroomPayload["room_id"] = fixedRoomId;
    commandData["room_id"] = fixedRoomId;

    // forward new fields location_*
    for (const auto& device : result[MULTIROOM_LOCATION_DEVICES].GetArray()) {
        multiroomPayload["location_devices_ids"].Push(device.GetString());
        commandData["location_devices_ids"].Push(device.GetString());
    }
    for (const auto& group : result[MULTIROOM_LOCATION_GROUPS].GetArray()) {
        multiroomPayload["location_groups_ids"].Push(group.GetString());
        commandData["location_groups_ids"].Push(group.GetString());
    }
    for (const auto& room : result[MULTIROOM_LOCATION_ROOMS].GetArray()) {
        multiroomPayload["location_rooms_ids"].Push(room.GetString());
        commandData["location_rooms_ids"].Push(room.GetString());
    }
    for (const auto& modelId : result[MULTIROOM_LOCATION_SMART_SPEAKER_MODELS].GetArray()) {
        multiroomPayload["location_smart_speaker_models"].Push(modelId.GetIntNumber());
        commandData["location_smart_speaker_models"].Push(modelId.GetIntNumber());
    }
    if (result.Has(MULTIROOM_LOCATION_EVERYWHERE)) {
        multiroomPayload["location_everywhere"] = true;
        commandData["location_everywhere"] = true;
    }

    context.AddCommand<TStartMultiroomDirective>(TStringBuf("start_multiroom"), std::move(multiroomPayload));
}

bool IsMultiroomCommandToBeRunEverywhere(const NSc::TValue& result) {
    constexpr auto isRealRoom = [](const NSc::TValue& room) {return room.GetString() != "everywhere";};
    if (isRealRoom(result[MULTIROOM_ROOM])) {
        return false;
    }

    if (!result[MULTIROOM_LOCATION_DEVICES].ArrayEmpty() ||
        !result[MULTIROOM_LOCATION_GROUPS].ArrayEmpty() ||
        !result[MULTIROOM_LOCATION_SMART_SPEAKER_MODELS].ArrayEmpty() ||
        AnyOf(result[MULTIROOM_LOCATION_ROOMS].GetArray(), isRealRoom))
    {
        return false;
    }

     return true;
}

bool ResultHasAnyMultiroomLocations(const NSc::TValue& result) {
    const TStringBuf locationFields[] = {MULTIROOM_ROOM, MULTIROOM_LOCATION_ROOMS, MULTIROOM_LOCATION_GROUPS,
                                         MULTIROOM_LOCATION_DEVICES, MULTIROOM_LOCATION_EVERYWHERE,
                                         MULTIROOM_LOCATION_SMART_SPEAKER_MODELS};
    return AnyOf(locationFields, [&result](const auto field) {return !result.Get(field).IsNull(); });
}

bool MayBeAddStartMultiroomDirective(TContext& context, const NSc::TValue& result, NSc::TValue& commandData) {
    if (ResultHasAnyMultiroomLocations(result)) {
        const auto enabledByExp = !context.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MULTIROOM);
        const auto multiroomEnabled = context.ClientFeatures().SupportsMultiroom();
        const auto multiroomClusterEnabled = context.ClientFeatures().SupportsMultiroomCluster();

        if (enabledByExp && multiroomEnabled) {
            if (multiroomClusterEnabled || IsMultiroomCommandToBeRunEverywhere(result)) {
                AddStartMultiroom(context, result, commandData);
                return true;
            } else {
                context.AddAttention("multiroom_rooms_not_supported");
            }
        } else if (enabledByExp || multiroomEnabled) {
            context.AddAttention("multiroom_not_supported");
        }
    } else if (!result.Get(ACTIVATE_MULTIROOM).IsNull()) {
        NSc::TValue multiroomPayload;
        // room_id is "all" for client multiroom logic.
        multiroomPayload["room_id"] = "all";
        multiroomPayload["location_current_group"] = true;
        context.AddCommand<TStartMultiroomDirective>(TStringBuf("start_multiroom"), std::move(multiroomPayload));
        return true;
    }
    return false;
}

} // namespace

TQuasarProvider::TQuasarProvider(TContext& ctx)
    : TBaseMusicProvider(ctx)
    , ServiceAnswer(ctx.ClientFeatures())
    , Action(FILTERS)
    , MusicQuasarClient(ctx.ClientFeatures().SupportsMusicQuasarClient() || ctx.MetaClientInfo().IsSmartSpeaker())
{
}

bool TQuasarProvider::InitRequestParams(const NSc::TValue& slotData) {
    LOG(INFO) << "TQuasarProvider::InitRequestParams called" << Endl;
    auto result = DoInitRequestParams(slotData);
    LOG(INFO) << "DoInitRequestParams returned request type: " << result << Endl;

    if (ShouldShowFirstTrack(Ctx.ClientFeatures(), result)) {
        // Show first track info for requests with filters
        RequestBody["showFirstTrack"].SetBool(true);
    }

    return result != ERequestType::Error;
}

TQuasarProvider::ERequestType TQuasarProvider::DoInitRequestParams(const NSc::TValue& slotData) {
    if (DataGetHasKey(slotData, RESULT_SLOTS)) {
        return ERequestType::Answer;
    }

    // Do not enqueue search results to user queue
    if (!MusicQuasarClient || Ctx.HasExpFlag("music_no_enqueue")) {
        RequestBody["doNotEnqueue"].SetBool(true);
    }

    // Data from search result
    if (DataGetHasKey(slotData, SNIPPET_SLOTS)) {
        return InitWithSearchSnippet(slotData);
    }

    TContext::TSlot* slotAction = Ctx.GetSlot("modifier");
    if (!IsSlotEmpty(slotAction)) {
        Action = TString(slotAction->Value.GetString());
    } else if (Ctx.FormName() == MUSIC_PLAY_MORE_FORM_NAME) {
        Action = TStringBuf("more");
    } else if (Ctx.FormName() == MUSIC_PLAY_LESS_FORM_NAME) {
        Action = TStringBuf("less");
    } else {
        NeedChangeOwner = true;
    }

    NSc::TValue& filters = RequestBody[Action];

    HasSearch = DataGetHasKey(slotData, SEARCH_SLOTS);
    HasObject = DataGetHasKey(slotData, OBJECT_SLOTS);
    HasFilters = DataGetHasKey(slotData, MAIN_FILTERS_SLOTS) || DataGetHasKey(slotData, OTHER_FILTERS_SLOTS);
    HasRadioSeeds = DataGetHasKey(slotData, RADIO_SEEDS_SLOTS);
    LOG(INFO) << "HasSearch=" << HasSearch << ", HasObject=" << HasObject << ", HasFilters=" << HasFilters <<
        ", HasRadioSeeds=" << HasRadioSeeds << Endl;

    if (HasFilters) {
        if (Ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe) {
            NSc::TValue genre;
            genre.Push(TStringBuf("forchildren"));
            filters["genre"] = genre;
            if (!slotData.Has(MAIN_FILTERS_SLOTS) || !slotData[MAIN_FILTERS_SLOTS].Has("genre") ||
                slotData[MAIN_FILTERS_SLOTS]["genre"] != genre)
            {
                Ctx.AddAttention(ATTENTION_RESTRICTED_CONTENT_SETTINGS);
            }
        } else {
            for (const auto& ft : { MAIN_FILTERS_SLOTS, OTHER_FILTERS_SLOTS }) {
                if (slotData.Has(ft)) {
                    for (const auto& key : slotData[ft].DictKeys()) {
                        filters[key] = slotData[ft][key];
                    }
                }
            }
        }
    }

    if (HasObject) {
        for (auto sName : SLOT_NAMES.find(SEARCH_SLOTS)->second) {
            TString sNameId = TString::Join(sName, "_id");
            if (slotData[OBJECT_SLOTS].Has(sNameId)) {
                filters[OBJECT]["id"].SetString(slotData[OBJECT_SLOTS][sNameId].ForceString());
                filters[OBJECT]["type"].SetString(sName);
            }
        }
    }

    if (HasSearch) {
        for (auto s : SLOT_NAMES.find(SEARCH_SLOTS)->second) {
            if (s == TStringBuf("search_text")) {
                filters[SEARCH]["unclassified"].SetString(MergeSearchText(Ctx));

                continue;
            }
            TStringBuf val = slotData[SEARCH_SLOTS][s].GetString();
            if (s == TStringBuf("playlist")) {
                TStringBuilder playlistText;
                playlistText << val;
                if (!playlistText.empty()) {
                    PlaylistSearch = true;
                    filters[SEARCH][s].SetString(playlistText);
                }
            } else if (!val.empty()) {
                if (s == TStringBuf("album") && val == TStringBuf("album")) {
                    filters[SEARCH]["desiredAnswerType"].SetString("album");
                } else {
                    filters[SEARCH][s].SetString(val);
                }
            }
        }
    }

    if (HasRadioSeeds) {
        if (slotData[RADIO_SEEDS_SLOTS].Has("radio_seeds")) {
            for (const auto& seed : StringSplitter(slotData[RADIO_SEEDS_SLOTS]["radio_seeds"].GetString()).Split(',').SkipEmpty()) {
                filters["radioSeeds"].Push().SetString(seed);
            }
        }
        if (slotData[RADIO_SEEDS_SLOTS].Has("track_to_start_radio_from")) {
            filters["trackToStartRadioFrom"] = slotData[RADIO_SEEDS_SLOTS]["track_to_start_radio_from"].GetString();
        }
    }

    if (DataGetHasKey(slotData, CUSTOM_SLOTS) && slotData[CUSTOM_SLOTS].Has("remote_type")) {
        RequestBody["remoteType"] = slotData[CUSTOM_SLOTS]["remote_type"].GetString();
    }

    if (DataGetHasKey(slotData, CUSTOM_SLOTS) && slotData[CUSTOM_SLOTS].Has(NAlice::NMusic::SLOT_MORNING_SHOW_CONFIG)) {
        MorningShowConfig = NSc::TValue::FromJson(slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_MORNING_SHOW_CONFIG].GetString());
    }

    const bool hasShowType = (
        DataGetHasKey(slotData, CUSTOM_SLOTS)
        && slotData[CUSTOM_SLOTS].Has(NAlice::NMusic::SLOT_MORNING_SHOW_TYPE)
    );
    const TString showType{
        slotData[CUSTOM_SLOTS][NAlice::NMusic::SLOT_MORNING_SHOW_TYPE].GetString({})
    };

    IsChildrenMorningShow = hasShowType && showType == "children"
        || Ctx.GetIsClassifiedAsChildRequest()
        || Ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe;

    if (IsChildrenMorningShow) {
        Ctx.GetAnalyticsInfoBuilder().AddObject("show.type", "children", "children");
    } else if (hasShowType) {
        Ctx.GetAnalyticsInfoBuilder().AddObject("show.type", showType, showType);
    }

    // Special playlists
    if (DataGetHasKey(slotData, CUSTOM_SLOTS) && slotData[CUSTOM_SLOTS].Has("special_playlist")) {
        TString spName(slotData[CUSTOM_SLOTS]["special_playlist"].GetString());
        if (spName == TStringBuf("chart") && (HasSearch || HasFilters)) {
            filters["isNew"].SetBool(true);
        } else {
            if (spName == TStringBuf("origin")) {
                if (MusicQuasarClient) {
                    if (!Ctx.ClientFeatures().SupportsMusicPlayerAllowShots()) {
                        NeedAttentionForAliceShots = true;
                        // Do not return Init..() here.
                    } else {
                        NeedAliceShots = true;
                        return InitWithSpecialPlaylist(spName);
                    }
                } else {
                    NeedAliceShots = true;
                    NeedAttentionForAliceShots = true;
                    // Do not return Init...() here
                    // Just pass to 'General' case
                }
            } else if (spName == TStringBuf("alice")) {
                if (!MusicQuasarClient) {
                    NeedAliceShots = true;
                    NeedAttentionForAliceShots = true;
                    // Do not Init here
                } else if (Ctx.ClientFeatures().SupportsMusicPlayerAllowShots()) {
                    // XXX(a-square): duplicates some logic in CreateTextQuery
                    NeedAliceShots = true;
                    spName = TStringBuf("origin");
                    return InitWithSpecialPlaylist(spName);
                } else {
                    return InitWithSpecialPlaylist(spName);
                }
            } else if (spName == TStringBuf("podcasts_default")) {
                if (Ctx.GetIsClassifiedAsChildRequest() || Ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe) {
                    spName = TStringBuf("podcasts_child");
                }
                return InitWithSpecialPlaylist(spName);
            } else {
                return InitWithSpecialPlaylist(spName);
            }
        }
    }

    if (DataGetHasKey(slotData, CUSTOM_SLOTS) && slotData[CUSTOM_SLOTS].Has(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO)) {
        return InitWithSpecialAnswerInfo(slotData[CUSTOM_SLOTS].Get(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO).GetString());
    }

    InitCommonParams(HasObject ? OBJECT : SEARCH);

    if (RequestBody[Action].DictEmpty()) {
        RequestBody["isGeneral"].SetBool(true);
        Ctx.AddAttention("is_general_playlist");
        return ERequestType::General;
    }

    RequestBody["textQuery"].SetString(MergeTextFromSlots(Ctx, TString{Ctx.Meta().Utterance()}, /* alarm= */ false));

    if (HasFilters) {
        return ERequestType::Filters;
    }

    if (HasObject) {
        return ERequestType::Answer;
    }

    return ERequestType::Text;
}

TQuasarProvider::ERequestType TQuasarProvider::InitWithSearchSnippet(const NSc::TValue& slotData) {
    // We do not need autoplay in quasar
    NSc::TValue answer = TSnippetProvider::MakeDataFromSnippet(Ctx, /* autoplay */ false, slotData[SNIPPET_SLOTS]["snippet"]);
    if (answer.IsNull()) {
        return ERequestType::Error;
    }

    NSc::TValue& obj = RequestBody[Action][OBJECT];
    obj["id"] = answer["id"];
    obj["type"] = answer["type"];

    RequestBody["textQuery"] = answer["title"].IsNull() ? answer["name"] : answer["title"];

    Ctx.DeleteSlot("snippet");

    InitCommonParams(OBJECT);

    return ERequestType::Text;
}

void TQuasarProvider::InitWithYandexMusicAnswerResult(const NSc::TValue& answer) {
    NSc::TValue& obj = RequestBody[Action][OBJECT];
    obj["id"] = answer["id"];
    obj["type"] = answer["type"];
    RequestBody["textQuery"] = answer["title"];
}

TQuasarProvider::ERequestType TQuasarProvider::InitWithSpecialPlaylist(const TString& spName) {
    if (spName.empty()) {
        return ERequestType::Error;
    }

    if (PERSONAL_SPECIAL_PLAYLISTS_DATA.contains(spName)) {
        NSc::TValue& obj = RequestBody[Action][OBJECT];
        obj["id"].SetString(spName);
        obj["type"].SetString("specialPlaylist");
        IsMorningShow = spName == "morningShow";
    } else {
        TYandexMusicAnswer spAnswer(Ctx.ClientFeatures());
        NSc::TValue answer;
        if (!spAnswer.AnswerWithSpecialPlaylist(spName, false, &answer)) {
            return ERequestType::Error;
        }
        IsMeditation = spName.StartsWith("meditation_");
        InitWithYandexMusicAnswerResult(answer);
    }
    InitCommonParams(OBJECT);

    if (spName == TStringBuf("ny_alice_playlist")) {
        RequestBody["modifiers"]["shuffle"].SetBool(true);
        NeedShuffle = true;
    } else if (IsIn(COMMON_SPECIAL_PLAYLISTS, spName)) {
        if (spName != "fairy_tales_default") {
            RequestBody["modifiers"]["repeat"].SetBool(true);
        }
        if (!spName.StartsWith("podcasts_")) {
            RequestBody["modifiers"]["shuffle"].SetBool(true);
        }
    }

    return ERequestType::Playlist;
}

TQuasarProvider::ERequestType TQuasarProvider::InitWithSpecialAnswerInfo(const NSc::TValue& specialAnswerInfo) {
    TYandexMusicAnswer yandexMusicAnswer(Ctx.ClientFeatures());
    NSc::TValue answer;
    if (!yandexMusicAnswer.MakeSpecialAnswer(specialAnswerInfo, false, &answer)) {
        return ERequestType::Error;
    }
    InitWithYandexMusicAnswerResult(answer);
    InitCommonParams(OBJECT);

    return ERequestType::Answer;
}

void TQuasarProvider::InitCommonParams(TStringBuf searchType) {
    RequestBody["contentSettings"].SetString(ToString(Ctx.GetContentRestrictionLevel()));
    RequestBody["userClassificationAge"].SetString(Ctx.GetIsClassifiedAsChildRequest() ? "child" : "adult");

    NSc::TValue& filters = RequestBody[Action];
    // Shuffle and repeat do not make sense for filters-only queries
    TContext::TSlot* slotOrder = Ctx.GetSlot("order");
    NeedShuffle = (!IsSlotEmpty(slotOrder) && slotOrder->Value.GetString() == TStringBuf("shuffle"));

    TContext::TSlot* slotRepeat = Ctx.GetSlot("repeat");
    NeedRepeat = (!IsSlotEmpty(slotRepeat) && slotRepeat->Value.GetString() == TStringBuf("repeat"));

    if (HasSearch || HasObject) {
        NSc::TValue& srch = filters[searchType];

        TContext::TSlot* slotNeedSimilar = Ctx.GetSlot("need_similar");
        if (!IsSlotEmpty(slotNeedSimilar) && slotNeedSimilar->Value.GetString() == TStringBuf("need_similar")) {
            srch["isSimilar"].SetBool(true);
        }

        TContext::TSlot* slotDesiredType = Ctx.GetSlot("desired_answer_type");
        if (!IsSlotEmpty(slotDesiredType) && slotDesiredType->Value.GetString() == TStringBuf("album")) {
            srch["desiredAnswerType"].SetString("album");
        }
    }

    if (NeedShuffle || NeedRepeat) {
        // new format
        RequestBody["modifiers"]["shuffle"].SetBool(NeedShuffle);
        RequestBody["modifiers"]["repeat"].SetBool(NeedRepeat);
    }

    TContext::TSlot* slotPersonal = Ctx.GetSlot("personality");
    if (!IsSlotEmpty(slotPersonal) && slotPersonal->Value.GetString() == TStringBuf("is_personal")) {
        filters["isPersonal"].SetBool(true);
        PersonalSearch = true;
    }

    TContext::TSlot* slotNovelty = Ctx.GetSlot("novelty");
    if (!IsSlotEmpty(slotNovelty)) {
        if (slotNovelty->Value.GetString() == TStringBuf("new")) {
            filters["isNew"].SetBool(true);
        } else if (slotNovelty->Value.GetString() == TStringBuf("discovery")) {
            filters["isDiscovery"].SetBool(true);
        }
    }

    // Biometry
    UserInfo.PlaylistOwnerUid = Ctx.Meta().DeviceState().Music().PlaylistOwner();

    if (Ctx.HasExpFlag("music_biometry")) {
        TBlackBoxAPI blackBoxAPI;
        TDataSyncAPI dataSyncAPI;
        TBiometryDelegate delegate{Ctx, blackBoxAPI, dataSyncAPI};
        NAlice::NBiometry::TBiometry biometry(Ctx.Meta(), delegate, NAlice::NBiometry::TBiometry::EMode::MaxAccuracy);
        if (biometry.HasScores() && biometry.HasIdentity()) {
            UserInfo.HasBiometry = true;
            // Ignore Passport/DataSync errors for now
            biometry.GetUserId(UserInfo.CurrentlySpeakingUid);
            LOG(DEBUG) << "HAS BIOMETRY; UID = " << UserInfo.CurrentlySpeakingUid << Endl;
        }

        // Do not change playlist owner in case of errors
        if (!UserInfo.CurrentlySpeakingUid) {
            UserInfo.CurrentlySpeakingUid = UserInfo.PlaylistOwnerUid;
        }
    }

    if (Ctx.Meta().HasLocation()) {
        const auto& location = Ctx.Meta().Location();
        RequestBody["location"]["lat"].SetNumber(location.Lat());
        RequestBody["location"]["lon"].SetNumber(location.Lon());
    }
}

bool TQuasarProvider::InitWithActionData(const NSc::TValue& actionData) {
    Action = FILTERS;
    NSc::TValue& requestFilters = RequestBody[Action];

    const NSc::TValue& actionObject = actionData["object"];

    if (actionObject.IsNull()) {
        return false;
    }

    // XXX(a-square): logic partially duplicated in ShouldUseWebSearchQuasar
    TStringBuf objectType = actionObject["type"].GetString();
    if (objectType == TStringBuf("tag") || objectType == TStringBuf("Radio")) {

        TStringBuf objectId = actionObject["id"].GetString();
        TStringBuf tagType;
        TStringBuf tagValue;
        if(!objectId.TrySplit('/', tagType, tagValue) && !objectId.TrySplit(':', tagType, tagValue)) {
            return false;
        }
        TVector<TString> similarTypes = {"track", "album", "artist", "playlist", "specialPlaylist"};
        if (Find(similarTypes.begin(), similarTypes.end(), tagType) != similarTypes.end()) {
            NSc::TValue& requestObject = requestFilters[OBJECT];
            requestObject["id"] = tagValue;
            requestObject["type"] = tagType;
            requestObject["isSimilar"].SetBool(true);
            HasObject = true;
        } else if (DataGetHasKey(actionData, MAIN_FILTERS_SLOTS) || DataGetHasKey(actionData, OTHER_FILTERS_SLOTS)) {
            requestFilters[tagType].Push().SetString(tagValue);
            HasFilters = true;
        }
    } else {
        NSc::TValue& requestObject = requestFilters[OBJECT];
        requestObject["id"] = actionObject["id"];
        requestObject["type"].SetString(objectType);
        if (!actionObject["startFromId"].IsNull()) {
            requestObject["startFromId"] = actionObject["startFromId"];
        }
        if (actionObject["startFromPosition"].IsNumber()) {
            requestObject["startFromPosition"] = actionObject["startFromPosition"];
        }
        HasObject = true;
    }

    if (actionData["shuffle"].GetBool()) {
        RequestBody["modifiers"]["shuffle"].SetBool(true);
        NeedShuffle = true;
    }

    if (actionData["repeat"].GetBool()) {
        RequestBody["modifiers"]["repeat"].SetBool(true);
        NeedRepeat = true;
    }

    // Show first track info for analytics/debugging
    if (ShouldShowFirstTrack(Ctx.ClientFeatures(), ERequestType::Text)) {
        RequestBody["showFirstTrack"].SetBool(true);
    }

    RequestBody["contentSettings"].SetString(Ctx.Meta().DeviceState().DeviceConfig().ContentSettings());
    // TODO: When biometry enabled, select owner of Application, not owner of Station
    UserInfo.PlaylistOwnerUid = Ctx.Meta().DeviceState().Music().PlaylistOwner();

    if (actionData.Has("offset")) {
        TrackOffset = actionData["offset"].ForceNumber(0);
    }

    if (RequestBody[Action].DictEmpty()) {
        RequestBody["isGeneral"].SetBool(true);
    }

    requestFilters["isPopular"].SetBool(actionData["isPopular"].GetBool());

    return true;
}

[[nodiscard]] TResultValue TQuasarProvider::Apply(NSc::TValue* out, NSc::TValue&& applyArguments) {
    NSc::TValue musicFilters;
    if (!applyArguments.IsNull()) {
        if (const auto* webAnswer = applyArguments.GetNoAdd(WEB_ANSWER)) {
            if (!webAnswer->IsNull()) {
                const auto& action = RequestBody[Action];
                LOG(INFO) << "Action is " << Action << ", RequestBody is " << RequestBody.ToJson() << Endl;
                musicFilters[OBJECT]["id"] = (*webAnswer)["id"];
                musicFilters[OBJECT]["type"] = (*webAnswer)["type"];
                musicFilters[OBJECT]["isSimilar"] = HasObject ? action.TrySelect("object/isSimilar")
                                                              : action.TrySelect("search/isSimilar");
                musicFilters[OBJECT]["desiredAnswerType"] = HasObject ? action.TrySelect("object/desiredAnswerType")
                                                                      : action.TrySelect("search/desiredAnswerType");
                musicFilters["isNew"] = action["isNew"];
                musicFilters["isPopular"] = action["isPopular"];

                if (NAlice::NMusic::IsTalesAlbumWithChapters((*webAnswer)["id"].ForceString())
                    && !(*webAnswer)["firstTrack"].IsNull()) {
                    musicFilters[OBJECT]["startFromId"] = (*webAnswer)["firstTrack"]["id"];
                }
                if ((*webAnswer)["firstTrack"]["album"]["genre"] == "naturesounds" ||
                    (*webAnswer)["subtype"] == "podcast") {
                    RequestBody["modifiers"]["repeat"].SetBool(true);
                    if (Ctx.HasExpFlag("shuffle_ambient_sounds")) {
                        RequestBody["modifiers"]["shuffle"].SetBool(true);
                    }
                }
            }
        } else {
            LOG(ERR) << "Expected \"" << WEB_ANSWER << "\" inside apply arguments" << Endl;
            ythrow yexception() << WEB_ANSWER << " not found";
        }
    }

    // GenerateCommands uses "room" value
    // Should be called before return, cause it can be overwritten in TQuasarMusicAnswer::ConvertAnswerToOutputFormat
    const auto fillMultiroomData = [&applyArguments, &out]() {
        if (const auto* room = applyArguments.GetNoAdd(MULTIROOM_ROOM); room && !room->IsNull()) {
            (*out)[MULTIROOM_ROOM].SetString(room->GetString());
        }
        if (const auto* flag = applyArguments.GetNoAdd(ACTIVATE_MULTIROOM); flag && !flag->IsNull()) {
            (*out)[ACTIVATE_MULTIROOM].SetString(flag->GetString());
        }
        for (const auto arrFieldName : {MULTIROOM_LOCATION_ROOMS, MULTIROOM_LOCATION_DEVICES, MULTIROOM_LOCATION_GROUPS}) {
            if (const auto* field = applyArguments.GetNoAdd(arrFieldName); field && !field->IsNull()) {
                for (const auto& el : field->GetArray()) {
                    (*out)[arrFieldName].Push(el.GetString());
                }
            }
        }
        if (const auto* field = applyArguments.GetNoAdd(MULTIROOM_LOCATION_SMART_SPEAKER_MODELS); field && !field->IsNull()) {
            for (const auto& el : field->GetArray()) {
                (*out)[MULTIROOM_LOCATION_SMART_SPEAKER_MODELS].Push(el.GetIntNumber());
            }
        }
        if (const auto* field = applyArguments.GetNoAdd(MULTIROOM_LOCATION_EVERYWHERE); field && !field->IsNull()) {
            (*out)[MULTIROOM_LOCATION_EVERYWHERE] = true;
        }
    };

    // Not TStringBuf cause it will be overwritten in TBaseMusicAnswer::MakeBestTrack
    const TString alarmId = out->Has("alarm_id") ? out->Get("alarm_id").ForceString() : "";
    const bool forAlarm = out->Has("alarm_id") || out->Has("for_alarm");

    if (NeedAliceShots && Ctx.ClientFeatures().SupportsMusicSDKPlayer()) {
        // DIALOG-5725 All requests from non-SmartSpeakers move to Music app
        TYandexMusicAnswer answer(Ctx.ClientFeatures());
        answer.AnswerWithSpecialPlaylist("origin", true, out);
        if (NeedAttentionForAliceShots) {
            Ctx.AddAttention("alice_shots_intro");
        }

        fillMultiroomData();
        return TResultValue();
    }

    if (NeedAttentionForAliceShots && MusicQuasarClient) {
        Ctx.AddErrorBlockWithCode(TError(TError::EType::MUSICERROR), TStringBuf("alice_shots_stub"));

        return TResultValue();
    }

    NSc::TValue requestJson = RequestBody.Clone();
    if (!musicFilters.IsNull()) {
        requestJson[Action] = std::move(musicFilters);
    }

    // Final request to Music (object from previous search or original params)
    NHttpFetcher::THandle::TRef handler = CreateMusicRequest(&requestJson);
    if (!handler) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("cannot_create_request")
        );
    }

    NSc::TValue answer;
    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &answer);
    Ctx.AddStatsCounter("MusicQuasar_quasar_query_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        if (reqError->Type == TError::EType::UNAUTHORIZED &&
            reqError->Msg == PAYMENT_REQUIRED)
        {
            NAlice::NBilling::TPromoAvailability promoAvailability;
            if (Ctx.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_CHECK_PLUS_PROMO)) {
                promoAvailability = NVideo::GetPlusPromoAvailability(Ctx, /* needActivatePromoUri = */ true);
            }
            if (promoAvailability.IsAvailable ||
                Ctx.HasExpFlag(NAlice::NExperiments::EXPERIMENTAL_FLAG_TEST_MUSIC_SKIP_PLUS_PROMO_CHECK)
            ) {
                LOG(ERR) << "MUSIC QUASAR API request error: payment-required. Promo-period available." << Endl;

                AddPromoSendPushDirective(Ctx, promoAvailability);

                if (Ctx.HasExpFlag(NAlice::NExperiments::EXP_MUSIC_EXTRA_PROMO_PERIOD) &&
                    Ctx.ClientFeatures().IsMiniSpeakerYandex() &&
                    !promoAvailability.ExtraPeriodExpiresDate.empty()
                ) {
                    LOG(INFO) << "User has extra subscription period until " << promoAvailability.ExtraPeriodExpiresDate << Endl;
                    auto* extraPromoDaysSlot = Ctx.GetOrCreateSlot(NAlice::NBilling::EXTRA_PROMO_PERIOD_EXPIRES_DATE, "string");
                    extraPromoDaysSlot->Value.SetString(promoAvailability.ExtraPeriodExpiresDate);
                    return TError(TError::EType::UNAUTHORIZED, TStringBuf(NAlice::NMusic::ERROR_CODE_EXTRA_PROMO_PERIOD_AVAILABLE));
                }

                return TError(TError::EType::UNAUTHORIZED, TStringBuf("promo_available"));
            } else if (!forAlarm && Ctx.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_SEND_PLUS_BUY_LINK)) {
                // XXX(a-square): sending push notification will not work in HollywoodMusic
                // until we start forwarding the permissions field to BASS.
                // See the bass_adapter library in Hollywood for details.
                Ctx.AddAttention(TStringBuf("plus_push"));
                Ctx.SendPushRequest("music", "link_for_buy_plus", Nothing(), {});
            }
            if (Ctx.ParentFormName() == "personal_assistant.scenarios.music_fairy_tale" && Ctx.HasExpFlag("fairy_tale_yaplus_response")) {
                Ctx.AddAttention(TStringBuf("fairy_tale_noplus"));
            }
        }

        LOG(ERR) << "MUSIC QUASAR API request error: " << reqError->Msg << Endl;
        return reqError;
    }

    LOG(DEBUG) << "RAW MUSIC QUASAR answer: " << answer.ToJson() << Endl;

    if (!forAlarm) {
        if (NeedShuffle) {
            Ctx.AddAttention("music_shuffle");
        }
        if (NeedRepeat) {
            Ctx.AddAttention("music_repeat");
        }
        if (IsMorningShow) {
            Ctx.AddAttention("morning_show");
        }
    }

    if (Ctx.FormName() == MUSIC_PLAY_FORM_NAME &&
        !IsSlotEmpty(Ctx.GetSlot(NAlice::NMusic::SLOT_STREAM)))
    {
        if (!IsSlotEmpty(Ctx.GetSlot("radio_seeds"))) {
            Ctx.AddAttention("radio_filtered_stream"); // An actual stream
        } else {
            Ctx.AddAttention("simplistic_turn_on_message"); // A mock for non-supporting surfaces
        }
    }

    if (TResultValue error = AddMusicAttentions(answer, out)) {
        return error;
    }

    if (TResultValue authorizationError = AddAuthorizationSuggest(Ctx)) {
        return authorizationError;
    }

    if (TResultValue fillError = FillMusicAnswer(answer, out)) {
        return fillError;
    }

    if (!alarmId.Empty()) {
        (*out)["alarm_id"].SetString(alarmId);
    }

    fillMultiroomData();
    return TResultValue();
}

TResultValue TQuasarProvider::SelectBestAnswer(const NSc::TValue& musicAnswer, const NSc::TValue& webAnswer, const NSc::TValue& attentions, NSc::TValue* bestAnswer) const {
    if (attentions.Has(ERRORS) || musicAnswer.IsNull()) {
        if (webAnswer.IsNull()) {
            return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
        }
        (*bestAnswer) = webAnswer;
        return TResultValue();
    }

    if (attentions.Has(ATTENTIONS)) {
        for (const auto& a : attentions[ATTENTIONS].GetArray()) {
            if (a["data"]["code"].GetString() == ATTENTION_FILTERS_NOT_APPLIED
                && a["data"][FILTERS]["isPersonal"].GetBool(false))
            {
                if (webAnswer.IsNull()) {
                    return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
                }
                (*bestAnswer) = webAnswer;
                return TResultValue();
            }
        }
    }

    (*bestAnswer) = musicAnswer;
    return TResultValue();
}

TResultValue TQuasarProvider::FillMusicAnswer(const NSc::TValue& rawAnswer, NSc::TValue* out) {
    const NSc::TValue& result = rawAnswer.TrySelect("result");
    if (result.IsNull()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("parsing_error")
        );
    }

    if (!ServiceAnswer.Init(result)) {
        return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
    }

    if (!ServiceAnswer.ConvertAnswerToOutputFormat(out)) {
        return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
    }

    if (Ctx.ClientFeatures().SupportsMusicSDKPlayer()) {
        if (result["directive"].IsNull()) {
            return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
        }
        Directive = result["directive"].Clone();
        Directive[ALICE_SESSION_ID].SetString(result[ALICE_SESSION_ID].GetString());
    }

    // First track for requests with filters
    const NSc::TValue& rawFirstTrack = result["firstTrack"];
    if (!rawFirstTrack.IsNull()) {
        TYandexMusicAnswer firstTrack(Ctx.ClientFeatures());
        NSc::TValue firstTrackValue;
        firstTrack.InitWithRelatedAnswer(TString("track"), rawFirstTrack, false);
        firstTrack.ConvertAnswerToOutputFormat(&firstTrackValue);
        (*out)["first_track"] = firstTrackValue;
        (*out)["first_track_uri"] = result["firstTrackUri"];
    }

    (*out)["session_id"] = result["sessionId"];

    if (!Ctx.HasExpFlag(NAlice::NExperiments::DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_NEW_CARDS) &&
        Ctx.ClientFeatures().IsSearchApp() && Ctx.ClientFeatures().SupportsDiv2Cards() && !Ctx.GetIsMusicVinsRequest())
    {

        if (Directive["type"].GetString() == "artist") {
            TryDefineAdditionalArtistsAndLikesCount(Directive["id"]["id"].ForceString(), TCgiParameters(), out);
        } else if (Directive["type"].GetString() == "album") {
            if (!result.TrySelect("object/value/artists").GetArray().empty()) {
                TryDefineAdditionalArtistsAndLikesCount(result.TrySelect("object/value/artists").GetArray()[0]["id"].ForceString(), TCgiParameters(), out);
            }
        } else if (Directive["type"].GetString() == "playlist") {
            TryDefineAdditionalPlaylistsAndPlaylistId(Directive["id"], TCgiParameters(), out);
        } else if (Directive["type"].GetString() == "track") {
            if (result.TrySelect("object/value/lyricsAvailable").GetBool(false)) {
                TStringBuilder query;
                TString artistName = ToLowerUTF8(result.TrySelect("object/value/artists").GetArray()[0]["name"].ForceString());
                TString songName = ToLowerUTF8(result.TrySelect("object/value/title").ForceString());
                query << artistName << " " << songName << " текст песни";
                (*out)["lyricsSearchUri"] = GenerateSearchUri(&Ctx, query);
            }
            TryDefineAdditionalTracks(result.TrySelect("object/value/artists").GetArray()[0]["id"].ForceString(), Directive["id"]["id"].ForceString(), TCgiParameters(), out);
        }

        if (!result.TrySelect("object/value/contentWarning").IsNull()) {
            (*out)["contentWarning"] = result.TrySelect("object/value/contentWarning").ForceString();
        }

        TStringBuf pathToCoverUri = (Directive["type"].GetString() == "artist" || Directive["type"].GetString() == "playlist")
                                        ? "object/value/cover/uri" : "object/value/coverUri";
        TryDefineColorForCoverUri(TBaseMusicAnswer::MakeCoverUri(result.TrySelect(pathToCoverUri).ForceString()), TCgiParameters(), out);
    }

    if (Ctx.ClientFeatures().IsSmartSpeaker() && result.TrySelect("result/object/type").GetString() == "playlist" ||
        Ctx.ClientFeatures().SupportsMusicSDKPlayer() && result.TrySelect("object/type").GetString() == "playlist")
    {
        i64 playlist_uid;
        bool isVerifiedUser;
        if (Ctx.ClientFeatures().IsSmartSpeaker()) {
            isVerifiedUser = result.TrySelect("result/object/value/owner/verified").GetBool(false);
            playlist_uid = result.TrySelect("result/object/value/owner/uid").GetIntNumber();
        } else {
            isVerifiedUser = result.TrySelect("object/value/owner/verified").GetBool(false);
            playlist_uid = result.TrySelect("object/value/owner/uid").GetIntNumber();
        }
        TString uid;
        if (!isVerifiedUser &&
            NBASS::TPersonalDataHelper(Ctx).GetUid(uid) &&
            FromString<i64>(uid) != playlist_uid)
        {
            Ctx.AddAttention(NAlice::NMusic::ATTENTION_UNVERIFIED_PLAYLIST);
        }
    }

    return TResultValue();
}

NHttpFetcher::THandle::TRef TQuasarProvider::CreateMusicRequest(NSc::TValue* requestJson, NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    NHttpFetcher::TRequestPtr request = Ctx.GetSources().MusicQuasar(TStringBuf("/quasar/query")).MakeOrAttachRequest(multiRequest);

    AddHeaders(CreateCommonHeaders(Ctx), request.Get());
    request->AddHeader(TStringBuf("Content-Type"), TStringBuf("application/json"));

    TCgiParameters cgi;
    PrepareCgiParams(&cgi);
    request->AddCgiParams(cgi);

    FillPostRequest(requestJson);
    const auto json = requestJson->ToJson();
    request->SetBody(json, TStringBuf("POST"));

    LOG(DEBUG) << "MUSIC QUASAR QUERY BODY: " << json << Endl;
    return request->Fetch();
}

void TQuasarProvider::PrepareCgiParams(TCgiParameters* cgi) const {
    if (UserInfo.HasBiometry && NeedChangeOwner && !UserInfo.CurrentlySpeakingUid.empty()) {
        cgi->InsertUnescaped("currentUser", UserInfo.CurrentlySpeakingUid);
    }
}

void TQuasarProvider::FillPostRequest(NSc::TValue* requestJson) const {
    (*requestJson)["uuid"] = Ctx.Meta().UUID();
    (*requestJson)["deviceId"] = Ctx.Meta().DeviceState().DeviceId();
    (*requestJson)["sessionId"] = Ctx.Meta().DeviceState().Music().SessionId();
    (*requestJson)["experiments"]["sessions"].SetBool(true);
    (*requestJson)["experiments"]["ugcEnabled"].SetBool(Ctx.HasExpFlag("ugc_enabled"));
    (*requestJson)["experiments"]["mediumRuExplicitContent"].SetBool(Ctx.HasExpFlag(EXPERIMENT_RU_EXPLICIT));
    (*requestJson)["experiments"]["isSmartSpeaker"].SetBool(MusicQuasarClient);
    if (IsMeditation) {
        (*requestJson)["experiments"]["isMeditation"].SetBool(true);
    }
    TStringBuilder musicExps;
    Ctx.OnEachExpFlag([requestJson, &musicExps, this](const auto expFlag) {
        if (!TryParseMorningshowConfigId(expFlag, EXPERIMENTAL_FLAG_MORNING_SHOW_CONFIG_ID_PREFIX, *requestJson)
            && expFlag.StartsWith(NAlice::NExperiments::EXP_MUSIC_EXP_FLAG_PREFIX)
        ) {
            musicExps << expFlag << "=" << Ctx.ExpFlag(expFlag).GetRef() << ";";
        }
    });
    if (musicExps) {
        (*requestJson)["experiments"]["aliceExperiments"] = musicExps;
    }
    if (IsChildrenMorningShow) {
        (*requestJson)["experiments"]["morningShowConfigId"].SetIntNumber(MORNING_SHOW_CHILD_CONFIG_ID);
        Ctx.OnEachExpFlag([requestJson](const auto expFlag) {
            TryParseMorningshowConfigId(expFlag, EXPERIMENTAL_FLAG_CHILDREN_SHOW_CONFIG_ID_PREFIX, *requestJson);
        });
    } else if (!MorningShowConfig.IsNull()) {
        (*requestJson)["morningShowConfig"] = MorningShowConfig;
    }
    (*requestJson)["supportedMusicTypes"] = MakeSupportedMusicTypesArray(Ctx.ClientFeatures());
    (*requestJson)["clientIp"] = Ctx.UserIP();
    (*requestJson)["requestId"] = Ctx.Meta().RequestId();
    (*requestJson)["timestamp"] = Ctx.Now().MilliSeconds();
    (*requestJson)["currentlyPlaying"] = Ctx.Meta().DeviceState().Music().CurrentlyPlaying().TrackId();
    if (!Ctx.Meta().MusicFrom()->empty()) {
        (*requestJson)["from"] = Ctx.Meta().MusicFrom();
    }
}

void TQuasarProvider::AddSuggest(const NSc::TValue& result) {
    if (MusicQuasarClient) {
        return;
    }

    TStringBuf resType = result.Get("type").GetString();
    if (!resType || !result.Has("id")) {
        return;
    }

    TStringBuilder path;
    path << '/';

    if (resType == TStringBuf("artist")) {
        // similar artists
        path << "artists/" << result.Get("id").ForceString() << "/brief-info";
        MakeSuggest(resType, path, TStringBuf("result/similarArtists"), TStringBuf(), TCgiParameters());
    } else if (resType == TStringBuf("track")) {
        // similar artists for all
        path << "artists/" << result.TrySelect("artists/0/id").ForceString() << "/brief-info";
        MakeSuggest(TStringBuf("artist"), path, TStringBuf("result/similarArtists"), TStringBuf(), TCgiParameters());
    } else if (resType == TStringBuf("album") && !result.TrySelect("artists").GetArray().empty()) {
        const NSc::TValue& artist = result.TrySelect("artists/0");
        if (artist.Get("is_various").GetBool(false)) {
            // If there are many artists in album,
            // suggest best artists of this genre
            path << "genre-overview";
            TCgiParameters cgi;
            cgi.InsertEscaped("genre", result.Get("genre").GetString());
            cgi.InsertEscaped("tracks-count", "0");
            cgi.InsertEscaped("artists-count", ToString(3));
            cgi.InsertEscaped("albums-count", "0");
            cgi.InsertEscaped("promotions-count", "0");
            MakeSuggest(TStringBuf("artist"), path, TStringBuf("result/artists"), TStringBuf(), cgi);
        } else {
            // albums of the first artist (excluding this album)
            path << "artists/" << artist.Get("id").ForceString() << "/brief-info";
            MakeSuggest(TStringBuf("album"), path, TStringBuf("result/albums"), result.Get("id").ForceString(), TCgiParameters());
        }
    }
}

void TQuasarProvider::MakeSuggest(TStringBuf section, TStringBuf path, TStringBuf jsonPath, TStringBuf id, const TCgiParameters& cgi) {
    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicSuggests(path), cgi);
    if (!handler) {
        return;
    }
    NSc::TValue json;

    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &json);
    Ctx.AddStatsCounter("MusicSuggests_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return;
    }

    bool autoplay = true;

    size_t counter = 0;
    for (const auto& j : json.TrySelect(jsonPath).GetArray()) {
        if (!id.empty() && id == j.Get("id").ForceString()) {
            continue;
        }
        TYandexMusicAnswer answer(Ctx.ClientFeatures());
        answer.InitWithRelatedAnswer(ToString(section), j, autoplay);
        NSc::TValue suggest;
        if (!answer.ConvertAnswerToOutputFormat(&suggest)) {
            continue;
        }

        NSc::TValue formUpdate;
        formUpdate["name"] = MUSIC_PLAY_FORM_NAME;
        formUpdate["resubmit"].SetBool(true);
        NSc::TValue& slot = formUpdate["slots"].SetArray().Push();
        slot["name"].SetString(TStringBuf("answer"));
        slot["type"].SetString(TStringBuf("music_result"));
        slot["optional"].SetBool(true);
        slot["value"] = suggest;
        Ctx.AddSuggest(TStringBuilder() << TStringBuf("music__suggest_") << section, suggest, formUpdate);

        ++counter;
        if (counter >= 3) {
            break;
        }
    }
}

NSc::TValue TQuasarProvider::ConvertAttentions(const NSc::TValue& musicAttentions) const {
    NSc::TValue vinsAttentions;
    for (const auto& a : musicAttentions.GetArray()) {
        if (a["code"].IsNull()) {
            continue;
        }
        TString origCode(a["code"].GetString());
        if (origCode == TStringBuf("user-not-authenticated")) {
            vinsAttentions[ERRORS][ERROR_UNAUTHORIZED].SetString("");
        } else if (origCode == TStringBuf("nothing-was-found")) {
            vinsAttentions[ERRORS][ERROR_MUSIC_NOT_FOUND].SetString("");
        } else if (origCode == TStringBuf("forbidden-content")) {
            vinsAttentions[ERRORS]["forbidden-content"].SetString("");
        } else if (origCode == TStringBuf("contains-adult-content") && Ctx.GetContentRestrictionLevel() == EContentRestrictionLevel::Safe) {
            vinsAttentions[ERRORS][ERROR_RESTRICTED_BY_CHILD_CONTENT_SETTINGS].SetString("");
        } else if (origCode == TStringBuf("explicit-content-filtered") || origCode == TStringBuf("may-contain-explicit-content")) {
            if (!(
                Ctx.Meta().DeviceState().DeviceConfig().ContentSettings() == TStringBuf("children") ||
                Ctx.HasExpFlag(EXPERIMENT_RU_EXPLICIT)
            )) {
                continue;
            }
            NSc::TValue& attention = vinsAttentions[ATTENTIONS].Push();
            attention["data"]["code"].SetString(origCode);
            attention["type"].SetString("explicit_content");
        } else if (origCode == TStringBuf("filters-not-applied") || origCode == TStringBuf("filters-not-parsed")) {
            NSc::TValue& attention = vinsAttentions[ATTENTIONS].Push();
            TString code(ATTENTION_FILTERS_NOT_APPLIED);

            for (const auto& filter : a["data"].DictKeys()) {
                if (filter == SEARCH) {
                    if (a["data"][filter].IsDict() && a["data"][filter].DictEmpty()
                        || a["data"][filter].IsArray() && a["data"][filter].ArrayEmpty())
                    {
                        continue;
                    }
                    code = TStringBuf("search_filters_not_applied");
                    attention["data"][SEARCH] = a["data"][filter];
                } else {
                    if (a["data"][filter].IsDict() && a["data"][filter].DictEmpty()
                        || a["data"][filter].IsArray() && a["data"][filter].ArrayEmpty())
                    {
                        continue;
                    }
                    attention["data"][FILTERS][filter] = a["data"][filter];
                }
            }
            attention["data"]["code"] = code;
            attention["type"].SetString("music_filters");
        } else if (origCode == TStringBuf("user-not-premium")) {
            vinsAttentions[ATTENTIONS][SUGGEST_YAPLUS].SetString("");
        } else {
            NSc::TValue& attention = vinsAttentions[ATTENTIONS].Push();
            attention["type"].SetString(origCode);
        }
    }
    return vinsAttentions;
}

TMaybe<double> TryGetScoreFromExpPrefix(const TStringBuf expPrefix, const TContext& ctx) {
    if (const auto& suffix = ctx.GetValueFromExpPrefix(expPrefix)) {
        double score;
        if (TryFromString<double>(suffix.GetRef(), score)) {
            return score;
        }
    }

    return Nothing();
}

TResultValue TQuasarProvider::AddMusicAttentions(const NSc::TValue& answer, NSc::TValue* out) {
    const NSc::TValue& musicAttentions = answer.TrySelect("result/attentions");
    const bool forAlarm = out->Has("for_alarm") || out->Has("alarm_id");
    NSc::TValue vinsAttentions = ConvertAttentions(musicAttentions);

    if (vinsAttentions.Has(ERRORS) && !vinsAttentions[ERRORS].DictEmpty()) {
        if (vinsAttentions[ERRORS].Has(ERROR_UNAUTHORIZED)) {
            if (forAlarm) {
                return TError(TError::EType::UNAUTHORIZED, ERROR_UNAUTHORIZED);
            } else {
                return TError(TError::EType::UNAUTHORIZED, TryAddAuthorizationSuggest(Ctx) ? ERROR_UNAUTHORIZED : ERROR_DO_NOT_SEND);
            }
        }
        if (vinsAttentions[ERRORS].Has(ERROR_MUSIC_NOT_FOUND)) {
            if (forAlarm) {
                return TError(TError::EType::MUSICERROR, ERROR_MUSIC_NOT_FOUND);
            } else if (TryFallBackToMusicVertical(RequestBody["isGeneral"].GetBool(false))) {
                return TError(TError::EType::MUSICERROR, ERROR_DO_NOT_SEND);
            }
        }
        TStringBuf errorText = vinsAttentions[ERRORS].DictKeys().front();
        if (!errorText.empty()) {
            if (errorText == ERROR_RESTRICTED_BY_CHILD_CONTENT_SETTINGS && IsFirstTrackPodcast(answer)) {
                Ctx.AddAttention(ATTENTION_FORBIDDEN_PODCAST);
            }
            return TError(TError::EType::MUSICERROR, errorText);
        }
    }

    if (Ctx.IsAuthorizedUser() &&
        Ctx.ClientFeatures().SupportsMusicSDKPlayer() &&
        vinsAttentions[ATTENTIONS].Has(SUGGEST_YAPLUS))
    {
        NSc::TValue data;
        data["uri"] = YAPLUS_URL_SOURCE_PP;
        Ctx.AddAttention(SUGGEST_YAPLUS);
        Ctx.AddSuggest(TStringBuf("yaplus"), std::move(data));
    }

    if (Ctx.ClientFeatures().SupportsMusicPlayer()) {
        Ctx.AddAttention(ATTENTION_SUPPORTS_MUSIC_PLAYER);
    }

    if (!forAlarm) {
        for (auto& a : vinsAttentions[ATTENTIONS].GetArray()) {
            Ctx.AddAttention(a["type"].GetString(), a["data"]);
        }
        if (!vinsAttentions.Has(ATTENTIONS) && (*out)["type"].GetString() != FILTERS && HasFilters) {
            Ctx.AddAttention(TStringBuf("music_filters"), NSc::TValue::FromJson("{\"code\":\"search_result_with_filters\"}"));
        }
    }

    return TResultValue();
}

bool TQuasarProvider::TryFallBackToMusicVertical(bool isGeneral) const {
    TString uri = GenerateMusicVerticalUri(Ctx);
    if (uri.Empty()) {
        return false;
    }
    NSc::TValue data;
    data["uri"] = uri;
    Ctx.AddSuggest(TStringBuf("fallback_to_music_vertical"), data);
    Ctx.AddCommand<TMusicVerticalShowDirective>(TStringBuf("open_uri"), std::move(data));
    if (isGeneral) {
        Ctx.AddAttention(TStringBuf("fallback_to_music_vertical_general"));
    } else {
        Ctx.AddAttention(TStringBuf("fallback_to_music_vertical_nothing_found"));
    }
    return true;
}

bool TQuasarProvider::TryConvertQuasarRequest(const bool multiroomStarted) {
    if (Ctx.GetIsMusicVinsRequest() &&
        Ctx.ClientFeatures().SupportsAudioClient() &&
        Ctx.HasExpFlag(NAlice::NExperiments::EXP_REDIRECT_TO_THIN_PLAYER) &&
        !multiroomStarted)
    {
        QuasarRequestConverter converter(RequestBody);
        if (converter.Convert()) {
            if (const auto action = Ctx.InputAction(); action.Defined()) {
                const NSc::TValue& actionData = action.Get()->Data;
                if (const TStringBuf alarmId = actionData["alarm_id"].GetString()) {
                    converter.SetAlarmId(alarmId);
                }
            }
            NAlice::NScenarios::TStackEngine stackEngine;
            stackEngine.AddActions()->MutableNewSession();

            auto& parsedUtterance = *stackEngine.AddActions()->MutableResetAdd()->AddEffects()->MutableParsedUtterance();
            *parsedUtterance.MutableTypedSemanticFrame() = converter.GetTypedSemanticFrame();

            auto& atm = *parsedUtterance.MutableAnalytics();
            atm.SetProductScenario("Music");
            atm.SetPurpose("play_music");
            atm.SetOriginInfo("ThickPlayer");
            atm.SetOrigin(NAlice::TAnalyticsTrackingModule::Scenario);

            Ctx.AddStackEngine(stackEngine);
            LOG(INFO) << "Redirected to thin player" << Endl;
            return true;
        } else {
            LOG(INFO) << "Can not convert quasar request to semantic frame" << Endl;
        }
    }
    return false;
}

void TQuasarProvider::MakeBlocks(const NSc::TValue& result) {
    if (MusicQuasarClient) {
        NSc::TValue commandData;
        commandData["uri"].SetString(TStringBuf(""));
        commandData["first_track_id"].SetString(result["first_track"]["id"].ForceString());
        commandData["first_track_genre"].SetString(GetFirstTrackGenre(result));
        commandData["first_track_duration"].SetString(result["first_track"]["durationMs"].ForceString());
        commandData["first_track_album_type"].SetString(result["first_track"]["album"]["type"].ForceString());
        commandData["first_track_human_readable"].SetString(CreateHumanReadableTitle(result["first_track"]));
        commandData["session_id"].SetString(result["session_id"].ForceString());
        if (UserInfo.HasBiometry) {
            commandData["uid"].SetString(NeedChangeOwner ? UserInfo.CurrentlySpeakingUid : UserInfo.PlaylistOwnerUid);
        }
        commandData["offset"].SetNumber(Max(TrackOffset, result["offset"].ForceNumber(0)));
        const TStringBuf alarmId = result["alarm_id"].GetString();
        if (alarmId) {
            commandData["alarm_id"].SetString(alarmId);
        }

        bool multiroomStarted = MayBeAddStartMultiroomDirective(Ctx, result, commandData);
        if (!TryConvertQuasarRequest(multiroomStarted)) {
            Ctx.AddCommand<TMusicSmartSpeakerPlayDirective>(TStringBuf("music_play"), std::move(commandData));
        }
    } else if (Ctx.ClientFeatures().SupportsMusicPlayer()) {
        NSc::TValue commandData;
        if (NeedAliceShots) {
            // DIALOG-5725 Playlist wit Alice shots enable only in Music app or SmartSpeakers
            TString playUri(result["uri"].GetString());
            commandData["uri"].SetString(playUri);
            Ctx.AddCommand<TMusicAppPlayDirective>(TStringBuf("open_uri"), std::move(commandData));
            TryAddDivCard(result, playUri, "music_play");
        } else if (!Directive.IsNull()) {
            TString playUri(MakeDeeplink(Ctx.ClientFeatures(), Directive));
            commandData["uri"].SetString(playUri);
            Ctx.AddCommand<TMusicInternalPlayerPlayDirective>(TStringBuf("open_uri"), std::move(commandData));
            TryAddDivCard(result, playUri, "music_play_alicesdk");
        } else {
            return;
        }
    }
}

void TQuasarProvider::TryDefineAdditionalArtistsAndLikesCount(TStringBuf artistId, const TCgiParameters& cgi, NSc::TValue* out) {
    TStringBuf path = TString::Join("/artists/", artistId, "/brief-info");
    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicSuggests(path), cgi);
    if (!handler) {
        return;
    }

    NSc::TValue json;
    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &json);
    Ctx.AddStatsCounter("MusicSuggests_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return;
    }

    for (const auto& artist : json.TrySelect("result/similarArtists").GetArray()) {
        if (artist.Get("id").ForceString().empty() || artistId == artist.Get("id").ForceString()) {
            continue;
        } else {
            NSc::TValue additionalArtist;
            additionalArtist["artistId"] = artist.Get("id").ForceString();
            additionalArtist["name"] = artist.Get("name").ForceString();
            additionalArtist["imageUri"] = TBaseMusicAnswer::MakeCoverUri(artist.TrySelect("cover/uri").ForceString());
            AdditionalArtists.Push(additionalArtist);
        }
    }

    (*out)["likesCount"] = json.TrySelect("result/artist/likesCount").ForceString("0");
    Shuffle(AdditionalArtists.GetArrayMutable().begin(), AdditionalArtists.GetArrayMutable().end());
    (*out)["additionalArtists"] = AdditionalArtists;
}

void TQuasarProvider::TryDefineAdditionalPlaylistsAndPlaylistId(const NSc::TValue& playlistId, const TCgiParameters& cgi, NSc::TValue* out) {
    TStringBuf path = TString::Join("/users/", playlistId["uid"].ForceString(), "/playlists/",
                                    playlistId["kind"].ForceString(), "?rich-tracks=false&withSimilarsLikesCount=true");

    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicSuggests(path), cgi);
    if (!handler) {
        return;
    }

    NSc::TValue json;
    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &json);
    Ctx.AddStatsCounter("MusicSuggests_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return;
    }

    for (const auto& playlist : json.TrySelect("result/similarPlaylists").GetArray()) {
        if (playlist.Get("kind").ForceString().empty() || playlistId["kind"].GetString() == playlist.Get("kind").ForceString()) {
            continue;
        } else {
            NSc::TValue additionalPlaylist;
            additionalPlaylist["kind"] = playlist.Get("kind").ForceString();
            additionalPlaylist["title"] = playlist.Get("title").ForceString();
            additionalPlaylist["imageUri"] = TBaseMusicAnswer::MakeCoverUri(playlist.Get("ogImage").ForceString());
            AdditionalPlaylists.Push(additionalPlaylist);
        }
    }

    (*out)["ownerLogin"] = json.TrySelect("result/owner/login").ForceString();
    (*out)["kind"] = playlistId["kind"].ForceString();
    Shuffle(AdditionalPlaylists.GetArrayMutable().begin(), AdditionalPlaylists.GetArrayMutable().end());
    (*out)["additionalPlaylists"] = AdditionalPlaylists;

}

void TQuasarProvider::TryDefineAdditionalTracks(TStringBuf artistId, TStringBuf trackId, const TCgiParameters& cgi, NSc::TValue* out) {
    TStringBuf path = TString::Join("/artists/", artistId, "/tracks");
    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicSuggests(path), cgi);
    if (!handler) {
        return;
    }

    NSc::TValue json;
    const auto startMillis = NAlice::SystemTimeNowMillis();
    TResultValue reqError = WaitAndParseResponse(handler, &json);
    Ctx.AddStatsCounter("MusicSuggests_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);
    if (reqError) {
        return;
    }

    for (const auto& track : json.TrySelect("result/tracks").GetArray()) {
        if (track.Get("id").ForceString().empty() || trackId == track.Get("id").ForceString()) {
            continue;
        } else {
            NSc::TValue additionalTrack;
            additionalTrack["trackId"] = track.Get("id").ForceString();
            additionalTrack["title"] = track.Get("title").ForceString();
            additionalTrack["imageUri"] = TBaseMusicAnswer::MakeCoverUri(track.Get("coverUri").ForceString());
            AdditionalTracks.Push(additionalTrack);
        }
    }

    NSc::TValue copy = AdditionalTracks.Clone();
    Shuffle(copy.GetArrayMutable().begin(), copy.GetArrayMutable().end());
    (*out)["additionalTracks"] = copy;
}

TStringBuf GetMetaPathFromCoverUri(TStringBuf coverUri) {
    TStringBuf handler;
    TStringBuf spaceName;
    TStringBuf groupId;
    TStringBuf imageName;

    coverUri = coverUri.RBefore('/');
    coverUri.RSplit('/', coverUri, imageName);
    coverUri.RSplit('/', coverUri, groupId);
    coverUri.RSplit('/', coverUri, spaceName);
    spaceName.Split('-', handler, spaceName);

    return TStringBuilder() << '/' << handler << "info-" << spaceName << '/' << groupId << '/' << imageName << "/meta";
}

void TQuasarProvider::TryDefineColorForCoverUri(TStringBuf coverUri, const TCgiParameters& cgi, NSc::TValue* out) {
    if (coverUri == DEFAULT_COVER) {
        return;
    }

    auto path = GetMetaPathFromCoverUri(coverUri);
    NHttpFetcher::THandle::TRef handler = CreateRequestHandler(Ctx.GetSources().MusicAvatarsColor(path), cgi);
    if (!handler) {
        return;
    }

    const auto startMillis = NAlice::SystemTimeNowMillis();
    NHttpFetcher::TResponse::TConstRef resp = handler->Wait();
    Ctx.AddStatsCounter("MusicAvatarsColor_milliseconds", NAlice::SystemTimeNowMillis() - startMillis);

    NSc::TValue json;
    if (resp->IsError() || resp->Data.empty() || !NSc::TValue::FromJson(json, resp->Data)) {
        return;
    }

    (*out)["mainColor"] = json["MainColor"].ForceString();
    (*out)["secondColor"] = json["SecondColor"].ForceString();
    (*out)["blackLogoNeeded"].SetBool(false);

    if (json["MainColor"].ForceString().size() == RGB_COLOR_LEN &&
        json["SecondColor"].ForceString().size() == RGB_COLOR_LEN)
    {
        auto green = stoi(json["MainColor"].ForceString().substr(3, 2), 0, 16) +
                         stoi(json["SecondColor"].ForceString().substr(3, 2), 0, 16);
        auto blue = stoi(json["MainColor"].ForceString().substr(5, 2), 0, 16) +
                        stoi(json["SecondColor"].ForceString().substr(5, 2), 0, 16);

        // if green and blue in RGB are both between 225 and 255 then a black logo is needed (450 is 225 * 2)
        if (green >= 450 && blue >= 450)  {
            (*out)["blackLogoNeeded"].SetBool(true);
        }
    }
}

TString TQuasarProvider::MakeDeeplink(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive) {
    TCgiParameters params = TYandexMusicAnswer::MakeDeeplinkParams(clientFeatures, directive);
    if (params.Has("track") && !Ctx.HasExpFlag(DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_ADDITIONAL_TRACKS)) {
        TStringBuilder additionalTracks;
        for (const auto& additionalTrack : AdditionalTracks.GetArray()) {
            additionalTracks << additionalTrack["trackId"].GetString() << ',';
        }
        params.ReplaceUnescaped("track", TString::Join(params.Get("track"), ',', additionalTracks));
    }
    return TStringBuilder() << "musicsdk://?" << params.Print();
}

} // namespace NMusic

} // namespace NBASS
