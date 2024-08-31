#include "request_builder.h"

#include <library/cpp/string_utils/base64/base64.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>

#include <alice/cuttlefish/library/cuttlefish/common/datasync_parser.h>

#include <alice/megamind/protos/common/iot.pb.h>
#include <alice/megamind/protos/common/experiments.pb.h>

#include <util/datetime/base.h>


namespace NGProxy {


TRequestBuilderImpl::TRequestBuilderImpl() {
    InitRequest();
}


void TRequestBuilderImpl::SetSession(const NAliceProtocol::TSessionContext& session) {
    Session = session;

    //
    //  Fill application
    //
    Request["application"]["app_id"] = session.GetAppId();
    Request["application"]["device_id"] = session.GetDeviceInfo().GetDeviceId();
    Request["application"]["lang"] = session.GetAppLang();

    if (session.GetDeviceInfo().HasDeviceManufacturer()) {
        Request["application"]["device_manufacturer"] = session.GetDeviceInfo().GetDeviceManufacturer();
    }

    if (session.GetDeviceInfo().HasDeviceModel()) {
        Request["application"]["device_model"] = session.GetDeviceInfo().GetDeviceModel();
    }

    if (session.GetDeviceInfo().HasDeviceModification()) {
        Request["application"]["device_revision "] = session.GetDeviceInfo().GetDeviceModification();
    }

    if (session.GetDeviceInfo().HasPlatform()) {
        Request["application"]["platform"] = session.GetDeviceInfo().GetPlatform();
    }

    if (session.GetUserInfo().HasUuid()) {
        Request["application"]["uuid"] = session.GetUserInfo().GetUuid();
    }


    //
    //  Fill device_state
    //
    Request["request"]["device_state"]["device_id"] = session.GetDeviceInfo().GetDeviceId();
}


void TRequestBuilderImpl::SetContext(const NAliceProtocol::TContextLoadResponse& context) {
    if (context.HasMementoResponse()) {
        auto content = context.GetMementoResponse().GetContent();
        Request["memento"] = Base64Encode(content);
    }

    if (context.HasIoTUserInfo()) {
        NAlice::TIoTUserInfo content = context.GetIoTUserInfo();
        Request["iot_user_info_data"] = Base64Encode(content.SerializeAsString());
    }

    if (context.HasLaasResponse()) {
        const TString content = context.GetLaasResponse().GetContent();

        NJson::TJsonValue laas;

        if (NJson::ReadJsonTree(content, &laas, false)) {
            Request["request"]["laas_region"] = laas;
        }
    }

    if (context.HasNotificatorResponse()) {
        // TODO
    }

    {
        NAlice::NCuttlefish::NAppHostServices::TDatasyncResponseParser parser;

        if (context.HasDatasyncResponse()) {
            parser.ParseDatasyncResponse(context.GetDatasyncResponse());
        } else if (context.HasDatasyncUuidResponse()) {
            parser.ParseDatasyncResponse(context.GetDatasyncUuidResponse());
            Request["request"]["personal_data"] = parser.PersonalData;
        } else if (context.HasDatasyncDeviceIdResponse()) {
            parser.ParseDatasyncResponse(context.GetDatasyncDeviceIdResponse());
            Request["request"]["personal_data"] = parser.PersonalData;
        }

        if (parser.PersonalData.IsMap() && parser.PersonalData.IsDefined()) {
            Request["request"]["personal_data"] = parser.PersonalData;
        }
    }

    if (context.HasFlagsInfo()) {
        auto flags = context.GetFlagsInfo();

        if (flags.AllTestIdsSize()) {
            NJson::TJsonArray testids;
            for (size_t i = 0; i < flags.AllTestIdsSize(); ++i) {
                testids.AppendValue(flags.GetAllTestIds(i));
            }

            Request["request"]["test_ids"] = testids;
        }

        if (flags.HasVoiceFlags()) {
            auto f = flags.GetVoiceFlags();
            if (f.StorageSize()) {
                NJson::TJsonMap fmap;

                auto it = f.GetStorage().begin();

                while (it != f.GetStorage().end()) {
                    if (it->second.HasString()) {
                        fmap[it->first] = it->second.GetString();
                    } else if (it->second.HasNumber()) {
                        fmap[it->first] = it->second.GetNumber();
                    } else if (it->second.HasBoolean()) {
                        fmap[it->first] = it->second.GetBoolean();
                    } else if (it->second.HasInteger()) {
                        fmap[it->first] = it->second.GetInteger();
                    }
                    ++it;
                }

                Request["request"]["experiments"] = fmap;
            }
        }
    }

}


void TRequestBuilderImpl::SetGrpcData(const NGProxy::TMetadata& meta, const NGProxy::GSetupRequestInfo& info, const NGProxy::GSetupRequest& req) {
    Metadata = meta;

    Request["header"]["session_id"] = meta.GetSessionId();
    Request["header"]["request_id"] = meta.GetRequestId();

    if (meta.HasOAuthToken()) {
        Request["oauth_token"] = meta.GetOAuthToken();
        Request["request"]["additional_options"]["oauth_token"] = meta.GetOAuthToken();
    }

    auto& experimentsMap = Request["request"]["experiments"];
    if (meta.HasExperiments()) {
        NJson::TJsonValue experiments;
        const auto expString = TStringBuf(meta.GetExperiments());
        if (NJson::ReadJsonTree(expString, &experiments)) {
            for (const auto& experiment : experiments.GetArraySafe())
            {
                experimentsMap[experiment.GetStringSafe()] = "1";
            }
        }
    }
    experimentsMap["video_qproxy_players"] = "1";
    experimentsMap["mm_enable_protocol_scenario=Video"] = "1";
    experimentsMap["use_memento"] = "1";
    experimentsMap["use_app_host_pure_Video_scenario"] = "1";
    experimentsMap["mm_enable_protocol_scenario=TvFeatureBoarding"] = "1";

    auto& supportedFeaturesMap = Request["request"]["additional_options"]["supported_features"];
    if (!meta.GetSupportedFeatures().empty()) {
        for (const auto& metaFeatures : meta.GetSupportedFeatures()) {
            NJson::TJsonValue supportedFeatures;
            if (NJson::ReadJsonTree(metaFeatures, &supportedFeatures)) {
                for (const auto& feature : supportedFeatures.GetArraySafe()) {
                    supportedFeaturesMap.AppendValue(feature.GetStringSafe());
                }
            }
        }
    }
    supportedFeaturesMap.AppendValue("server_action");
    supportedFeaturesMap.AppendValue("set_alarm");
    supportedFeaturesMap.AppendValue("set_timer");

    auto& unsupportedFeaturesMap = Request["request"]["additional_options"]["unsupported_features"];
    if (!meta.GetUnsupportedFeatures().empty()) {
        for (const auto& metaFeatures : meta.GetUnsupportedFeatures()) {
            NJson::TJsonValue unsupportedFeatures;
            if (NJson::ReadJsonTree(metaFeatures, &unsupportedFeatures)) {
                for (const auto& feature : unsupportedFeatures.GetArraySafe()) {
                    unsupportedFeaturesMap.AppendValue(feature.GetStringSafe());
                }
            }
        }
    }

    Request["request"]["additional_options"]["server_time_ms"] = TInstant::Now().MilliSeconds();
    Request["request"]["additional_options"]["bass_options"]["client_ip"] = TString{meta.GetIpAddr()};
    if (meta.HasUserAgent()) {
        Request["request"]["additional_options"]["bass_options"]["user_agent"] = meta.GetUserAgent();
    }

    if (meta.HasApplication()) {
        // TODO(akormushkin@) Fill app_info with base64 encode
        // Request["request"]["additional_options"]["app_info"] = meta.GetApplication();
    }

    SetGrpcRequest(Request, meta, info, req);
}


void TRequestBuilderImpl::InitRequest() {
    Request["application"] = NJson::TJsonMap();
    Request["header"] = NJson::TJsonMap();
    Request["request"] = NJson::TJsonMap();
    Request["request"]["event"] = NJson::TJsonMap();
    Request["request"]["additional_options"] = NJson::TJsonMap();
    Request["request"]["additional_options"]["bass_options"] = NJson::TJsonMap();
    Request["request"]["experiments"] = NJson::TJsonMap();
}


NAppHostHttp::THttpRequest TRequestBuilderImpl::Build() const {
    NAppHostHttp::THttpRequest req;

    req.SetMethod(::NAppHostHttp::THttpRequest_EMethod_Get);
    req.SetScheme(::NAppHostHttp::THttpRequest_EScheme_Http);

    {
        auto it = req.AddHeaders();
        it->SetName("Content-Type");
        it->SetValue("application/json");
    }

    if (Request["header"].Has("request_id") && Request["header"]["request_id"].IsString()){
        auto it = req.AddHeaders();
        it->SetName("x-alice-client-reqid");
        it->SetValue(Request["header"]["request_id"].GetStringSafe(""));
    }

    if (Metadata.HasUserAgent()) {
        {
            auto it = req.AddHeaders();
            it->SetName("x-ya-user-agent");
            it->SetValue(Metadata.GetUserAgent());
        }

        {
            auto it = req.AddHeaders();
            it->SetName("User-Agent");
            it->SetValue(Metadata.GetUserAgent());
        }
    }

    {
        auto it = req.AddHeaders();
        it->SetName("X-Alice-AppType");
        it->SetValue(Session.GetAppType());
    }

    {
        auto it = req.AddHeaders();
        it->SetName("X-Alice-AppId");
        it->SetValue(Session.GetAppId());
    }

    if (Metadata.GetInternalRequest()) {
        {
            auto it = req.AddHeaders();
            it->SetName("X-Ya-Internal-Request");
            it->SetValue("1");
        }

        {
            auto it = req.AddHeaders();
            it->SetName("X-Yandex-Internal-Request");
            it->SetValue("1");
        }
    }

    req.SetContent(NJson::WriteJson(Request));

    return req;
}


}   // namespace NGProxy
