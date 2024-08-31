#include "util.h"

#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_defs.h>
#include <alice/megamind/protos/common/data_source_type.pb.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/library/geo/protos/user_location.pb.h>

#include <util/string/builder.h>

namespace NAlice::NVideoCommon {

namespace {

TMaybe<TString>  GetSupportedVideoCodecs(const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    const auto& interfaces = baseRequestProto.GetInterfaces();
    TString videoCodecs = "";
    if (interfaces.GetVideoCodecAVC()) {
        videoCodecs += "AVC,";
    }

    if (interfaces.GetVideoCodecHEVC()) {
        videoCodecs += "HEVC,";
    }

    if (interfaces.GetVideoCodecVP9()) {
        videoCodecs += "VP9,";
    }

    if (videoCodecs.size() > 0) {
        return videoCodecs.substr(0, videoCodecs.size() - 1);
    }

    return {};
}

TMaybe<TString> GetSupportedAudioCodecs(const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    const auto& interfaces = baseRequestProto.GetInterfaces();
    TString audioCodecs = "";

    if (interfaces.GetAudioCodecAAC()) {
        audioCodecs += "AAC,";
    }

    if (interfaces.GetAudioCodecAC3()) {
        audioCodecs += "AC3,";
    }

    if (interfaces.GetAudioCodecEAC3()) {
        audioCodecs += "EAC3,";
    }

    if (interfaces.GetAudioCodecVORBIS()) {
        audioCodecs += "VORBIS,";
    }

    if (interfaces.GetAudioCodecOPUS()) {
        audioCodecs += "OPUS,";
    }

    if (audioCodecs.size() > 0) {
        return audioCodecs.substr(0, audioCodecs.size() - 1);
    }

    return {};
}

TMaybe<TString> GetCurrentHDCPLevel(const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    const auto& hdcpLevel = baseRequestProto.GetDeviceState().GetScreen().GetHdcpLevel();
    TString сurrentHDCPLevel = "";

    if (hdcpLevel == NAlice::TDeviceState_TScreen_EHdcpLevel_current_HDCP_level_none) {
        сurrentHDCPLevel += "None,";
    }

    if (hdcpLevel == NAlice::TDeviceState_TScreen_EHdcpLevel_current_HDCP_level_1X) {
        сurrentHDCPLevel += "1X,";
    }

    if (hdcpLevel == NAlice::TDeviceState_TScreen_EHdcpLevel_current_HDCP_level_2X) {
        сurrentHDCPLevel += "2X,";
    }

    if (сurrentHDCPLevel.size() > 0) {
        return сurrentHDCPLevel.substr(0, сurrentHDCPLevel.size() - 1);
    }

    return {};
}

TMaybe<TString>  GetSupportedDynamicRange(const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    TString dynamicRange = "";
    const auto& deviceState = baseRequestProto.GetDeviceState();

    for (const auto& item : deviceState.GetScreen().GetDynamicRanges()) {
        if (item == NAlice::TDeviceState_TScreen_EDynamicRange_dynamic_range_SDR) {
            dynamicRange += "SDR,";
        } else if (item == NAlice::TDeviceState_TScreen_EDynamicRange_dynamic_range_HDR10) {
            dynamicRange += "HDR10,";
        } else if (item == NAlice::TDeviceState_TScreen_EDynamicRange_dynamic_range_HDR10Plus) {
            dynamicRange += "HDR10Plus,";
        } else if (item == NAlice::TDeviceState_TScreen_EDynamicRange_dynamic_range_DV) {
            dynamicRange += "DV,";
        } else if (item == NAlice::TDeviceState_TScreen_EDynamicRange_dynamic_range_HLG) {
            dynamicRange += "HLG,";
        }
    }

    if (dynamicRange.size() > 0) {
        return dynamicRange.substr(0, dynamicRange.size() - 1);
    }

    return {};
}

TMaybe<TString>  GetSupportedVideoFormat(const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    TString videoFormat = "";

    const auto& deviceState = baseRequestProto.GetDeviceState();

    for (const auto& item : deviceState.GetScreen().GetSupportedScreenResolutions()) {
        if (item == NAlice::TDeviceState_TScreen_EScreenResolution_video_format_SD) {
            videoFormat += "SD,";
        } else if (item == NAlice::TDeviceState_TScreen_EScreenResolution_video_format_HD) {
            videoFormat += "HD,";
        } else if (item == NAlice::TDeviceState_TScreen_EScreenResolution_video_format_UHD) {
            videoFormat += "UHD,";
        }
    }

    if (videoFormat.size() > 0) {
        return videoFormat.substr(0, videoFormat.size()-1);
    }

    return {};
}

} // namespace

TString MakeEntref(const TString& intoId) {
    return TStringBuilder() << "entnext=" << intoId;
}

void AddIrrelevantResponse(NAlice::NHollywood::TScenarioHandleContext& ctx) {
    NAlice::NHollywood::TRunResponseBuilder responseBuilder;
    responseBuilder.SetIrrelevant();
    responseBuilder.CreateResponseBodyBuilder();
    ctx.ServiceCtx.AddProtobufItem(*std::move(responseBuilder).BuildResponse(), NAlice::NHollywood::RESPONSE_ITEM);
}

void FillAnalyticsInfo(NAlice::NHollywood::TResponseBodyBuilder& bodyBuilder,
                       TStringBuf intent,
                       TStringBuf productScenarioName) {
    auto& analyticsInfoBuilder = bodyBuilder.CreateAnalyticsInfoBuilder();
    analyticsInfoBuilder.SetIntentName(TString{intent});
    analyticsInfoBuilder.SetProductScenarioName(TString{productScenarioName});
}

void AddCodecsByBaseRequest(TProxyRequestBuilder& requestBuilder, const NScenarios::TScenarioBaseRequest& baseProto) {
    if (const auto videoCodecs = GetSupportedVideoCodecs(baseProto)) {
        requestBuilder.AddHeader("X-Device-Video-Codecs", ToString(videoCodecs));
    }

    if (const auto audioCodecs = GetSupportedAudioCodecs(baseProto)) {
        requestBuilder.AddHeader("X-Device-Audio-Codecs", ToString(audioCodecs));
    }

    if (const auto сurrentHDCPLevel = GetCurrentHDCPLevel(baseProto)) {
        requestBuilder.AddHeader("supportsCurrentHDCPLevel", ToString(сurrentHDCPLevel));
    }

    if (const auto dynamicRange = GetSupportedDynamicRange(baseProto)) {
        requestBuilder.AddHeader("X-Device-Dynamic-Ranges", ToString(dynamicRange));
    }

    if (const auto videoFormat = GetSupportedVideoFormat(baseProto)) {
        requestBuilder.AddHeader("X-Device-Video-Formats", ToString(videoFormat));
    }
}

void AddCodecHeadersIntoRequest(TProxyRequestBuilder& requestBuilder, const NHollywood::TScenarioBaseRequestWrapper& scenarioRequest) {
    return AddCodecsByBaseRequest(requestBuilder, scenarioRequest.BaseRequestProto());
}
void AddCodecHeadersIntoRequest(TProxyRequestBuilder& requestBuilder, const NScenarios::TScenarioBaseRequest& baseRequestProto) {
    return AddCodecsByBaseRequest(requestBuilder, baseRequestProto);
}

/*
 * BEWARE using this function! make sure that your scenario has APP_INFO dataSource
 *
 * e.g. https://a.yandex-team.ru/arc/trunk/arcadia/alice/megamind/configs/production/scenarios/mordovia_video_selection.pb.txt?rev=r7862895#L12
 *
 * without app_info cgi param created webview page may have problems with passing UAAS restrictions
 *
 */
TCgiParameters GetDefaultWebviewCgiParams(const NHollywood::TScenarioRunRequestWrapper& request) {
    TCgiParameters cgi;
    cgi.InsertUnescaped(TStringBuf("parent-reqid"), request.RequestId());
    cgi.InsertUnescaped(TStringBuf("uuid"), request.ClientInfo().Uuid);
    cgi.InsertUnescaped(TStringBuf("deviceid"), request.BaseRequestProto().GetDeviceState().GetDeviceId());
    const auto* appInfo = request.GetDataSource(NAlice::EDataSourceType::APP_INFO);
    if (appInfo && appInfo->HasAppInfo()) {
        cgi.InsertUnescaped(TStringBuf("app_info"), appInfo->GetAppInfo().GetValue());
    }

    if (const auto video_codec = GetSupportedVideoCodecs(request.BaseRequestProto())) {
        cgi.InsertUnescaped(TStringBuf("video_codec"),  ToString(video_codec));
    }

    if (const auto audio_codec = GetSupportedAudioCodecs(request.BaseRequestProto())) {
        cgi.InsertUnescaped(TStringBuf("audio_codec"), ToString(audio_codec));
    }

    if (const auto dynamic_range = GetSupportedDynamicRange(request.BaseRequestProto())) {
        cgi.InsertUnescaped(TStringBuf("dynamic_range"), ToString(dynamic_range));
    }

    if (const auto video_format = GetSupportedVideoFormat(request.BaseRequestProto())) {
        cgi.InsertUnescaped(TStringBuf("video_format"), ToString(video_format));
    }

    if (const auto current_hdcp_level = GetCurrentHDCPLevel(request.BaseRequestProto())) {
        cgi.InsertUnescaped(TStringBuf("current_hdcp_level"), ToString(current_hdcp_level));
    }

    if (request.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISREGARD_UAAS)) {
        cgi.InsertUnescaped(TStringBuf("no-tests"), "1");
    }


    TStringBuf testidsFromMegamind = GetTestidsFromMegamindCookies(request.BaseRequestProto().GetOptions().GetMegamindCookies());
    if (!testidsFromMegamind.empty()) {
        cgi.InsertUnescaped(TStringBuf("test-id"), testidsFromMegamind);
    } else if (const auto flagValue = request.ExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_FIX_TESTIDS); flagValue.Defined()) {
        cgi.InsertUnescaped(TStringBuf("test-id"), flagValue.GetRef());
    }

    if (const auto flagValue = request.ExpFlag(NAlice::NVideoCommon::FLAG_MORDOVIA_CGI_STRING); flagValue.Defined()) {
        for (const auto& param : StringSplitter(flagValue.GetRef()).Split('&').SkipEmpty()) {
            TStringBuf stringParam(param);
            cgi.InsertUnescaped(stringParam.NextTok('='), stringParam);
        }
    }

    return cgi;
}

inline constexpr TStringBuf CGI_PARAM_IPREG = TStringBuf("ipreg");

void AddIpregParam(TProxyRequestBuilder& requestBuilder, const NHollywood::TScenarioRunRequestWrapper& request) {
    if (const auto* userLocationPtr = request.GetDataSource(NAlice::EDataSourceType::USER_LOCATION)) {
        const auto& userLocationProto = userLocationPtr->GetUserLocation();
        requestBuilder.AddCgiParam(CGI_PARAM_IPREG, ToString(userLocationProto.GetUserRegion()));
    }
};

void AddIpregParam(TCgiParameters& cgi, const NHollywood::TScenarioRunRequestWrapper& request) {
    if (const auto* userLocationPtr = request.GetDataSource(NAlice::EDataSourceType::USER_LOCATION)) {
        const auto& userLocationProto = userLocationPtr->GetUserLocation();
        cgi.InsertUnescaped(CGI_PARAM_IPREG, ToString(userLocationProto.GetUserRegion()));
    }
};

} // namespace NAlice::NVideoCommon
