#include "get_track_url_handles.h"
#include "multiroom.h"
#include "music_common.h"
#include "shots.h"

#include <alice/hollywood/library/scenarios/music/music_backend_api/api_path/api_path.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_parsers/content_parser.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/content_requests/content_requests.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/download_info_parser.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/signature_token.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/get_track_url/track_quality_selector.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/music_queue/music_queue.h>
#include <alice/hollywood/library/scenarios/music/music_backend_api/result_renders.h>
#include <alice/hollywood/library/scenarios/music/music_request_builder/music_request_builder.h>
#include <alice/hollywood/library/scenarios/music/proto/cache_data.pb.h>
#include <alice/hollywood/library/scenarios/music/proto/music_context.pb.h>
#include <alice/hollywood/library/scenarios/music/show_view_builder/show_view_builder.h>
#include <alice/hollywood/library/scenarios/music/util/music_proxy_request.h>
#include <alice/hollywood/library/scenarios/music/util/util.h>

#include <alice/library/cachalot_cache/cachalot_cache.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/billing/billing.h>
#include <alice/library/network/common.h>

#include <library/cpp/cgiparam/cgiparam.h>
#include <library/cpp/string_utils/url/url.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString CACHE_PUT_ASYNC_NODE_NAME = "MUSIC_SCENARIO_CACHE_PUT_ASYNC";
const TString CONTENT_PROXY_CACHE_SET_REQUEST_ITEM = "content_proxy_cache_set_request";
const TString CONTENT_PROXY_CACHE_GET_RESPONSE_ITEM = "content_proxy_cache_get_response";

constexpr inline std::initializer_list<TStringBuf> CONTENT_RESPONSE_ITEMS = {MUSIC_RESPONSE_ITEM,
                                                                             MUSIC_RADIO_RESPONSE_ITEM,
                                                                             MUSIC_GENERATIVE_RESPONSE_ITEM};

bool CheckFiltrationMode(const TScenarioHandleContext& ctx, const TMusicQueueWrapper& mq) {
    if(!mq.HasCurrentItem()) {
        return true;
    }
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    const TScenarioApplyRequestWrapper request{requestProto, ctx.ServiceCtx};
    const auto filtrationMode = request.FiltrationMode();
    const auto contentWarning = mq.CurrentItem().GetContentWarning();
    if ((filtrationMode == NScenarios::TUserPreferences_EFiltrationMode_Safe &&
         contentWarning != EContentWarning::ChildSafe) ||
        (filtrationMode == NScenarios::TUserPreferences_EFiltrationMode_FamilySearch &&
         contentWarning == EContentWarning::Explicit)) {
        return false;
    }
    return true;
}

TMaybeRawHttpResponse GetHttpResponseBody(const TScenarioHandleContext& ctx) {
    if (ctx.ServiceCtx.HasProtobufItem(MUSIC_CACHED_RESPONSE_BODY_ITEM)) {
        const auto musicCachedResponseBodyItem = ctx.ServiceCtx.GetOnlyProtobufItem<TBuffer>(MUSIC_CACHED_RESPONSE_BODY_ITEM);
        return std::make_pair(MUSIC_CACHED_RESPONSE_BODY_ITEM, TString(musicCachedResponseBodyItem.GetData()));
    }

    return GetFirstOfRawHttpResponses(ctx, CONTENT_RESPONSE_ITEMS);
}

bool NeedNeighboringTracksRequest(const TMusicQueueWrapper& mq) {
    const auto type = mq.ContentId().GetType();
    return type == TContentId_EContentType_Album || type == TContentId_EContentType_Artist ||
        type == TContentId_EContentType_Playlist;
}

} // namespace

namespace NImpl {

bool ShouldPlayBitrate192Kbps(const TScenarioApplyRequestWrapper& request) {
    return request.Interfaces().GetSupportsAudioBitrate192Kbps() && request.ClientInfo().IsMiniSpeaker() &&
           !request.ClientInfo().IsMidiSpeakerYandex();
}

NAppHostHttp::THttpRequest DownloadInfoMp3GetAlicePrepareProxyImpl(const TMusicQueueWrapper& mq,
                                                          const NScenarios::TRequestMeta& meta,
                                                          const TScenarioApplyRequestWrapper& request,
                                                          TRTLogger& logger, const TString& userId,
                                                          const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo) {
    const auto& currentItem = mq.CurrentItem();
    const auto& clientInfo = request.ClientInfo();

    TMaybe<DownloadInfoFlag> flag;
    if (request.HasExpFlag(EXP_HW_MUSIC_USE_DOWNLOAD_INFO_FORMAT_FLAGS)) {
        if (ShouldPlayBitrate192Kbps(request)) {
            flag = DownloadInfoFlag::LQ; // request 192 kbps
        } else {
            flag = DownloadInfoFlag::HQ; // request 320 kbps
        }
    }

    TString path = NApiPath::DownloadInfoMp3GetAlice(currentItem.GetTrackId(), userId, flag);
    return TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "DownloadInfoMp3GetAlice")
        .BuildAndMove();
}

NAppHostHttp::THttpRequest DownloadInfoHlsPrepareProxyImpl(const TMusicQueueWrapper& mq, const NScenarios::TRequestMeta& meta,
                                                  const NAlice::TClientInfo& clientInfo, TRTLogger& logger,
                                                  const TString& userId, TInstant ts, const bool enableCrossDc,
                                                  const TMusicRequestModeInfo& musicRequestModeInfo) {
    const auto& currentItem = mq.CurrentItem();
    auto sign = NAlice::NHollywood::NMusic::CalculateHlsSignatureToken(currentItem.GetTrackId(), ts);
    TString path = NApiPath::DownloadInfoHls(currentItem.GetTrackId(), userId, ts, sign);
    return TMusicRequestBuilder(path, meta, clientInfo, logger, enableCrossDc, musicRequestModeInfo, "DownloadInfoHls")
        .BuildAndMove();
}

bool HasHlsContainer(const TDownloadInfoOptions& downloadOptions) {
    return AnyOf(downloadOptions, [](const auto& item) {
        return item.Container == EAudioContainer::HLS;
    });
}

const TDownloadInfoItem* GetDownloadInfo(TRTLogger& logger,
                                         const TScenarioApplyRequestWrapper& request,
                                         const TDownloadInfoOptions& downloadOptions) {
    THighQualitySelector sel;
    sel.SetAllowedCodecs(EAudioCodec::MP3);
    if (request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_USE_HLS) && HasHlsContainer(downloadOptions)) {
        sel.SetAllowedContainer(EAudioContainer::HLS);
    }
    if (ShouldPlayBitrate192Kbps(request)) {
        LOG_INFO(logger) << "Try choose lower bitrate = 192";
        sel.SetDesiredBitrateInKbps(192);
    }
    return sel(downloadOptions);
}

TMaybe<NAppHostHttp::THttpRequest> UrlRequestPrepareProxyImpl(const TScenarioApplyRequestWrapper& request,
                                                     const TStringBuf response, const NScenarios::TRequestMeta& meta,
                                                     TRTLogger& logger) {
    const auto downloadOptions = ParseDownloadInfo(response, logger);
    const auto urlInfo = GetDownloadInfo(logger, request, downloadOptions);
    Y_ENSURE(urlInfo);
    if (request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_USE_HLS)) {
        if (urlInfo->Container == EAudioContainer::HLS) {
            LOG_INFO(logger) << "Requested HLS container received, skipping download url";
            return {};
        }
        LOG_INFO(logger) << "Requested HLS container, but haven't received it, falling back to mp3";
    }

    auto uri = NNetwork::ParseUri(urlInfo->DownloadInfoUrl);
    const auto query = uri.GetField(NUri::TField::FieldQuery);
    auto cgiParams = TCgiParameters(query);
    if (cgiParams.Has("service")) {
        LOG_WARN(logger) << "Skipping add 'service=alice' query param to the MDS request url";
    } else {
        cgiParams.InsertUnescaped("service", "alice"); // Need this for MDSSUPPORT-1064
    }
    auto finalQuery = cgiParams.Print();
    uri.FldMemSet(NUri::TField::FieldQuery, finalQuery);

    const auto downloadInfoUrl = uri.PrintS();
    const auto path = GetPathAndQuery(downloadInfoUrl, false);
    LOG_INFO(logger) << "Selected track: bitrate = " << urlInfo->BitrateInKbps <<
                     ", codec = " << urlInfo->Codec << ", pathAndQuery = " << path;

    // NOTE1: MDS does not require 'Authorization: OAuth <...>' header
    // NOTE2: DownloadInfoUrl is very short living thing. After a few seconds request to such url
    // returns 401 Authorization Required (don't be surprized by this)
    auto httpRequest = THttpProxyNoRtlogRequestBuilder(path, meta, logger, "DownloadInfoUrl")
        .BuildAndMove();

    // XXX(vitvlkv): Ugly hack to workaround MDSSUPPORT-605 problem. NOTE: Host:port for url is still taken from backend
    // description, but we force Host header to be without the port here
    // NOTE: The root of the problem is that music backend goes to mds and signs url WITHOUT port for us. Then
    // from here we are actually trying to request that url, but WITH the port. So we provide Host header without it
    auto& header = *httpRequest.AddHeaders();
    header.SetName("Host");
    header.SetValue("storage.mds.yandex.net");
    return httpRequest;
}

} // namespace NImpl

void TDownloadInfoPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    auto& logger = ctx.Ctx.Logger();
    auto mCtx = GetMusicContext(ctx.ServiceCtx);
    const bool hasMusicSubscription = mCtx.GetAccountStatus().GetHasMusicSubscription();

    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TScenarioApplyRequestWrapper request(requestProto, ctx.ServiceCtx);
    const auto& applyArgs = request.UnpackArgumentsAndGetRef<TMusicArguments>();
    auto& scState = *mCtx.MutableScenarioState();
    TryInitPlaybackContextBiometryOptions(logger, scState);
    TMusicQueueWrapper mq(logger, *scState.MutableQueue());
    const auto& biometryOpts = mq.GetBiometryOptions();

    if (HaveHttpResponse(ctx, MUSIC_BILLING_RESPONSE_ITEM)) {
        LOG_INFO(logger) << "About to parse billing response...";
        const auto jsonStr = GetRawHttpResponse(ctx, MUSIC_BILLING_RESPONSE_ITEM);
        NAlice::NBilling::TPromoAvailability promoAvailability;
        const auto& billingResponse = NAlice::NBilling::ParseBillingResponse(jsonStr);
        if (const auto error = std::get_if<NAlice::NBilling::TBillingError>(&billingResponse)) {
            LOG_ERROR(logger) << "Billing response parsing error. " << error->Message() << Endl;
        } else {
            promoAvailability = std::get<NAlice::NBilling::TPromoAvailability>(billingResponse);
        }

        if (promoAvailability.IsAvailable ||
            request.HasExpFlag(NAlice::NExperiments::EXPERIMENTAL_FLAG_TEST_MUSIC_SKIP_PLUS_PROMO_CHECK)) {
            LOG_INFO(logger) << "User has no subsription, but promo is available. Will send them a push with "
                                "activation uri";
            auto& promo = *mCtx.MutableAccountStatus()->MutablePromo();
            promo.SetActivatePromoUri(promoAvailability.ActivatePromoUri);

            if (request.HasExpFlag(NAlice::NExperiments::EXP_MUSIC_EXTRA_PROMO_PERIOD) &&
                request.ClientInfo().IsMiniSpeakerYandex() &&
                !promoAvailability.ExtraPeriodExpiresDate.empty()
            ) {
                LOG_INFO(logger) << "User has extra subscription period until " << promoAvailability.ExtraPeriodExpiresDate;
                promo.SetExtraPeriodExpiresDate(promoAvailability.ExtraPeriodExpiresDate);
            }
        } else {
            LOG_INFO(logger) << "User has no subsription, no promo. So they cannot use music";
        }
        AddMusicContext(ctx.ServiceCtx, mCtx);
        return;
    } else if (!hasMusicSubscription) {
        LOG_INFO(logger) << "Have no billing response and user has no subsription. They cannot use music";
        AddMusicContext(ctx.ServiceCtx, mCtx);
        return;
    }

    const auto& playerCommand = applyArgs.GetPlayerCommand();

    if (const auto maybeResponse = GetHttpResponseBody(ctx)) {
        const auto response = maybeResponse->second;
        auto& sensors = ctx.Ctx.GlobalContext().Sensors();
        ParseContent(logger, sensors, response, mq, mCtx, playerCommand != TMusicArguments_EPlayerCommand_Shuffle);

        if (mCtx.GetNeedRadioSkip() || mCtx.GetNeedRadioDislike() || mCtx.GetNeedOnboardingRadioLikeDislike()) {
            mq.ClearQueue();
            LOG_INFO(logger) << "Cleared all items in the MusicQueue (because of radio skip/dislike or onboarding like/dislike) after ParseContent";
        }

        if (mCtx.GetFirstPlay()) {
            mq.CalcContentErrorsAndAttentions(mCtx);
            LOG_INFO(logger) << "QueueSize = " << mq.QueueSize()
                             << ", FilteredOut = " << mq.GetFilteredOut()
                             << ", HaveExplicitContent = " << mq.HaveExplicitContent()
                             << ", HaveNonChildSafe = " << mq.HaveNonChildSafeContent()
                             << ", FiltrationMode = "
                             << NScenarios::TUserPreferences_EFiltrationMode_Name(mq.FiltrationMode());
            if (mCtx.GetContentStatus().GetErrorVer2() != NMusic::NoError) {
                AddMusicContext(ctx.ServiceCtx, mCtx);
                return;
            }
        } else {
            LOG_INFO(logger) << "QueueSize = " << mq.QueueSize();
        }
        mCtx.SetBatchOfTracksRequested(true);

        if (request.HasExpFlag(NExperiments::EXP_HW_MUSIC_TRACK_CACHE) &&
            mq.ContentId().GetType() == TContentId_EContentType_Track &&
            maybeResponse->first == MUSIC_RESPONSE_ITEM)
        {
            if (const ui64 regionId = mCtx.GetAccountStatus().GetMusicSubscriptionRegionId(); regionId != 0) {
                const ui64 hash = mq.GetContentHash(mCtx.GetAccountStatus().GetMusicSubscriptionRegionId());
                auto cachalotGetRequest = NAlice::NAppHostServices::TCachalotCache::MakeSetRequest(
                    ToString(hash),
                    maybeResponse->second,
                    CACHALOT_MUSIC_SCENARIO_STORAGE_TAG
                );
                LOG_INFO(logger) << "Adding cachalot set request with hash " << hash;
                ctx.ServiceCtx.AddProtobufItem(cachalotGetRequest, CONTENT_PROXY_CACHE_SET_REQUEST_ITEM);
                ctx.ServiceCtx.AddBalancingHint(CACHE_PUT_ASYNC_NODE_NAME, hash);
            } else {
                LOG_ERR(logger) << "Got empty music subscription region id";
            }
        }

        if (maybeResponse->first == MUSIC_CACHED_RESPONSE_BODY_ITEM) {
            mCtx.SetCacheHit(true);
        }

    } else {
        Y_ENSURE(!ShouldReturnContentResponse(mCtx, mq), "Need content response");
    }

    if (!CheckFiltrationMode(ctx, mq)) {
        mCtx.MutableContentStatus()->SetErrorVer2(ErrorRestrictedByChildVer2);
        LOG_INFO(logger) << "ErrorRestrictedByChildVer2 was set";
        if (mq.HasCurrentItemOrHistory() &&
            mq.CurrentItem().HasTrackInfo() && mq.CurrentItem().GetTrackInfo().GetAlbumType() == "podcast") {
            mCtx.MutableContentStatus()->SetAttentionVer2(AttentionForbiddenPodcast);
        }
        AddMusicContext(ctx.ServiceCtx, mCtx);
        return;
    }

    if (mq.HasShotsEnabled() && !mq.HasExtraBeforeCurrentItem()) {
        LOG_INFO(logger) << "Add use after track request";
        AddShotsRequest(ctx, mq.GetBiometryUserId(), mq,
                        MakeMusicRequestModeInfo(EAuthMethod::UserId, mCtx.GetAccountStatus().GetUid(), scState));
    }

    const bool enableCrossDc = request.HasExpFlag(NExperiments::EXP_HW_MUSIC_ENABLE_CROSS_DC);

    if (mq.ContentId().GetType() != TContentId_EContentType_Generative &&
        mq.ContentId().GetType() != TContentId_EContentType_FmRadio && ShouldReceiveNewContent(mCtx) &&
        !(playerCommand == TMusicArguments_EPlayerCommand_Dislike &&
          !IsAudioPlayerPlaying(request.BaseRequestProto().GetDeviceState())))
    {
        const auto& userId = mq.GetBiometryUserIdOrFallback(mCtx.GetAccountStatus().GetUid());
        NAppHostHttp::THttpRequest req;
        TStringBuf item;

        const bool supportsHls = request.HasExpFlag(EXP_HW_MUSIC_THIN_CLIENT_USE_HLS)
            && request.Interfaces().GetHasAudioClientHls()
            && !WillPlayInMultiroomSession(request);

        auto musicRequestModeInfo = TMusicRequestModeInfoBuilder()
                                .SetAuthMethod(EAuthMethod::UserId)
                                .SetRequestMode(ToRequestMode(mq.GetBiometryPlaybackMode()))
                                .SetOwnerUserId(mCtx.GetAccountStatus().GetUid())
                                .SetRequesterUserId(userId)
                                .BuildAndMove();

        if (supportsHls) {
            req = NImpl::DownloadInfoHlsPrepareProxyImpl(mq, ctx.RequestMeta, request.ClientInfo(),
                                                         ctx.Ctx.Logger(), userId, TInstant::MilliSeconds(request.ServerTimeMs()),
                                                         enableCrossDc, musicRequestModeInfo);
            item = MUSIC_REQUEST_ITEM;
        } else {
            req = NImpl::DownloadInfoMp3GetAlicePrepareProxyImpl(mq, ctx.RequestMeta,
                                                                 request,
                                                                 ctx.Ctx.Logger(),
                                                                 userId,
                                                                 enableCrossDc,
                                                                 musicRequestModeInfo);
            item = MUSIC_REQUEST_ITEM_MP3_GET_ALICE;
        }
        AddMusicProxyRequest(ctx, req, item);
    }

    // render data needs content info about nearly tracks when we play paged entity (playlist/album/artist)
    // and coverUrl color info
    if (request.Interfaces().GetSupportsShowView() && request.HasExpFlag(EXP_HW_MUSIC_SHOW_VIEW)) {
        if (NeedNeighboringTracksRequest(mq)) {
            using THelper = TNeighboringTracksRequestHelper<ERequestPhase::Before>;

            const auto [leftTrackPosition, rightTrackPosition] = mq.GetNeighboringTracksBound(request);
            const auto [pageIdx, pageSize] = THelper::CalculatePageIdxAndSize(leftTrackPosition, rightTrackPosition);
            const auto customRequestParams = NApiPath::TApiPathRequestParams{
                /* pageIdx = */ pageIdx,
                /* pageSize = */ pageSize,
                /* shuffle = */ mq.GetShuffle(),
                /* shuffleSeed = */ mq.GetShuffleSeed(),
            };

            // prepare request
            const auto biometryData = ProcessBiometryOrFallback(ctx.Ctx.Logger(), request, TStringBuf{applyArgs.GetAccountStatus().GetUid()});
            const auto& requesterUserId = mq.GetBiometryUserIdOrFallback(mCtx.GetAccountStatus().GetUid());
            auto metaProvider = MakeRequestMetaProviderFromPlaybackBiometry(ctx.RequestMeta, biometryOpts);
            const auto httpReq = PrepareContentRequest(request, mq, mCtx, metaProvider,
                                                 ctx.Ctx.Logger(), biometryData,    // TODO(klim-roma): probably use PlaybackMode from scState instead of biometryData
                                                 requesterUserId, customRequestParams);

            // add request
            THelper{ctx}.AddRequest(httpReq);
        }

        if (const auto& coverUriTemplate = mq.CurrentItem().GetCoverUrl(); !coverUriTemplate.Empty()) {
            using THelper = TAvatarColorsRequestHelper<ERequestPhase::Before>;
            THelper{ctx}.AddRequest(ConstructCoverUri(coverUriTemplate));
        }
    }

    AddMusicContext(ctx.ServiceCtx, mCtx);
}

void TUrlRequestPrepareHandle::Do(TScenarioHandleContext& ctx) const {
    const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioApplyRequest>(ctx.ServiceCtx, REQUEST_ITEM);
    TScenarioApplyRequestWrapper request(requestProto, ctx.ServiceCtx);

    const auto response = GetRawHttpResponse(ctx, MUSIC_RESPONSE_ITEM);
    auto req = NImpl::UrlRequestPrepareProxyImpl(request, response, ctx.RequestMeta, ctx.Ctx.Logger());
    if (req) {
        AddMusicProxyRequest(ctx, *req);
    }
}

void TCacheAdapterHandle::Do(TScenarioHandleContext& ctx) const {
    Y_ENSURE(ctx.ServiceCtx.HasProtobufItem(CONTENT_PROXY_CACHE_GET_RESPONSE_ITEM));
    Y_ENSURE(ctx.ServiceCtx.HasProtobufItem(MUSIC_REQUEST_ITEM));
    const auto cacheResponse = ctx.ServiceCtx.GetOnlyProtobufItem<NCachalotProtocol::TResponse>(CONTENT_PROXY_CACHE_GET_RESPONSE_ITEM);

    auto& logger = ctx.Ctx.Logger();
    LOG_INFO(logger) << "Cachalot response status: " << EResponseStatus_Name(cacheResponse.GetStatus());

    if (cacheResponse.GetStatus() == NCachalotProtocol::OK) {
        TBuffer buffer;
        buffer.SetData(std::move(cacheResponse.GetGetResp().GetData()));
        ctx.ServiceCtx.AddProtobufItem(buffer, MUSIC_CACHED_RESPONSE_BODY_ITEM);
    } else {
        const auto httpRequest = ctx.ServiceCtx.GetOnlyProtobufItem<NAppHostHttp::THttpRequest>(MUSIC_REQUEST_ITEM);

        ctx.ServiceCtx.AddProtobufItem(httpRequest, MUSIC_REQUEST_ITEM);

        LOG_INFO(logger) << "Request item readded with request key: " << MUSIC_REQUEST_ITEM;
    }
}

} // namespace NAlice::NHollywood::NMusic
