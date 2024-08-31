#pragma once

#include "answers.h"
#include "common_headers.h"

#include <alice/bass/forms/context/context.h>
#include <alice/bass/forms/vins.h>
#include <alice/bass/libs/fetcher/request.h>
#include <alice/library/util/system_time.h>
#include <library/cpp/neh/http_common.h>
#include <library/cpp/cgiparam/cgiparam.h>
#include <util/string/builder.h>

namespace NBASS::NMusic {

inline constexpr TStringBuf MUSIC_PLAY_FORM_NAME = "personal_assistant.scenarios.music_play";
inline constexpr TStringBuf MUSIC_PLAY_MORE_FORM_NAME = "personal_assistant.scenarios.music_play_more";
inline constexpr TStringBuf MUSIC_PLAY_LESS_FORM_NAME = "personal_assistant.scenarios.music_play_less";
inline constexpr TStringBuf MUSIC_PLAY_ANAPHORA_FORM_NAME = "personal_assistant.scenarios.music_play_anaphora";

inline constexpr TStringBuf QUASAR_MUSIC_PLAY_OBJECT_ACTION_NAME = "quasar.music_play_object";
inline constexpr TStringBuf QUASAR_MUSIC_PLAY_OBJECT_ACTION_STUB_INTENT = QUASAR_MUSIC_PLAY_OBJECT_ACTION_NAME;

inline constexpr TStringBuf CUSTOM_SLOTS = "custom";
inline constexpr TStringBuf MAIN_FILTERS_SLOTS = "main_filters";
inline constexpr TStringBuf OBJECT_SLOTS = "object";
inline constexpr TStringBuf OTHER_FILTERS_SLOTS = "other_filters";
inline constexpr TStringBuf RADIO_SEEDS_SLOTS = "radio_seeds";
inline constexpr TStringBuf RESULT_SLOTS = "result";
inline constexpr TStringBuf SEARCH_SLOTS = "search";
inline constexpr TStringBuf SNIPPET_SLOTS = "snippet";

extern const THashMap<TStringBuf, TVector<TStringBuf>> SLOT_NAMES;

inline constexpr TStringBuf ATTENTION_FILTERS_NOT_APPLIED = "filters_not_applied";
inline constexpr TStringBuf ATTENTION_RESTRICTED_CONTENT_SETTINGS = "restricted_content_settings";
inline constexpr TStringBuf ATTENTION_SUPPORTS_MUSIC_PLAYER = "supports_music_player";
inline constexpr TStringBuf ERROR_MUSIC_NOT_FOUND = "music_not_found";
inline constexpr TStringBuf ERROR_UNAUTHORIZED = "unauthorized";
inline constexpr TStringBuf ERROR_DO_NOT_SEND = "do_not_send_this_error";

inline constexpr TStringBuf SUGGEST_YAPLUS = "suggest_yaplus";
inline constexpr TStringBuf PAYMENT_REQUIRED = "payment-required";

inline constexpr TStringBuf SEARCH_SOURCE_WEB = "web";
inline constexpr TStringBuf SEARCH_SOURCE_MUSIC = "music";

inline constexpr TStringBuf ACTIVATE_MULTIROOM = "activate_multiroom";
inline constexpr TStringBuf WEB_ANSWER = "web_answer";
inline constexpr TStringBuf MULTIROOM_ROOM = "multiroom_room";
inline constexpr TStringBuf MULTIROOM_LOCATION_ROOMS = "multiroom_location_rooms";
inline constexpr TStringBuf MULTIROOM_LOCATION_GROUPS = "multiroom_location_groups";
inline constexpr TStringBuf MULTIROOM_LOCATION_DEVICES = "multiroom_location_devices";
inline constexpr TStringBuf MULTIROOM_LOCATION_SMART_SPEAKER_MODELS = "multiroom_location_smart_speaker_models";
inline constexpr TStringBuf MULTIROOM_LOCATION_EVERYWHERE = "multiroom_location_everywhere";

const TString YAPLUS_URL_SOURCE_PP = "https://plus.yandex.ru/?source=pp_music_web&utm_content=alice_music_card";

namespace NImpl {

// used internally by providers to keep track of which kind of request we're serving
enum class EProtocolStage {
    Legacy, // legacy VINS-to-BASS request
    Run, // Hollywood's run stage
    Apply, // Hollywood's apply stage
};

} // namespace NImpl

inline bool DataGetHasKey(const NSc::TValue& slotData, const TStringBuf key) {
    return slotData[TString::Join(TStringBuf("has_"), key)].GetBool(false);
}

inline void DataSetHasKey(NSc::TValue& slotData, const TStringBuf key, bool value) {
    slotData[TString::Join(TStringBuf("has_"), key)].SetBool(value);
}

struct TMusicContinuationPayload {
    bool NeedApply = false;
    NSc::TValue ApplyArguments;
    NSc::TValue FeaturesData;
};

class IMusicProvider {
public:
    IMusicProvider(TContext& ctx)
        : Ctx(ctx)
    {
    }

    virtual ~IMusicProvider() = default;

    virtual bool InitRequestParams(const NSc::TValue& slotData) = 0;
    virtual void AddSuggest(const NSc::TValue& result) = 0;
    virtual void MakeBlocks(const NSc::TValue& result) = 0;

    virtual NHttpFetcher::THandle::TRef CreateRequestHandler(TSourceRequestFactory source, const TCgiParameters& cgi, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr) const = 0;
    virtual TResultValue ParseProviderResponse(NHttpFetcher::TResponse::TConstRef resp, NSc::TValue* answer) const = 0;
    TResultValue WaitAndParseResponse(NHttpFetcher::THandle::TRef handle, NSc::TValue* answer) const {
        return ParseProviderResponse(handle->Wait(), answer);
    }

    [[nodiscard]] virtual TResultValue Apply(NSc::TValue* out, NSc::TValue&& applyArguments);

protected:
    TContext& Ctx;
};

class TBaseMusicProvider: public IMusicProvider {
public:
    TBaseMusicProvider(TContext& ctx);
    void MakeBlocks(const NSc::TValue& result) override;

    NHttpFetcher::THandle::TRef CreateRequestHandler(TSourceRequestFactory source, const TCgiParameters& cgi, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr) const override;
    TResultValue ParseProviderResponse(NHttpFetcher::TResponse::TConstRef handler, NSc::TValue* answer) const override;
    static NHttpFetcher::THandle::TRef CreateRequestHandlerBase(TContext& Ctx, TSourceRequestFactory source, const TCgiParameters& cgi, TStringBuf method, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr);
    static TResultValue SendLike(TContext& ctx, const NSc::TValue& musicResult, const TStringBuf userId);
    static TResultValue ParseProviderResponseBase(NHttpFetcher::TResponse::TConstRef handler, NSc::TValue* answer);

protected:
    virtual void AdjustCommandData(NSc::TValue& commandData);

    bool TryAddDivCard(const NSc::TValue& result, const TStringBuf playUri, const TStringBuf logId) const;
};

class TYandexMusicProvider: public TBaseMusicProvider {
public:
    TYandexMusicProvider(TContext& ctx);
    bool InitRequestParams(const NSc::TValue& slotData) override;
    void AddSuggest(const NSc::TValue& result) override;

    [[nodiscard]] TResultValue Apply(NSc::TValue* out, NSc::TValue&& applyArguments) override;

protected:
    NHttpFetcher::THandle::TRef CreateSearchHandler(NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr) const;
    void MakeSuggest(TStringBuf section, TStringBuf path, TStringBuf jsonPath, TStringBuf id, const TCgiParameters& cgi);
    void AdjustCommandData(NSc::TValue& commandData) override;

protected:
    TYandexMusicAnswer ServiceAnswer;
    TString SpecialPlaylist;
    TString SpecialAnswerRawInfo;
    TString SearchText;
    bool Autoplay = false;
};

class TYandexRadioProvider: public TBaseMusicProvider {
public:
    TYandexRadioProvider(TContext& ctx);
    bool InitRequestParams(const NSc::TValue& slotData) override;
    void AddSuggest(const NSc::TValue& result) override;

    [[nodiscard]] TResultValue Apply(NSc::TValue* out, NSc::TValue&& applyArguments) override;

protected:
    TString Station;
    bool Autoplay = false;
    TYandexRadioAnswer ServiceAnswer;
};

class TSnippetProvider: public TYandexMusicProvider {
public:
    TSnippetProvider(TContext& ctx);
    bool InitRequestParams(const NSc::TValue& slotData) override;
    void AddSuggest(const NSc::TValue& result) override;

    static NSc::TValue MakeDataFromSnippet(TContext& ctx, bool autoplay, const NSc::TValue& snippet);

    [[nodiscard]] TResultValue Apply(NSc::TValue* out, NSc::TValue&& applyArguments) override;

protected:
    NSc::TValue Snippet;
};

class TQuasarProvider: public TBaseMusicProvider {
public:
    enum class ERequestType {
        Error /* "error" */,

        Filters /* "filters" */,
        General /* "general" */,
        Playlist /* "playlist" */,
        Answer /* "answer" */, // may fall back to text
        Text /* "text" */,
    };

public:
    TQuasarProvider(TContext& ctx);
    bool InitRequestParams(const NSc::TValue& slotData) override;
    bool InitWithActionData(const NSc::TValue& actionData);

    void AddSuggest(const NSc::TValue& result) override;
    void MakeBlocks(const NSc::TValue& result) override;

    [[nodiscard]] TResultValue Apply(NSc::TValue* out, NSc::TValue&& applyArguments) override;

protected:
    ERequestType DoInitRequestParams(const NSc::TValue& slotData);
    ERequestType InitWithSpecialPlaylist(const TString& spName);
    ERequestType InitWithSpecialAnswerInfo(const NSc::TValue& specialAnswerInfo);
    ERequestType InitWithSearchSnippet(const NSc::TValue& slotData);
    void InitWithYandexMusicAnswerResult(const NSc::TValue& answer);
    void InitCommonParams(TStringBuf searchType);

    void FillPostRequest(NSc::TValue* requestJson) const;
    void PrepareCgiParams(TCgiParameters* cgi) const;
    bool TryFallBackToMusicVertical(bool isGeneral) const;

    NHttpFetcher::THandle::TRef CreateMusicRequest(NSc::TValue* requestJson, NHttpFetcher::IMultiRequest::TRef multiRequest = nullptr) const;

    TResultValue FillMusicAnswer(const NSc::TValue& rawAnswer, NSc::TValue* out);
    TResultValue SelectBestAnswer(const NSc::TValue& musicAnswer, const NSc::TValue& webAnswer, const NSc::TValue& attentions, NSc::TValue* bestAnswer) const;

    void MakeSuggest(TStringBuf section, TStringBuf path, TStringBuf jsonPath, TStringBuf id, const TCgiParameters& cgi);
    TResultValue AddMusicAttentions(const NSc::TValue& answer, NSc::TValue* out);
    NSc::TValue ConvertAttentions(const NSc::TValue& musicAttentions) const;

    void TryDefineAdditionalArtistsAndLikesCount(TStringBuf artistId, const TCgiParameters& cgi, NSc::TValue* out);
    void TryDefineAdditionalPlaylistsAndPlaylistId(const NSc::TValue& playlistId, const TCgiParameters& cgi, NSc::TValue* out);
    void TryDefineAdditionalTracks(TStringBuf artistId, TStringBuf trackId, const TCgiParameters& cgi, NSc::TValue* out);
    void TryDefineColorForCoverUri(TStringBuf coverUri, const TCgiParameters& cgi, NSc::TValue* out);

    TString MakeDeeplink(const NAlice::TClientFeatures& clientFeatures, const NSc::TValue& directive);

    bool TryConvertQuasarRequest(const bool multiroomStarted);

protected:
    TQuasarMusicAnswer ServiceAnswer;
    NSc::TValue Directive;
    NSc::TValue RequestBody;
    TString Action;

    double TrackOffset = 0;

    bool MusicQuasarClient = true;

    bool HasSearch = false;
    bool HasObject = false;
    bool HasFilters = false;
    bool HasRadioSeeds = false;
    bool PersonalSearch = false;
    bool PlaylistSearch = false;

    struct TUserRequestInfo {
        TString PlaylistOwnerUid;
        TString CurrentlySpeakingUid;
        TString CurrentlySpeakingName;
        bool HasBiometry = false;
    };

    TUserRequestInfo UserInfo;
    bool NeedChangeOwner = false;

    bool NeedShuffle = false;
    bool NeedRepeat = false;

    bool NeedAliceShots = false;
    bool NeedAttentionForAliceShots = false;

    bool IsMorningShow = false;
    bool IsMeditation = false;

    NSc::TValue MorningShowConfig;
    bool IsChildrenMorningShow = false;

    NSc::TValue AdditionalArtists;
    NSc::TValue AdditionalPlaylists;
    NSc::TValue AdditionalTracks;
};

TResultValue AddAuthorizationSuggest(TContext& ctx);
bool TryAddAuthorizationSuggest(TContext& ctx, const bool shouldSuggestAuthorization = false);
bool ShouldUseWebSearch(const NAlice::TClientFeatures& clientFeatures,
                        const NSc::TValue& slotData,
                        const NSc::TValue& actionData);
bool CheckRadioFilters(const NSc::TValue& filters);
TString MergeTextFromSlots(TContext& ctx, const TString& utterance, const bool alarm);
TString MergeSearchText(TContext& ctx);

} // namespace NBASS::NMusic
