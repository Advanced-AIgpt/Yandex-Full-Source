#include "providers.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/bass/libs/fetcher/neh.h>
#include <alice/bass/libs/logging_v2/logger.h>
#include <alice/bass/libs/push_notification/create_callback_data.h>
#include <alice/bass/libs/push_notification/request.h>

#include <alice/library/music/defs.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/langs/langs.h>

namespace NBASS::NMusic {

namespace {

// Order of slots for merging for old scenario (where we have only text search)
const TVector<TStringBuf> SLOTS_ORDER_FOR_TEXT = {
    TStringBuf("novelty"),
    TStringBuf("need_similar"),
    TStringBuf("mood"),
    TStringBuf("genre"),
    TStringBuf("artist"),
    TStringBuf("album"),
    TStringBuf("track"),
    TStringBuf("playlist"),
    TStringBuf("activity"),
    TStringBuf("epoch"),
    TStringBuf("language"),
    TStringBuf("vocal"),
    TStringBuf("target_type"), // only used for alarms
};

const TVector<TStringBuf> SLOTS_ORDER_FOR_TEXT_LITE = {
    TStringBuf("artist"),
    TStringBuf("album"),
    TStringBuf("track"),
    TStringBuf("playlist"),
    TStringBuf("target_type"), // only used for alarms
};

const THashSet<TStringBuf> RUSSIAN_GENRES = {
    TStringBuf("rock"),
    TStringBuf("pop"),
    TStringBuf("rap"),
    TStringBuf("estrada"),
    TStringBuf("bard")
};

const THashMap<TStringBuf, TStringBuf> MUSIC_REDIRECTS = {
    { TStringBuf("album"), TStringBuf("https://redirect.appmetrica.yandex.com/serve/675548979559187313") },
    { TStringBuf("artist"), TStringBuf("https://redirect.appmetrica.yandex.com/serve/459376213719336466") },
    { TStringBuf("playlist"), TStringBuf("https://redirect.appmetrica.yandex.com/serve/243203434541549406") },
    { TStringBuf("track"), TStringBuf("https://redirect.appmetrica.yandex.com/serve/1107834798845505384") },
};

inline void AddSpaceToNonEmptyText(TStringBuilder& text) {
    if (!text.empty()) {
        text << " ";
    }
}

bool ShouldUseWebSearchQuasar(const NSc::TValue& slotData, const NSc::TValue& actionData) {
    const bool hasSearch = DataGetHasKey(slotData, SEARCH_SLOTS);

    bool hasFilters = DataGetHasKey(slotData, MAIN_FILTERS_SLOTS) || DataGetHasKey(slotData, OTHER_FILTERS_SLOTS);
    bool hasObject = DataGetHasKey(slotData, OBJECT_SLOTS);

    // TODO(a-square): is this actually useful, or is the server action never search-based?
    if (const auto& actionObject = actionData["object"]; actionObject.IsDict()) {
        const TStringBuf objectType = actionObject["type"].GetString();
        if (objectType == TStringBuf("tag")) {
            TStringBuf objectId = actionObject["id"].GetString();
            TStringBuf tagType;
            TStringBuf tagValue;
            if (objectId.TrySplit('/', tagType, tagValue) || objectId.TrySplit(':', tagType, tagValue)) {
                hasFilters = true;
            }
        } else {
            hasObject = true;
        }
    }

    const bool personalSearch = slotData[CUSTOM_SLOTS]["personality"].GetString() == TStringBuf("is_personal");
    const bool playlistSearch = !slotData[SEARCH_SLOTS]["playlist"].GetString().empty();

    // XXX(a-square): currently just accepting that we will waste a search when the shots logic is on;
    // the alternative is duplication of some pretty brittle code
    const bool shotsLogic = false;

    LOG(DEBUG) << "ShouldUseWebSearchQuasar: "
               << "hasSearch = " << hasSearch
               << ", hasFilters = " << hasFilters
               << ", hasObject = " << hasObject
               << ", personalSearch = " << personalSearch
               << ", playlistSearch = " << playlistSearch
               << ", shotsLogic = " << shotsLogic
               << Endl;

    return
        hasSearch &&
        !hasObject &&
        !personalSearch &&
        !playlistSearch &&
        !shotsLogic;
}

bool ShouldUseWebSearchLegacy(const NSc::TValue& slotData) {
    const auto& playlistId = slotData.TrySelect("object/playlist_id");
    if (playlistId.IsString()) {
        LOG(DEBUG) << "ShouldUseWebSearchLegacy: playlist" << Endl;
        return false; // !SpecialPlaylist
    }

    if (DataGetHasKey(slotData, RESULT_SLOTS) ||
        DataGetHasKey(slotData, SEARCH_SLOTS) ||
        DataGetHasKey(slotData, OTHER_FILTERS_SLOTS)
    ) {
        LOG(DEBUG) << "ShouldUseWebSearchLegacy: result, search or other filter slots" << Endl;
        return true;
    }

    if (DataGetHasKey(slotData, CUSTOM_SLOTS)) {
        if (
            slotData[CUSTOM_SLOTS].Has("special_playlist") ||
            slotData[CUSTOM_SLOTS].Has(NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO)
        ) {
            LOG(DEBUG) << "ShouldUseWebSearchLegacy: special playlist" << Endl;
            return false; // !SpecialPlaylist && !SpecialAnswerRawInfo
        }

        if (slotData[CUSTOM_SLOTS].Has("novelty") && slotData["custom"]["novelty"].GetString() == TStringBuf("new")) {
            LOG(DEBUG) << "ShouldUseWebSearchLegacy: novelty = new" << Endl;
            return true;
        }
    }

    if (DataGetHasKey(slotData, SNIPPET_SLOTS)) {
        LOG(DEBUG) << "ShouldUseWebSearchLegacy: snippet slots" << Endl;
        return true; // SNIPPET provider
    }

    if (!DataGetHasKey(slotData, MAIN_FILTERS_SLOTS) || CheckRadioFilters(slotData[MAIN_FILTERS_SLOTS])) {
        LOG(DEBUG) << "ShouldUseWebSearchLegacy: main filters match YaRadio" << Endl;
        return false; // YARADIO provider
    }

    LOG(DEBUG) << "ShouldUseWebSearchLegacy: default to YaMusic" << Endl;
    return true;
}

} // namespace

const THashMap<TStringBuf, TVector<TStringBuf>> SLOT_NAMES = {
    {
        SEARCH_SLOTS, {
            TStringBuf("artist"),
            TStringBuf("album"),
            TStringBuf("track"),
            TStringBuf("playlist"),
            TStringBuf("search_text"),
        }
    },
    {
        OBJECT_SLOTS, {
            TStringBuf("artist_id"),
            TStringBuf("album_id"),
            TStringBuf("track_id"),
            TStringBuf("playlist_id"),
        }
    },
    {
        MAIN_FILTERS_SLOTS, {
            TStringBuf("genre"),
            TStringBuf("mood"),
            TStringBuf("activity"),
            TStringBuf("epoch"),
            TStringBuf("language"),
        }
    },
    {
        OTHER_FILTERS_SLOTS, {
            TStringBuf("vocal"),
        }
    },
    {
        RADIO_SEEDS_SLOTS, {
            TStringBuf("radio_seeds"),
            TStringBuf("track_to_start_radio_from"),
        }
    },
    {
        CUSTOM_SLOTS, {
            TStringBuf("novelty"),
            TStringBuf("personality"),
            TStringBuf("need_similar"),
            TStringBuf("special_playlist"),
            TStringBuf("remote_type"),
            NAlice::NMusic::SLOT_MORNING_SHOW_CONFIG,
            NAlice::NMusic::SLOT_MORNING_SHOW_TYPE,
            NAlice::NMusic::SLOT_SPECIAL_ANSWER_INFO,
            NAlice::NMusic::SLOT_LOCATION,
            NAlice::NMusic::SLOT_LOCATION_ROOM,
            NAlice::NMusic::SLOT_LOCATION_GROUP,
            NAlice::NMusic::SLOT_LOCATION_DEVICE,
            NAlice::NMusic::SLOT_LOCATION_EVERYWHERE,
            NAlice::NMusic::SLOT_LOCATION_SMART_SPEAKER_MODEL,
            NAlice::NMusic::SLOT_ACTIVATE_MULTIROOM
        }
    },
    {
        SNIPPET_SLOTS,
        {
            TStringBuf("snippet"),
        }
    },
    {
        RESULT_SLOTS,
        {
            TStringBuf("answer"),
        }
    }
};

const TStringBuf PATH_FOR_LIKE = "/likes/tracks/add";

TResultValue AddAuthorizationSuggest(TContext& ctx) {
    return TryAddAuthorizationSuggest(ctx, /* shouldSuggestAuthorization */ true)
        ? TError(TError::EType::UNAUTHORIZED, ERROR_UNAUTHORIZED)
        : TResultValue();
}

bool TryAddAuthorizationSuggest(TContext& ctx, const bool shouldSuggestAuthorization) {
    const TClientInfo& clientInfo = ctx.MetaClientInfo();
    const TClientFeatures& clientFeatures = ctx.ClientFeatures();
    if (ctx.IsAuthorizedUser() || clientInfo.IsSmartSpeaker() || clientInfo.IsYaMusic()) {
        return false;
    }
    ctx.AddAttention("unauthorized");
    if (clientFeatures.SupportsMusicSDKPlayer() && clientFeatures.SupportsButtons()) {
        if (shouldSuggestAuthorization) {
            ctx.AddAttention("suggest_authorization_from_music_play");
        }
        const TString uri = GenerateAuthorizationUri(ctx);
        if (uri.Empty()) {
            return shouldSuggestAuthorization;
        }
        NSc::TValue data;
        data["uri"] = uri;
        ctx.AddSuggest("authorize", std::move(data));
    }
    return false;
}

TString MergeSearchText(TContext& ctx) {
    const TContext::TSlot* slotSearchText = ctx.GetSlot("search_text");
    TStringBuilder searchText;

    if (!IsSlotEmpty(slotSearchText)) {
        const NSc::TValue& value = slotSearchText->Value;
        if (value.IsArray()) {
            for (const auto& t : value.GetArray()) {
                AddSpaceToNonEmptyText(searchText);
                searchText << t.GetString();
            }
        } else if (value.IsString()) {
            searchText << value.GetString();
        }
    }
    return searchText;
}

TString MergeTextFromSlots(TContext& ctx, const TString& utterance, const bool alarm) {
    TStringBuilder searchText;

    if (!alarm && ctx.HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_USE_UTTERANCE_FOR_SEARCH_QUERY)) {
        return utterance;
    }

    if (alarm || !ctx.ClientFeatures().HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_BARE_SEARCH_TEXT)) {
        const TVector<TStringBuf>& slotList =
            (!alarm && ctx.ClientFeatures().HasExpFlag(EXPERIMENTAL_FLAG_MUSIC_LITE_SEARCH_TEXT))
                ? SLOTS_ORDER_FOR_TEXT_LITE
                : SLOTS_ORDER_FOR_TEXT;

        for (const auto& slotName : slotList) {
            const TContext::TSlot* slot = ctx.GetSlot(slotName);
            if (!IsSlotEmpty(slot)) {
                TString slotText(slot->SourceText.GetString());
                if (slotText.empty()) {
                    continue;
                }
                AddSpaceToNonEmptyText(searchText);
                searchText << slotText;
            }
        }
    }

    TString tmp = MergeSearchText(ctx);

    if (!tmp.empty()) {
        AddSpaceToNonEmptyText(searchText);
        searchText << tmp;
    }

    return searchText;
}

TBaseMusicProvider::TBaseMusicProvider(TContext& ctx)
    : IMusicProvider(ctx)
{
}

bool TBaseMusicProvider::TryAddDivCard(const NSc::TValue& result, const TStringBuf playUri, const TStringBuf logId) const {
    static const THashSet<TStringBuf> SUPPORTED_CARD_TYPES = { "track", "album", "artist", "playlist", "podcast", "podcast-episode" };

    if (!Ctx.ClientFeatures().SupportsDivCards() || Ctx.HasExpFlag(EXPERIMENTAL_FLAG_DISABLE_MUSIC_ATTRACTIVE_CARD)) {
        return false;
    }

    TStringBuf cardType = result["type"];
    TStringBuf cardSubtype = result["subtype"];
    if (!SUPPORTED_CARD_TYPES.contains(cardType) && !SUPPORTED_CARD_TYPES.contains(cardSubtype)) {
        return false;
    }
    NSc::TValue card;
    card["not_available"].SetBool(false);
    card["play_inside_app"].SetBool(Ctx.ClientFeatures().SupportsMusicSDKPlayer());
    card["coverUri"].SetString(result["coverUri"]);
    if (!playUri.Empty()) {
        card["playUri"].SetString(playUri);
    }
    card["log_id"].SetString(logId);
    if (!Ctx.HasExpFlag(NAlice::NExperiments::DISABLE_EXPERIMENTAL_FLAG_MUSIC_SEARCH_APP_NEW_CARDS) && Ctx.ClientFeatures().IsSearchApp() && Ctx.ClientFeatures().SupportsDiv2Cards() &&
        (cardType == "track" || cardType == "album" || cardType == "artist" || cardType == "playlist") && !Ctx.GetIsMusicVinsRequest())
    {
        if (MUSIC_REDIRECTS.contains(cardType)) {
            if (cardType == "playlist") {
                card["redirectUri"].SetString(TStringBuilder() << MUSIC_REDIRECTS.at(cardType) << "?name=" << result["ownerLogin"].GetString() << "&id=" << result["kind"].GetString());
            } else {
                card["redirectUri"].SetString(TStringBuilder() << MUSIC_REDIRECTS.at(cardType) << "?id=" << result["id"].GetString());
            }
        }
        Ctx.AddDiv2CardBlock(TStringBuilder() << "music_search_app__" << cardType, std::move(card), /* hideBorders= */false);
        if (Ctx.HasAnyAttention({"suggest_authorization_from_music_play", "suggest_yaplus"})) {
            Ctx.AddTextCardBlock("music_start");
        }
    } else {
        Ctx.AddTextCardBlock("music_start");
        Ctx.AddDivCardBlock(TStringBuilder() << "music__" << cardType, std::move(card));
    }
    return true;
}

void TBaseMusicProvider::AdjustCommandData(NSc::TValue& commandData) {
    Y_UNUSED(commandData);
}

void TBaseMusicProvider::MakeBlocks(const NSc::TValue& result) {
    if (!result.Get("uri").IsString()) {
        return;
    }
    NSc::TValue commandData;
    TStringBuf playUri = result.Get("uri").GetString();
    commandData["uri"].SetString(playUri);
    if (Ctx.MetaClientInfo().IsSmartSpeaker()) {
        Ctx.AddCommand<TMusicSmartSpeakerPlayDirective>(TStringBuf("music_play"), commandData);
    } else {
        Ctx.AddCommand<TMusicAppPlayDirective>(TStringBuf("open_uri"), commandData);
    }

    if (!TryAddDivCard(result, playUri, "music_play")) {
        Ctx.AddSuggest(TStringBuf("music__open_uri"));
    }
}

NHttpFetcher::THandle::TRef TBaseMusicProvider::CreateRequestHandlerBase(TContext& ctx, TSourceRequestFactory source, const TCgiParameters& cgi, TStringBuf method, NHttpFetcher::IMultiRequest::TRef multiRequest) {
    NHttpFetcher::TRequestPtr request = multiRequest ? source.AttachRequest(multiRequest) : source.Request();
    request->SetMethod(method).AddCgiParams(cgi);
    AddHeaders(CreateCommonHeaders(ctx), request.Get());
    return request->Fetch();
}

NHttpFetcher::THandle::TRef TBaseMusicProvider::CreateRequestHandler(TSourceRequestFactory source, const TCgiParameters& cgi, NHttpFetcher::IMultiRequest::TRef multiRequest) const {
    return CreateRequestHandlerBase(Ctx, source, cgi, TStringBuf("GET"), multiRequest);
}

TResultValue TBaseMusicProvider::ParseProviderResponseBase(NHttpFetcher::TResponse::TConstRef resp, NSc::TValue* answer) {
    if (resp->IsError()) {
        LOG(WARNING) << "MUSIC request ERROR: " << resp->GetErrorText() << Endl;
        i32 errCode = resp->Code;
        if (errCode == 400 || errCode == 401) {
            return TError(
                TError::EType::UNAUTHORIZED,
                TStringBuf("music_authorization_problem")
            );
        } else if (errCode == 402) {
            return TError(
                TError::EType::UNAUTHORIZED,
                PAYMENT_REQUIRED
            );
        } else {
            return TError(
                TError::EType::SYSTEM,
                resp->GetErrorText()
            );
        }
    }

    TString data = resp->Data;
    if (data.empty()) {
        return TError(
            TError::EType::SYSTEM,
            TStringBuf("empty_answer")
        );
    }

    if (!NSc::TValue::FromJson(*answer, data) || !answer->Has("result")) {
        return TError(
           TError::EType::SYSTEM,
           TStringBuf("parsing_error")
        );
    }

    return TResultValue();
}

TResultValue TBaseMusicProvider::SendLike(TContext& ctx, const NSc::TValue& musicResult, const TStringBuf userId) {
    TPersonalDataHelper personalDataHelper(ctx);
    TString puid;
    TCgiParameters cgi;
    TStringBuilder parameter, path;
    if (userId.Empty()) {
        if (!personalDataHelper.GetUid(puid)) {
            LOG(ERR) << "MUSIC: Failed to get Passport-UID from TPersonalDataHelper" << Endl;
            TryAddAuthorizationSuggest(ctx);
            return TResultValue();
        }
    } else {
        puid = userId;
    }
    path << TStringBuf("/users/") << puid << PATH_FOR_LIKE;
    parameter << musicResult["id"].ForceString() << TStringBuf(":") << musicResult.TrySelect("album/id").ForceString();
    cgi.InsertUnescaped("track-id", parameter);

    NHttpFetcher::THandle::TRef handler = CreateRequestHandlerBase(ctx, ctx.GetSources().MusicCatalog(path), cgi, TStringBuf("POST"));
    if (!handler) {
        return TError(TError::EType::SYSTEM,TStringBuf("cannot_create_request"));
    }
    NSc::TValue json;
    if (TResultValue reqError = ParseProviderResponseBase(handler->Wait(), &json)) {
        return reqError;
    }
    return ResultSuccess();
}

TResultValue TBaseMusicProvider::ParseProviderResponse(NHttpFetcher::TResponse::TConstRef resp, NSc::TValue* answer) const {
    return ParseProviderResponseBase(resp, answer);
}

[[nodiscard]] TResultValue IMusicProvider::Apply(NSc::TValue* /* out */, NSc::TValue&& /* applyArguments */) {
    return TResultValue();
}

bool ShouldUseWebSearch(const NAlice::TClientFeatures& clientFeatures,
                        const NSc::TValue& slotData,
                        const NSc::TValue& actionData) {
    if (clientFeatures.SupportsMusicPlayer()) {
        const bool result = ShouldUseWebSearchQuasar(slotData, actionData);
        LOG(INFO) << "ShouldUseWebSearchQuasar returned " << result << Endl;
        return result;
    }

    const bool result = ShouldUseWebSearchLegacy(slotData);
    LOG(INFO) << "ShouldUseWebSearchLegacy returned " << result << Endl;
    return result;
}

bool CheckRadioFilters(const NSc::TValue& filters) {
    auto getFilter = [](const NSc::TValue& filterValues) -> TStringBuf {
        const auto& array = filterValues.GetArray();
        if (array.empty()) {
            return {};
        }

        return array[0].GetString();
    };

    auto parseLanguage = [](const TStringBuf str) -> ELanguage {
        if (str == TStringBuf("russian")) {
            return ELanguage::LANG_RUS;
        }

        if (str == TStringBuf("ukrainian")) {
            return ELanguage::LANG_UKR;
        }

        return ELanguage::LANG_UNK;
    };

    const auto language = parseLanguage(getFilter(filters[TStringBuf("language")]));
    const auto genre = getFilter(filters[TStringBuf("genre")]);

    const size_t size = filters.GetDict().size();

    // Localizable genres taken from music's LocalizedGenresMap class in arcadia/alice/music.
    // No need to do anything with the "local-indie" genre, Begemot's tagger does the right thing here.
    if (language == ELanguage::LANG_RUS) {
        if (RUSSIAN_GENRES.contains(genre)) {
            return size == 2;
        }
    } else if (language == ELanguage::LANG_UKR) {
        if (genre == TStringBuf("rock")) {
            return size == 2;
        }
    }

    // If no localizable genres found, fall back to assuming that radio can only handle a single filter
    return size == 1;
}

} // namespace NBASS::NMusic
