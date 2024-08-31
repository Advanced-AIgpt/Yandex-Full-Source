#include "common.h"
#include "continue_render_handle.h"
#include "requests_helper.h"
#include "web_os_helper.h"

#include <alice/hollywood/library/scenarios/music/analytics_info/analytics_info.h>
#include <alice/hollywood/library/scenarios/music/common.h>
#include <alice/hollywood/library/scenarios/music/fast_data.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/content_id.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_id/playlist_id.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/music_sdk_uri_builder/music_sdk_uri_builder.h>
#include <alice/hollywood/library/scenarios/music/music_sdk_handles/nlg_data_builder/nlg_data_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>

#include <alice/bass/libs/client/experimental_flags.h>
#include <alice/library/json/json.h>
#include <alice/library/url_builder/url_builder.h>

#include <util/random/shuffle.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NMusic::NMusicSdk {

namespace {

constexpr TStringBuf YAPLUS_URL_SOURCE_PP = "https://plus.yandex.ru/?source=pp_music_web&utm_content=alice_music_card";
constexpr TStringBuf STATION_PROMO_URL_SOURCE_PP =
    "https://plus.yandex.ru/station-lite?utm_source=pp&utm_medium=dialog_alice&utm_campaign=MSCAMP-24|lite";
constexpr double STATION_PROMO_PROBABILITY = 0.2;

TString ConstructTrackId(const TVector<NJson::TJsonValue>& additionalTracks, const TString& originalTrackId) {
    auto sb = TStringBuilder{} << originalTrackId;
    for (const auto& track : additionalTracks) {
        sb << "," << track["trackId"].GetString();
    }
    return sb;
}

TMaybe<NJson::TJsonValue> ConstructBassState(const TMusicArguments& continueArgs) {
    const bool hasBassScenarioState = !continueArgs.GetBassScenarioState().Empty();
    if (!hasBassScenarioState) {
        return Nothing();
    }
    return JsonFromString(continueArgs.GetBassScenarioState());
}

TMaybe<TContentId> ConstructContentId(const TMusicArguments_TMusicSearchResult& searchResult) {
    return ContentIdFromText(searchResult.GetContentType(), searchResult.GetContentId());
}

TFrame ConstructFrame(const TScenarioInputWrapper& input) {
    const auto frameProto = input.FindSemanticFrame(MUSIC_PLAY_FRAME);
    Y_ENSURE(frameProto);
    return TFrame::FromProto(*frameProto);
}

class TMusicSdkContinueRenderHandleImpl {
public:
    TMusicSdkContinueRenderHandleImpl(TScenarioHandleContext& ctx)
        : Ctx_{ctx}
        , Logger_{Ctx_.Ctx.Logger()}
        , RequestProto_{GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM)}
        , Request_{TScenarioApplyRequestWrapper(RequestProto_, Ctx_.ServiceCtx)}
        , ContinueArgs_{Request_.UnpackArgumentsAndGetRef<TMusicArguments>()}

        , BassState_{ConstructBassState(ContinueArgs_)}
        , ContentId_{ConstructContentId(ContinueArgs_.GetMusicSearchResult())}
        , Frame_{ConstructFrame(Request_.Input())}

        , ArtistBriefInfo_{ctx}
        , SingleTrack_{ctx}
        , ArtistTracks_{ctx}
        , GenreOverview_{ctx}
        , PlaylistSearch_{ctx}
        , SpecialPlaylist_{ctx}
        , PlaylistInfo_{ctx}
        , PredefinedPlaylistInfo_{ctx}
        , AvatarColors_{ctx}

        , Nlg_(TNlgWrapper::Create(ctx.Ctx.Nlg(), Request_, ctx.Rng, ctx.UserLang))
        , Response_{&Nlg_}
        , BodyBuilder_{Response_.CreateResponseBodyBuilder(&Frame_)}
        , NlgDataBuilder_{Logger_, Request_, Frame_}
    {}

    void Do() {
        if (Ctx_.ServiceCtx.CheckFlagInInputContext(PLAYLIST_SETDOWN_RESPONSE_FAILED_APPHOST_FLAG)) {
            // make early response, open Ya.Music site
            const auto response = ConstructFallbackToMusicVerticalResponse<NScenarios::TScenarioContinueResponse>
                (Ctx_.Ctx.Logger(), Request_, Nlg_, Frame_, /* isGeneral = */ true);
            Ctx_.ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
            return;
        }

        // Clarify content id if missing
        ClarifyContentId();

        // Fill some data for NLG builder
        SetPlaylistOwnerLoginForNlg();
        SetPlaylistVerifiedInfoForNlg();

        // Add suggests

        if(!TryAddAuthorizationSuggest() && !TryAddStationPromoAttention(*Ctx_.Ctx.GlobalContext().FastData().GetFastData<TStationPromoFastData>())) {
            TryAddYaPlusSuggest();
        }

        if (GenreOverview_.HasResponse()) {
            // "top artists of this genre" can be found /genre-overview music API
            AddArtistsSuggests(GenreOverview_.FindRawTopArtists());
        } else if (ArtistBriefInfo_.HasResponse()) {
            // "similar artists" or "other albums" can be found at /brief-info music API
            if (ContentId_->GetType() == TContentId_EContentType_Track || ContentId_->GetType() == TContentId_EContentType_Artist) {
                AddArtistsSuggests(ArtistBriefInfo_.FindRawSimilarArtists(/* limit= */ 3));
            } else if (ContentId_->GetType() == TContentId_EContentType_Album) {
                const auto otherAlbums = ArtistBriefInfo_.FindRawOtherAlbums(atoi(ContentId_->GetId().data()));
                AddAlbumsSuggests(otherAlbums);
            }
        } else if (PlaylistSearch_.HasResponse() || SpecialPlaylist_.HasResponse() ||
                   PlaylistInfo_.HasResponse() || PredefinedPlaylistInfo_.HasResponse())
        {
            TMaybe<TContentJsonInfoBuilder> contentInfoBuilder;

            if (PlaylistSearch_.HasResponse()) {
                contentInfoBuilder.ConstructInPlace("playlist", Request_.Interfaces(), Request_.ClientInfo(), *PlaylistSearch_.TryGetResponse());
            } else if (SpecialPlaylist_.HasResponse()) {
                contentInfoBuilder.ConstructInPlace("special_playlist", Request_.Interfaces(), Request_.ClientInfo(), *SpecialPlaylist_.TryGetResponse());
            } else {
                const auto& rawResponse = PlaylistInfo_.HasResponse() ? *PlaylistInfo_.TryGetResponse() : *PredefinedPlaylistInfo_.TryGetResponse();
                contentInfoBuilder.ConstructInPlace("predefined_playlist", Request_.Interfaces(), Request_.ClientInfo(), rawResponse);
            }

            // playlist "answer" slot is not based on bass run response
            if (auto playlistAnswer = contentInfoBuilder->Build()) {
                NlgDataBuilder_.ReplaceAnswerSlot(std::move(*playlistAnswer));
            }
        }

        if (ContentId_->GetType() == TContentId_EContentType_Track) {
            // there might be additional tracks
            const TString& originalTrackId = ContentId_->GetId();
            const auto additionalTracks = ArtistTracks_.FindPreparedOtherTracks(atoi(originalTrackId.data()));
            ContentId_->SetId(ConstructTrackId(additionalTracks, originalTrackId));
            NlgDataBuilder_.CopyToAnswerSlotWithShuffle(Ctx_.Rng, "additionalTracks", additionalTracks);
        }
        else if (ContentId_->GetType() == TContentId_EContentType_Artist ||
                 ContentId_->GetType() == TContentId_EContentType_Album)
        {
            const auto additionalArtists = ArtistBriefInfo_.FindPreparedSimilarArtists();
            NlgDataBuilder_.SetLikesCount(ArtistBriefInfo_.GetLikesCount().GetOrElse(0));
            NlgDataBuilder_.CopyToAnswerSlotWithShuffle(Ctx_.Rng, "additionalArtists", additionalArtists);
        }
        else if (ContentId_->GetType() == TContentId_EContentType_Playlist) {
            const auto playlistId = TPlaylistId::FromString(ContentId_->GetId());
            TVector<NJson::TJsonValue> additionalPlaylists;
            if (PlaylistInfo_.HasResponse()) {
                additionalPlaylists = PlaylistInfo_.FindPreparedSimilarPlaylists(*playlistId);
            } else if (PredefinedPlaylistInfo_.HasResponse()) {
                additionalPlaylists = PredefinedPlaylistInfo_.FindPreparedSimilarPlaylists(*playlistId);
            }
            NlgDataBuilder_.CopyToAnswerSlotWithShuffle(Ctx_.Rng, "additionalPlaylists", additionalPlaylists);
        }

        TryAddSearchSuggest(Request_.ClientInfo(), Request_.Input(), BodyBuilder_);
        AddOnboardingSuggest(BodyBuilder_, Nlg_, NlgDataBuilder_.GetNlgData());

        // Add music sdk uri (but in some cases we may not provide it)
        TMaybe<TString> musicSdkUri = TryConstructMusicSdkUri();
        if (Request_.ClientInfo().IsLegatus()) {
            AddWebOSLaunchAppDirective(Request_, NlgDataBuilder_, BodyBuilder_,
                /* isUserUnauthorizedOrWithoutSubscription */ false, ContentId_, &ContinueArgs_.GetEnvironmentState());
        } else {
            if (musicSdkUri.Defined()) {
                LOG_INFO(Logger_) << "Adding music sdk open uri directive";
                NlgDataBuilder_.AddMusicSdkUri(*ContentId_, *musicSdkUri);
                AddOpenUriDirective(BodyBuilder_, std::move(*musicSdkUri));
            } else {
                LOG_INFO(Logger_) << "Adding vins open uri directive";
                TString uri{NlgDataBuilder_.GetAnswerUri()};
                NlgDataBuilder_.AddMusicSdkUri(*ContentId_, uri, /* logId = */ "music_play");
                AddOpenUriDirective(BodyBuilder_, std::move(uri), /* name = */ "music_app_or_site_play");
            }
        }

        // Add Div2 card (if we can), Text, Voice
        if (AvatarColors_.HasResponse()) {
            NlgDataBuilder_.AddBackgroundGradient(AvatarColors_.GetMainColor(), AvatarColors_.GetSecondColor());
        }
        if (SingleTrack_.HasResponse() && SingleTrack_.LyricsAvailable()) {
            NlgDataBuilder_.AddLyricsSearchUri(SingleTrack_.ArtistName(), SingleTrack_.Title());
        }

        // Searchapp suggests to have an account
        TryAddAuthorizationText();
        if (!TryAddDivCard()) {
            // Just add text
            BodyBuilder_.AddRenderedText(TEMPLATE_MUSIC_PLAY, "render_result", NlgDataBuilder_.GetNlgData());
        }
        BodyBuilder_.AddRenderedVoice(TEMPLATE_MUSIC_PLAY, "render_result", NlgDataBuilder_.GetNlgData());

        // Add analytics info
        AddAnalyticsInfo();

        // Make response
        const auto responseProto = std::move(Response_).BuildResponse();
        Ctx_.ServiceCtx.AddProtobufItem(*responseProto, RESPONSE_ITEM);
    }

private:
    void ClarifyContentId() {
        if (PlaylistSearch_.HasResponse() || SpecialPlaylist_.HasResponse()) {
            NJson::TJsonValue const* rawResponse;
            if (PlaylistSearch_.HasResponse()) {
                rawResponse = PlaylistSearch_.TryGetResponse().Get();
                ContentId_ = ContentIdFromText("playlist", NUsualPlaylist::FindPlaylistId(*rawResponse));
            } else {
                rawResponse = SpecialPlaylist_.TryGetResponse().Get();
                ContentId_ = ContentIdFromText("playlist", *NSpecialPlaylist::FindPlaylistId(*rawResponse));
            }
        }
    }

    void SetPlaylistOwnerLoginForNlg() {
        TMaybe<TString> ownerLogin;
        if (PlaylistSearch_.HasResponse()) {
            ownerLogin = NUsualPlaylist::FindOwnerLogin(*PlaylistSearch_.TryGetResponse());
        } else if (SpecialPlaylist_.HasResponse()) {
            ownerLogin = NSpecialPlaylist::FindOwnerLogin(*SpecialPlaylist_.TryGetResponse());
        } else if (PredefinedPlaylistInfo_.HasResponse()) {
            ownerLogin = NPredefinedPlaylist::FindOwnerLogin(*PredefinedPlaylistInfo_.TryGetResponse());
        }

        if (ownerLogin.Defined()) {
            NlgDataBuilder_.SetPlaylistOwnerLogin(*ownerLogin);
        }
    }

    void SetPlaylistVerifiedInfoForNlg() {
        TMaybe<TString> ownerUid;
        TMaybe<bool> ownerIsVerified;
        if (PlaylistSearch_.HasResponse()) {
            ownerUid = NUsualPlaylist::FindOwnerUid(*PlaylistSearch_.TryGetResponse());
            ownerIsVerified = NUsualPlaylist::FindOwnerIsVerified(*PlaylistSearch_.TryGetResponse());
        } else if (SpecialPlaylist_.HasResponse()) {
            // XXX(sparkle): somehow all special playlists are verified...
            //ownerUid = NSpecialPlaylist::FindOwnerUid(*SpecialPlaylist_.TryGetResponse());
            //ownerIsVerified = NSpecialPlaylist::FindOwnerIsVerified(*SpecialPlaylist_.TryGetResponse());
        } else if (PredefinedPlaylistInfo_.HasResponse()) {
            ownerUid = NPredefinedPlaylist::FindOwnerUid(*PredefinedPlaylistInfo_.TryGetResponse());
            ownerIsVerified = NPredefinedPlaylist::FindOwnerIsVerified(*PredefinedPlaylistInfo_.TryGetResponse());
        }

        if (ownerUid.Defined() && ownerIsVerified.Defined()) {
            if ((*ownerIsVerified) == false && (*ownerUid) != ContinueArgs_.GetAccountStatus().GetUid()) {
                NlgDataBuilder_.AddAttention("unverified_playlist");
            }
        }
    }

    void AddArtistsSuggests(const TVector<const NJson::TJsonValue*>& artistObjs) {
        for (const auto& artistObj : artistObjs) {
            TMaybe<NJson::TJsonValue> nlgDataJson = TContentJsonInfoBuilder("artist", Request_.Interfaces(), Request_.ClientInfo(), *artistObj).Build();
            if (!nlgDataJson.Defined()) {
                continue;
            }

            TNlgData nlgData(Logger_, Request_);
            nlgData.Context["music__suggest_artist"]["data"] = std::move(*nlgDataJson);

            const auto caption = Nlg_.RenderPhrase(
                /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
                /* phraseName = */ "render_suggest_caption__music__suggest_artist",
                /* nlgData = */ nlgData
            ).Text;

            const auto utterance = Nlg_.RenderPhrase(
                /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
                /* phraseName = */ "render_suggest_utterance__music__suggest_artist",
                /* nlgData = */ nlgData
            ).Text;

            BodyBuilder_.AddTypeTextSuggest(/* text= */ caption, /* typeText = */ utterance, /* name = */ "render_buttons_type");
        }
    }

    void AddAlbumsSuggests(const TVector<const NJson::TJsonValue*>& albumObjs) {
        for (const auto* albumObj : albumObjs) {
            TMaybe<NJson::TJsonValue> nlgDataJson = TContentJsonInfoBuilder("album", Request_.Interfaces(), Request_.ClientInfo(), *albumObj).Build();
            if (!nlgDataJson.Defined()) {
                continue;
            }

            TNlgData nlgData(Logger_, Request_);
            nlgData.Context["music__suggest_album"]["data"] = std::move(*nlgDataJson);

            const auto caption = Nlg_.RenderPhrase(
                /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
                /* phraseName = */ "render_suggest_caption__music__suggest_album",
                /* nlgData = */ nlgData
            ).Text;

            const auto utterance = Nlg_.RenderPhrase(
                /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
                /* phraseName = */ "render_suggest_utterance__music__suggest_album",
                /* nlgData = */ nlgData
            ).Text;

            BodyBuilder_.AddTypeTextSuggest(/* text= */ caption, /* typeText = */ utterance, /* name = */ "render_buttons_type");
        }
    }

    void AddSuggestWithUri(const TStringBuf uri, const TStringBuf phraseName) {
        NScenarios::TDirective directive;
        auto& openUriDirective = *directive.MutableOpenUriDirective();
        openUriDirective.SetName("render_buttons_open_uri");
        openUriDirective.SetUri(uri.data(), uri.size());

        TResponseBodyBuilder::TSuggest suggest;
        suggest.Directives.push_back(std::move(directive));
        suggest.ButtonForText = Nlg_.RenderPhrase(TEMPLATE_MUSIC_PLAY, phraseName,
                                                  NlgDataBuilder_.GetNlgData()).Text;

        BodyBuilder_.AddRenderedSuggest(std::move(suggest));
    }

    bool TryAddAuthorizationSuggest() {
        // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_provider.cpp?rev=r8764159#L234
        if (IsUserAuthorized(Ctx_.RequestMeta)) {
            return false;
        }
        NlgDataBuilder_.AddAttention("unauthorized");
        NlgDataBuilder_.AddAttention("suggest_authorization_from_music_play");

        const auto authUri = GenerateAuthorizationUri(Request_.Interfaces().GetCanOpenYandexAuth());
        if (!authUri.Empty()) {
            AddSuggestWithUri(authUri, "render_suggest_caption__authorize");
        }
        return true;
    }

    bool TryAddYaPlusSuggest() {
        // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8764214#L1156
        //           https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/quasar_provider.cpp?rev=r8764223#L1210
        const bool userHasSubscription = ContinueArgs_.GetAccountStatus().GetHasMusicSubscription();
        if (userHasSubscription) {
            // the user doesn't need Ya.Plus
            return false;
        }
        NlgDataBuilder_.AddAttention("suggest_yaplus");
        AddSuggestWithUri(YAPLUS_URL_SOURCE_PP, "render_suggest_caption__yaplus");
        return true;
    }

    bool TryAddStationPromoAttention(const TStationPromoFastData& stationPromoFastData) {
        // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/hollywood/library/scenarios/music/music_backend_api/music_common.cpp?rev=r8762357#L47
        const bool canTryAddAttention = Request_.ClientInfo().IsSearchApp() &&
                                        !ContinueArgs_.GetHasSmartDevices();
        if (!canTryAddAttention) {
            return false;
        }

        // check probability
        double data = STATION_PROMO_PROBABILITY;
        TMaybe<TString> experimentValue = Request_.GetValueFromExpPrefix(NBASS::EXPERIMENTAL_FLAG_STATION_PROMO);
        if (experimentValue.Defined()) {
            TryFromString<double>(experimentValue.GetRef(), data);
        }

        if (ContinueArgs_.GetAccountStatus().GetHasPlus() && Ctx_.Rng.RandomDouble(0, 1) >= data) {
            return false;
        } else if (!ContinueArgs_.GetAccountStatus().GetHasPlus()) {
            ui64 puid;
            if (!TryFromString(ContinueArgs_.GetAccountStatus().GetUid(), puid) || !stationPromoFastData.HasNoPlusPuid(puid)) {
                return false;
            }
        }

        NlgDataBuilder_.AddAttention("station_promo");
        if (ContinueArgs_.GetAccountStatus().GetHasPlus()) {
            NlgDataBuilder_.AddAttention("has_plus");
        }
        AddSuggestWithUri(STATION_PROMO_URL_SOURCE_PP, "render_suggest_caption__station_promo");
        return true;
    }

    TMaybe<TString> TryConstructMusicSdkUri() {
        Y_ENSURE(ContentId_);
        TMusicSdkUriBuilder musicSdkUriBuilder{Request_.ClientInfo().Name, ContentTypeToText(ContentId_->GetType())};

        if (ContentId_->GetType() == TContentId_EContentType_Track) {
            musicSdkUriBuilder.SetTrackId(ContentId_->GetId());
        } else if (ContentId_->GetType() == TContentId_EContentType_Album) {
            musicSdkUriBuilder.SetAlbumId(ContentId_->GetId());
        } else if (ContentId_->GetType() == TContentId_EContentType_Artist) {
            musicSdkUriBuilder.SetArtistId(ContentId_->GetId());
        } else if (ContentId_->GetType() == TContentId_EContentType_Playlist) {
            auto playlistId = TPlaylistId::FromString(ContentId_->GetId());
            if (playlistId->Owner == PLAYLIST_ORIGIN_OWNER_UID) {
                NlgDataBuilder_.AddAttention("alice_shots_intro");
                return Nothing();
            }
            musicSdkUriBuilder.SetPlaylistOwnerAndKind(playlistId->Owner, playlistId->Kind);
        }

        const auto& fairyTaleArgs = ContinueArgs_.GetFairyTaleArguments();
        const bool isGeneralFairyTaleRequest = fairyTaleArgs.GetIsFairyTaleSubscenario() &&
                                               !fairyTaleArgs.GetIsOndemand();
        const bool needShuffle = NeedShuffle(Frame_) || isGeneralFairyTaleRequest;

        return musicSdkUriBuilder
            .SetShuffle(needShuffle)
            .SetRepeatMode(GetRepeatMode(Frame_))
            .Build();
    }

    void TryAddAuthorizationText() {
        if (NlgDataBuilder_.HasAttention("suggest_authorization_from_music_play") ||
            NlgDataBuilder_.HasAttention("suggest_yaplus") ||
            NlgDataBuilder_.HasAttention("station_promo"))
        {
            BodyBuilder_.AddRenderedTextWithButtons(
                /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
                /* phraseName = */ "music_start",
                /* buttons = */ {},
                /* nlgData = */ NlgDataBuilder_.GetNlgData()
            );
        }
    }

    bool TryAddDivCard() {
        // old code: https://a.yandex-team.ru/arc/trunk/arcadia/alice/bass/forms/music/base_provider.cpp?rev=r8709318#L315

        if (!CanRenderDiv2Cards(Request_)) {
            return false;
        }

        BodyBuilder_.AddRenderedDiv2Card(
            /* nlgTemplateName = */ TEMPLATE_MUSIC_PLAY,
            /* phraseName = */ TString::Join("music_search_app__", ContentTypeToText(ContentId_->GetType())),
            /* nlgData = */ NlgDataBuilder_.GetNlgData()
        );
        return true;
    }

    void AddAnalyticsInfo() {
        auto& analyticsInfoBuilder = BodyBuilder_.CreateAnalyticsInfoBuilder();
        FillAnalyticsInfoFromContinueArgs(analyticsInfoBuilder, ContinueArgs_);
        if (BassState_.Defined()) {
            if (const auto& webAnswer = (*BassState_)["apply_arguments"]["web_answer"]; webAnswer.IsMap()) {
                FillAnalyticsInfoFromWebAnswer(analyticsInfoBuilder, Request_, webAnswer);
            }
        }
        if (ContentId_->GetType() == TContentId_EContentType_Playlist) {
            // playlist was redirected from radio slots
            if (BassState_.Empty()) {
                analyticsInfoBuilder.AddSelectedSourceEvent(AnalyticsInfoInstant(Request_), "music");
            }

            // playlists were searched in Hollywood, not in Bass
            analyticsInfoBuilder.AddAnalyticsInfoMusicEvent(
                /* instant= */ AnalyticsInfoInstant(Request_),
                /* answerType= */ NScenarios::TAnalyticsInfo_TEvent_TMusicEvent_EAnswerType_Playlist,
                /* id= */ ContentId_->GetId(),
                /* uri= */ TString{NlgDataBuilder_.GetAnswerUri()}
            );
        }
    }

private:
    // base classes
    TScenarioHandleContext& Ctx_;
    TRTLogger& Logger_;
    const NScenarios::TScenarioApplyRequest RequestProto_;
    const TScenarioApplyRequestWrapper Request_;
    const TMusicArguments& ContinueArgs_;

    // bass state and other
    const TMaybe<NJson::TJsonValue> BassState_;
    TMaybe<TContentId> ContentId_;
    const TFrame Frame_;

    // http responses
    const TArtistBriefInfoRequestHelper<ERequestPhase::After> ArtistBriefInfo_;
    const TSingleTrackRequestHelper<ERequestPhase::After> SingleTrack_;
    const TArtistTracksRequestHelper<ERequestPhase::After> ArtistTracks_;
    const TGenreOverviewRequestHelper<ERequestPhase::After> GenreOverview_;
    const TPlaylistSearchRequestHelper<ERequestPhase::After> PlaylistSearch_;
    const TSpecialPlaylistRequestHelper<ERequestPhase::After> SpecialPlaylist_;
    const TPlaylistInfoRequestHelper<ERequestPhase::After> PlaylistInfo_;
    const TPredefinedPlaylistInfoRequestHelper<ERequestPhase::After> PredefinedPlaylistInfo_;
    const TAvatarColorsRequestHelper<ERequestPhase::After> AvatarColors_;

    // response builders
    TNlgWrapper Nlg_;
    TContinueResponseBuilder Response_;
    TResponseBodyBuilder& BodyBuilder_;
    TNlgDataBuilder NlgDataBuilder_;
};

} // namespace

void TMusicSdkContinueRenderHandle::Do(TScenarioHandleContext& ctx) const {
    TMusicSdkContinueRenderHandleImpl{ctx}.Do();
}

} // namespace NAlice::NHollywood::NMusic::NMusicSdk
