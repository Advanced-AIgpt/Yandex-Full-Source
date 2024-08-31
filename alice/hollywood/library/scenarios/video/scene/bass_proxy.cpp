#include "bass_proxy.h"

#include <alice/hollywood/library/framework/core/render_impl.h>
#include <alice/hollywood/library/http_proxy/http_proxy.h>
#include <alice/library/network/headers.h>

#include <alice/megamind/protos/common/environment_state.pb.h>
#include <alice/protos/endpoint/endpoint.pb.h>

using namespace NAlice::NScenarios;
namespace NAlice::NHollywoodFw::NVideo {

inline static const TStringBuf TV_REQUEST = "tv_request";
inline static const TStringBuf SMARTSPEAKER_REQUEST = "smartspeaker_request";
inline static const TStringBuf OTHERS_REQUEST = "others_request";
inline static const TStringBuf NO_RESPONSE = "no_response";


void IncreaseBassMetrics(NMetrics::ISensors& sensors, TStringBuf labelName) {
    NMonitoring::TLabels label{
        {"scenario_name", "video"},
        {"name", "bass_proxy_metrics"},
    };

    label.Add("subname", labelName);
    sensors.IncRate(label);
}

TRetSetup TVideoBassProxy::RunSetup(const TBassProxySceneArgs&, const TRunRequest& request, const TStorage&) const {
    auto runRequest = request.GetRunRequest();
    TString requestType = "/run"; // only run used

    try {
        LOG_INFO(request.Debug().Logger()) << "Adding datasources if any";

        for (int dsId = 1; dsId < EDataSourceType_ARRAYSIZE; ++dsId) {
            if (!EDataSourceType_IsValid(dsId) || dsId == NAlice::EDataSourceType::WEB_SEARCH_DOCS) { // skip WEB_SEARCH_DOCS forcefully: see SMARTTVBACKEND-1231
                continue;
            }
            if (const TDataSource* ds = request.GetDataSource(static_cast<NAlice::EDataSourceType>(dsId), false); ds) {
                (*runRequest.MutableDataSources())[dsId] = *ds;
            }
        }

        TString data;
        if (!runRequest.SerializeToString(&data)) {
            TError err(TError::EErrorDefinition::ProtobufCast);
            err.Details() << requestType << " prepare pack: failed to serialize proto " << runRequest.GetTypeName() << " to string";
            return err;
        }

        auto builder = NHollywood::THttpProxyRequestBuilder(requestType, request.GetRequestMeta(), request.Debug().Logger(), "video_bass")
                            .SetMethod(NAppHostHttp::THttpRequest_EMethod::THttpRequest_EMethod_Post)
                            .SetBody(data, NContentTypes::APPLICATION_PROTOBUF)
                            .AddHeader(NNetwork::HEADER_CONTENT_TYPE, NContentTypes::APPLICATION_PROTOBUF);
        if (request.GetRequestMeta().GetOAuthToken()) {
            builder.SetUseOAuth();
        }
        TSetup setup(request);
        setup.Attach(builder.Build());
        LOG_INFO(request.Debug().Logger()) << "Packed HTTP " << requestType << " request.";
        return setup;
    } catch (...) {
        TError err(TError::EErrorDefinition::ProtobufCast);
        err.Details() << requestType << " prepare pack: failed to pack HTTP request" << runRequest.GetTypeName() << " to string\nerror: " << CurrentExceptionMessage();
        return err;
    }
}

TRetMain TVideoBassProxy::Main(const TBassProxySceneArgs&, const TRunRequest& request, TStorage&, const TSource& source) const {
    try {
        auto rawResponse = source.GetRawHttpContent();
        if (auto response = TScenarioRunResponse(); response.ParseFromString(*rawResponse)) {
            request.AI().OverrideIntent(response.GetFeatures().GetIntent());
            LOG_INFO(request.Debug().Logger()) << "Overrided bass intent: " << response.GetFeatures().GetIntent();
            auto returnValue = TReturnValueRender(&TVideoBassProxy::RenderRun, response, response.GetFeatures());
            if (response.GetFeatures().GetIsIrrelevant()) {
                return returnValue.MakeIrrelevantAnswerFromScene();
            }
            return returnValue;
        } else {
            TError err(TError::EErrorDefinition::ProtobufCast);
            err.Details() << " prepare pack: failed to parse HTTP BASS request /run to proto TScenarioRunResponse\nerror: " << CurrentExceptionMessage();
            return err;
        }
    } catch (yexception exception) {
        IncreaseBassMetrics(request.System().GetSensors(), NO_RESPONSE);
        TError err(TError::EErrorDefinition::Exception);
        err.Details() << "No videobass response\n" << CurrentExceptionMessage();
        return err;
    }
}

TRetResponse TVideoBassProxy::RenderRun(const TScenarioRunResponse& response, TRender& render) const {
    render.GetResponseBody() = response.GetResponseBody();
    if (render.GetRequest().Client().GetClientInfo().IsTvDevice() || render.GetRequest().Client().GetClientInfo().IsYaModule()) {
        IncreaseBassMetrics(render.GetRequest().System().GetSensors(), TV_REQUEST);
    } else if (render.GetRequest().Client().GetClientInfo().IsSmartSpeaker()) {
        IncreaseBassMetrics(render.GetRequest().System().GetSensors(), SMARTSPEAKER_REQUEST);
    } else {
        IncreaseBassMetrics(render.GetRequest().System().GetSensors(), OTHERS_REQUEST);
    }
    return TReturnValueSuccess();
}

} // namespace NAlice::NHollywoodFw::NVideo
