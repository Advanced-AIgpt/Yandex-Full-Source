#include "music_request_builder.h"

#include <alice/library/client/client_info.h>
#include <alice/library/experiments/flags.h>
#include <alice/library/network/headers.h>

#include <library/cpp/svnversion/svnversion.h>

namespace NAlice::NHollywood::NMusic {

namespace {

const TString HEADER_X_YANDEX_MUSIC_CLIENT_VALUE = TString("AliceMusicThinClient/") + GetArcadiaLastChange();

TString ConstructClientIdForMusicBack(const TClientInfo& clientInfo) {
    TStringBuilder clientId;
    clientId << "os=" << clientInfo.OSName
             << "; os_version=" << clientInfo.OSVersion
             << "; manufacturer=" << clientInfo.Manufacturer
             << "; model=" << clientInfo.DeviceModel
             << "; clid=0"
             << "; device_id=" << clientInfo.DeviceId
             << "; uuid=" << clientInfo.Uuid;
    return clientId;
}

} // namespace

const TString& TGuestRequestMetaProvider::GetRequestId() const {
    return Meta_.GetRequestId();
}

const TString& TGuestRequestMetaProvider::GetClientIP() const {
    return Meta_.GetClientIP();
}

const TString& TGuestRequestMetaProvider::GetOAuthToken() const {
    return GuestOAuthToken_;
}

const TString& TGuestRequestMetaProvider::GetUserTicket() const {
    return Meta_.GetUserTicket();
}

TMusicRequestBuilder::TMusicRequestBuilder(const TStringBuf path, const TMusicRequestMeta& requestMeta, TRTLogger& logger,
                                           const TMusicRequestModeInfo& musicRequestModeInfo,
                                           const TString name)
    : THttpProxyNoRtlogRequestBuilder(path, requestMeta.GetRequestMeta(), logger, name, /* randomizeRequestId = */ true)
{
    Init(TClientInfo{requestMeta.GetClientInfo()}, requestMeta.GetEnableCrossDc(), musicRequestModeInfo, logger);
}

TMusicRequestBuilder::TMusicRequestBuilder(const TStringBuf path, const NScenarios::TRequestMeta& meta,
                                           const TClientInfo& clientInfo, TRTLogger& logger,
                                           const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo,
                                           const TString name)
: THttpProxyNoRtlogRequestBuilder(path, meta, logger, name, /* randomizeRequestId = */ true)
{
    Init(clientInfo, enableCrossDc, musicRequestModeInfo, logger);
}

TMusicRequestBuilder::TMusicRequestBuilder(const TStringBuf path, TAtomicSharedPtr<IRequestMetaProvider> metaProvider,
                                           const TClientInfo& clientInfo, TRTLogger& logger,
                                           const bool enableCrossDc, const TMusicRequestModeInfo& musicRequestModeInfo,
                                           const TString name)
: THttpProxyNoRtlogRequestBuilder(path, std::move(metaProvider), logger, name, /* randomizeRequestId = */ true)
{
    Init(clientInfo, enableCrossDc, musicRequestModeInfo, logger);
}

void TMusicRequestBuilder::Init(const TClientInfo& clientInfo, const bool enableCrossDc,
                                const TMusicRequestModeInfo& musicRequestModeInfo, TRTLogger& logger) {
    LOG_INFO(logger) << "Preparing request to music backend"
                     << ": AuthMethod = " << ToString(musicRequestModeInfo.AuthMethod)
                     << ", RequestMode = " << ToString(musicRequestModeInfo.RequestMode)
                     << ", OwnerUserId = " << musicRequestModeInfo.OwnerUserId
                     << ", RequesterUserId = " << musicRequestModeInfo.RequesterUserId;

    AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_MUSIC_CLIENT, HEADER_X_YANDEX_MUSIC_CLIENT_VALUE);

    AddHeader(NAlice::NNetwork::HEADER_X_YANDEX_MUSIC_DEVICE, ConstructClientIdForMusicBack(clientInfo));

    if (!enableCrossDc) {
        AddBalancingHint();
    }
}

} // namespace NAlice::NHollywood::NMusic
