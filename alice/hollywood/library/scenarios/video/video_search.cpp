#include "search_metrics.h"
#include "video_utils.h"
#include "video_search.h"
#include <alice/library/geo/protos/user_location.pb.h>
#include <alice/library/search_result_parser/video/parsers.h>
#include <alice/library/util/search_convert.h>
#include <alice/library/video_common/defs.h>
#include <alice/library/video_common/mordovia_webview_helpers.h>
#include <alice/protos/data/search_result/search_result.pb.h>
#include <alice/protos/data/video/video_scenes.pb.h>
#include <util/system/hostname.h>


namespace NAlice::NHollywood::NVideo {

    const NJson::TJsonValue& GetLastSection(const NAppHost::IServiceContext& ctx, TStringBuf type) {
        NAppHost::TContextItemRefArray responses = ctx.GetItemRefs(type);

        static_assert(
            std::is_same<decltype(responses.back()), const NJson::TJsonValue&>::value,
            "NAppHost::TContextItemRefArray::front() must return a const reference.");
        return responses.empty() ? NJson::TJsonValue::UNDEFINED : responses.back();
    }

    TMaybe<TString> GetSupportedVideoCodecs(const TScenarioRunRequestWrapper& request) {
        TString videoCodecs = "";
        if (request.Interfaces().GetVideoCodecAVC()) {
            videoCodecs += "AVC,";
        }

        if (request.Interfaces().GetVideoCodecHEVC()) {
            videoCodecs += "HEVC,";
        }

        if (request.Interfaces().GetVideoCodecVP9()) {
            videoCodecs += "VP9,";
        }

        if (videoCodecs.size() > 0) {
            return videoCodecs.substr(0, videoCodecs.size() - 1);
        }

        return {};
    }

    TMaybe<TString> GetSupportedAudioCodecs(const TScenarioRunRequestWrapper& request) {
        TString audioCodecs = "";

        if (request.Interfaces().GetAudioCodecAAC()) {
            audioCodecs += "AAC,";
        }

        if (request.Interfaces().GetAudioCodecAC3()) {
            audioCodecs += "AC3,";
        }

        if (request.Interfaces().GetAudioCodecEAC3()) {
            audioCodecs += "EAC3,";
        }

        if (request.Interfaces().GetAudioCodecVORBIS()) {
            audioCodecs += "VORBIS,";
        }

        if (request.Interfaces().GetAudioCodecOPUS()) {
            audioCodecs += "OPUS,";
        }

        if (audioCodecs.size() > 0) {
            return audioCodecs.substr(0, audioCodecs.size() - 1);
        }

        return {};
    }

    TMaybe<TString> GetCurrentHDCPLevel(const TScenarioRunRequestWrapper& request) {
        TString сurrentHDCPLevel = "";

        if (request.Interfaces().GetCurrentHDCPLevelNone()) {
            сurrentHDCPLevel += "None,";
        }

        if (request.Interfaces().GetCurrentHDCPLevel1X()) {
            сurrentHDCPLevel += "1X,";
        }

        if (request.Interfaces().GetCurrentHDCPLevel2X()) {
            сurrentHDCPLevel += "2X,";
        }

        if (сurrentHDCPLevel.size() > 0) {
            return сurrentHDCPLevel.substr(0, сurrentHDCPLevel.size() - 1);
        }

        return {};
    }

    TMaybe<TString> GetSupportedDynamicRange(const TScenarioRunRequestWrapper& request) {
        TString dynamicRange = "";

        if (request.Interfaces().GetDynamicRangeSDR()) {
            dynamicRange += "SDR,";
        } else if (request.Interfaces().GetDynamicRangeHDR10()) {
            dynamicRange += "HDR10,";
        } else if (request.Interfaces().GetDynamicRangeHDR10Plus()) {
            dynamicRange += "HDR10Plus,";
        } else if (request.Interfaces().GetDynamicRangeDV()) {
            dynamicRange += "DV,";
        } else if (request.Interfaces().GetDynamicRangeHLG()) {
            dynamicRange += "HLG,";
        }

        if (dynamicRange.size() > 0) {
            return dynamicRange.substr(0, dynamicRange.size() - 1);
        }

        return {};
    }

    TMaybe<TString> GetSupportedVideoFormat(const TScenarioRunRequestWrapper& request) {
        TString videoFormat = "";

        if (request.Interfaces().GetVideoFormatSD()) {
            videoFormat += "SD,";
        } else if (request.Interfaces().GetVideoFormatHD()) {
            videoFormat += "HD,";
        } else if (request.Interfaces().GetVideoFormatUHD()) {
            videoFormat += "UHD,";
        }

        if (videoFormat.size() > 0) {
            return videoFormat.substr(0, videoFormat.size() - 1);
        }

        return {};
    }

    void AddCodecHeadersIntoRequest(NJson::TJsonValue& request, const TScenarioRunRequestWrapper& requestWrapper) {
        auto addHeader = [&request](const TString& key, const TString& value) {
            NJson::TJsonValue header;
            header.SetValue(NJson::JSON_ARRAY);
            header.AppendValue(key);
            header.AppendValue(value);
            request["headers"].AppendValue(header);
        };
        if (const auto videoCodecs = GetSupportedVideoCodecs(requestWrapper)) {
            addHeader("X-Device-Video-Codecs", ToString(videoCodecs));
        }

        if (const auto audioCodecs = GetSupportedAudioCodecs(requestWrapper)) {
            addHeader("X-Device-Audio-Codecs", ToString(audioCodecs));
        }

        if (const auto сurrentHDCPLevel = GetCurrentHDCPLevel(requestWrapper)) {
            addHeader("supportsCurrentHDCPLevel", ToString(сurrentHDCPLevel));
        }

        if (const auto dynamicRange = GetSupportedDynamicRange(requestWrapper)) {
            addHeader("X-Device-Dynamic-Ranges", ToString(dynamicRange));
        }

        if (const auto videoFormat = GetSupportedVideoFormat(requestWrapper)) {
            addHeader("X-Device-Video-Formats", ToString(videoFormat));
        }
    }

    void AddDeviceParamsToVideoUrl(TCgiParameters& params, const TScenarioRunRequestWrapper& request) {
        auto addDeviceParam = [&params, &request](const TStringBuf paramName, TMaybe<TString> (*paramGetter)(const TScenarioRunRequestWrapper&)) {
            if (const auto paramValue = paramGetter(request)) {
                params.InsertUnescaped(paramName, *paramValue);
            }
        };
        addDeviceParam(TStringBuf("audio_codec"), &GetSupportedAudioCodecs);
        addDeviceParam(TStringBuf("video_codec"), &GetSupportedVideoCodecs);
        addDeviceParam(TStringBuf("dynamic_range"), &GetSupportedDynamicRange);
        addDeviceParam(TStringBuf("video_format"), &GetSupportedVideoFormat);
        addDeviceParam(TStringBuf("current_hdcp_level"), &GetCurrentHDCPLevel);
    }

    void AddTestidsToCgiParams(const TScenarioRunRequestWrapper& request, TCgiParameters& cgi) {
        if (request.HasExpFlag(NVideoCommon::FLAG_VIDEO_DISREGARD_UAAS)) {
            cgi.InsertUnescaped(TStringBuf("no-tests"), "1");
        }
        TStringBuf testidsFromMegamind = NVideoCommon::GetTestidsFromMegamindCookies(request.BaseRequestProto().GetOptions().GetMegamindCookies());
        if (!testidsFromMegamind.empty()) {
            cgi.InsertUnescaped(TStringBuf("test-id"), testidsFromMegamind);
        } else if (const auto flagValue = request.ExpFlag(NVideoCommon::FLAG_VIDEO_FIX_TESTIDS); flagValue.Defined()) {
            cgi.InsertUnescaped(TStringBuf("test-id"), flagValue.GetRef());
        }
    }

    void GetRequiredVideoCgiParams(const TScenarioRunRequestWrapper& requestWrapper, TCgiParameters& cgi) {
        cgi.InsertUnescaped(TStringBuf("parent-reqid"), requestWrapper.RequestId());
        cgi.InsertUnescaped(TStringBuf("uuid"), NAlice::ConvertUuidForSearch(requestWrapper.ClientInfo().Uuid));
        cgi.InsertUnescaped(TStringBuf("deviceid"), requestWrapper.ClientInfo().DeviceId);
        // cgi.InsertUnescaped(TStringBuf("app_info"), );
        AddDeviceParamsToVideoUrl(cgi, requestWrapper);
        AddTestidsToCgiParams(requestWrapper, cgi);
    }

    void SetupVideoSearchRequest(TScenarioHandleContext& ctx, const TScenarioRunRequestWrapper& requestWrapper, const TVideoSearchCallArgs& searchCallArgs) {
        auto& logger = ctx.Ctx.Logger();
        TCgiParameters cgi;

        auto addRearr = [&cgi](const TString& rearr) {
            cgi.InsertUnescaped(TStringBuf("rearr"), rearr);
        };

        if (const auto* userLocationPtr = requestWrapper.GetDataSource(NAlice::EDataSourceType::USER_LOCATION)) {
            const auto& userLocationProto = userLocationPtr->GetUserLocation();
            cgi.InsertUnescaped("ipreg", ToString(userLocationProto.GetUserRegion()));
        }
        GetRequiredVideoCgiParams(requestWrapper, cgi);
        // TODO make normal client
        TString client = "tvandroid";
        cgi.InsertUnescaped(TStringBuf("client"), client);
        if (client == "quasar") {
            cgi.InsertUnescaped(TStringBuf("relev"), TStringBuf("station_request=1"));
            addRearr("scheme_Local/VideoSnippets/ReplaceVhDups=0");
        } else if (client == "tvandroid" || client == "tvmodule2") {
            addRearr("scheme_Local/FilterBannedVideo/SvodForAll=0");
            addRearr("scheme_Local/FilterBannedVideo/SvodForYaPlusUsers=1");
            addRearr("scheme_Local/VideoExtraItems/FilterUnusedObjectsForDevices=1");
            addRearr("scheme_Local/VideoExtraItems/EntityDataForDevices=1");
            addRearr("scheme_Local/VideoPlayers/AddDevice=1");
            cgi.InsertUnescaped(TStringBuf("unban_hosts"), TStringBuf("1"));
            cgi.InsertUnescaped(TStringBuf("gta"), TStringBuf("onto_id"));
            cgi.InsertUnescaped(TStringBuf("gta"), TStringBuf("vh_uuid"));
            cgi.InsertUnescaped(TStringBuf("ipreg"), TStringBuf("213"));
        }
        addRearr("forced_player_device=" + client);

        // prices in 3rd-party resources
        if (requestWrapper.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_FILM_OFFERS_PRICES_ALL)) {
            cgi.InsertEscaped(TStringBuf("rearr"), ToString("entsearch_experiment=VideoBaseInfoFullVariants"));
        } else if (requestWrapper.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_FILM_OFFERS_PRICES_MIN)) {
            cgi.InsertEscaped(TStringBuf("rearr"), ToString("entsearch_experiment=VideoBaseInfoOnlyPrice"));
        }

        cgi.InsertUnescaped(TStringBuf("exp_flags"), TStringBuf("video_oo_page_size=40"));
        cgi.InsertUnescaped(TStringBuf("no-tests"), TStringBuf("da"));
        cgi.InsertUnescaped(TStringBuf("pers_suggest"), TStringBuf("0"));

        // constructing restriction params
        const auto& restrictionMode = searchCallArgs.GetRestrictionMode();
        if (!restrictionMode.Empty()) {
            if (restrictionMode == "family") {
                cgi.InsertUnescaped("relev", "pf=strict");
            } else if (restrictionMode == "moderate") {
                cgi.InsertUnescaped("relev", "pf=moderate");
            } else if (restrictionMode == "kids" && !searchCallArgs.GetRestrictionAge().Empty()) {
                cgi.InsertUnescaped("relev", "age_limit="+searchCallArgs.GetRestrictionAge());
            } else if (restrictionMode == "kids") {
                cgi.InsertUnescaped("relev", "pf=strict");
            } else {
                cgi.InsertUnescaped("relev", "pf=off");
            }
        } else {
            LOG_ERROR(logger) << "Unknown restriction mode: " << restrictionMode;
        }

        if (!searchCallArgs.GetSearchEntref().Empty()) {
            cgi.InsertUnescaped(TStringBuf("entref"), searchCallArgs.GetSearchEntref());
        }
        if (!searchCallArgs.GetSearchText().Empty()) {
            cgi.InsertUnescaped(TStringBuf("text"), searchCallArgs.GetSearchText());
        } else {
            cgi.InsertUnescaped(TStringBuf("text"), TStringBuf("видео"));
        }
        // AppendFlagsToCgi(cgi, expFlags, "SEARCH_CGI");

        NJson::TJsonValue request;
        request["headers"].SetValue(NJson::JSON_ARRAY);
        auto addHeader = [&request](const TStringBuf& key, const TString& value) {
            NJson::TJsonValue header;
            header.SetValue(NJson::JSON_ARRAY);
            header.AppendValue(key);
            header.AppendValue(value);
            request["headers"].AppendValue(header);
        };
        const auto host = "yandex.ru";
        addHeader("Host", host);

        // addHeader(NAlice::NNetwork::HEADER_X_YANDEX_APP_INFO, context.GetAppInfoHeader());
        if (const auto* appInfoPtr = requestWrapper.GetDataSource(NAlice::EDataSourceType::APP_INFO)) {
            const auto& appInfo = appInfoPtr->GetAppInfo();
            addHeader(NAlice::NNetwork::HEADER_X_YANDEX_APP_INFO, appInfo.GetValue());
        }
        addHeader(NAlice::NNetwork::HEADER_USER_AGENT, requestWrapper.BaseRequestProto().GetOptions().GetUserAgent());

        // if (requestWrapper.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_DISABLE_OAUTH) && requestWrapper.UserTicket().Defined()) {
            // addHeader(NAlice::NNetwork::HEADER_X_YA_USER_TICKET, *requestWrapper.UserTicket());
        // } else if (context.IsAuthorizedUser()) {
            // addHeader("Authorization", context.UserAuthorizationHeader());
        // }

        const auto appHostParams = GetLastSection(ctx.ServiceCtx, "app_host_params");
        TString appHostReqid = appHostParams["reqid"].GetString();
        TStringBuilder reqid;
        if (appHostReqid.find('$') < appHostReqid.Size()) {
            reqid << appHostReqid.substr(0, appHostReqid.find('$'));
            reqid << "-" << requestWrapper.BaseRequestProto().GetRandomSeed();
            TString remoteHost = HostName();
            if (remoteHost) {
                reqid << "-" << remoteHost;
            }
        } else {
            reqid << requestWrapper.BaseRequestProto().GetServerTimeMs() << "000";
            reqid << "-" << requestWrapper.BaseRequestProto().GetRandomSeed();
        }
        LOG_INFO(logger) << "ALICE Reqid: " << appHostReqid;
        LOG_INFO(logger) << "VideoSearch Reqid: " << reqid;
        addHeader(NAlice::NNetwork::HEADER_X_YANDEX_REQ_ID, reqid);

        addHeader(NAlice::NNetwork::HEADER_X_YANDEX_INTERNAL_REQUEST, "1");
        request["uri"] = TStringBuilder{} << "/video/result?" << cgi.Print();
        request["type"] = "http_request";
        request["method"] = "GET";

        ctx.ServiceCtx.AddItem(request, "video_search_request");
        LOG_INFO(logger) << "http://" << host << request["uri"].GetString();

        if (requestWrapper.HasExpFlag(NAlice::NVideoCommon::FLAG_VIDEO_SEARCH_DEBUG_GIZMOS)) {
            LOG_INFO(logger) << "whole request: " << JsonToString(request);
        }
    }

    void TVideoSearchPrepare::Do(TScenarioHandleContext& ctx) const {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

        const auto searchArgs = GetOnlyProtoOrThrow<TVideoSearchCallArgs>(ctx.ServiceCtx, VIDEO_SEARCH_CALL);
        auto& logger = ctx.Ctx.Logger();
        LOG_INFO(logger) << "Preparing request";
        SetupVideoSearchRequest(ctx, request, searchArgs);
    }

    void TVideoSearchProcess::Do(TScenarioHandleContext& ctx) const {
        const auto requestProto = GetOnlyProtoOrThrow<NScenarios::TScenarioRunRequest>(ctx.ServiceCtx, REQUEST_ITEM);
        const TScenarioRunRequestWrapper request{requestProto, ctx.ServiceCtx};

        auto& logger = ctx.Ctx.Logger();
        auto& sensors = ctx.Ctx.GlobalContext().Sensors();
        NJson::TJsonValue videoSearchResponse;
        auto httpResponse = GetLastSection(ctx.ServiceCtx, "video_search_response");
        if (httpResponse.IsNull()) {
            NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchResponded(sensors, false);
            LOG_ERROR(logger) << "Undefined video search response";
            return;
        }
        NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchResponded(sensors, true);
        NJson::ReadJsonFastTree(httpResponse["content"].GetStringRobust(), &videoSearchResponse);
        LOG_INFO(logger) << "VideoSearch reqid: " << videoSearchResponse["reqid"].GetString();

        if (TMaybe<NTv::TCarouselItemWrapper> baseInfo = SearchResultParser::ParseBaseInfo(videoSearchResponse["entity_data"], logger)) {
            LOG_INFO(logger) << "Added baseInfo:\n"
                             << baseInfo->ShortUtf8DebugString();
            NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchBaseInfo(sensors, baseInfo);
            ctx.ServiceCtx.AddProtobufItem(std::move(*baseInfo), VIDEO__SEARCH_BASE_INFO);
        } else {
            NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchBaseInfo(sensors, Nothing());
            LOG_INFO(logger) << "BaseInfo was not parsed from videosearch response";
        }

        bool useHalfPiratesFromBaseInfo = request.HasExpFlag(NVideoCommon::FLAG_VIDEO_HALFPIRATE_FROM_BASEINFO);
        if (TMaybe<TTvSearchResultData> searchResult = SearchResultParser::ParseJsonResponse(videoSearchResponse, logger, useHalfPiratesFromBaseInfo)) {
            if (request.HasExpFlag(NVideoCommon::FLAG_VIDEO_DUMP_VIDEOSEARCH_RESULT)) {
                LOG_INFO(logger) << "Added searchResult:\n"
                                 << searchResult->ShortUtf8DebugString();
            }
            NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchResult(sensors, searchResult);
            ctx.ServiceCtx.AddProtobufItem(std::move(*searchResult), VIDEO__SEARCH_RESULT);
        } else {
            NAlice::NHollywoodFw::NVideo::NSearchMetrics::TrackVideoSearchResult(sensors, Nothing());
            LOG_ERR(logger) << "SearchResult was not parsed";
        }
    }

}
