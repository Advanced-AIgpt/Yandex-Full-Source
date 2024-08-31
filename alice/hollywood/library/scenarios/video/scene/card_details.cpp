#include "card_details.h"

#include <alice/hollywood/library/framework/core/codegen/gen_directives.pb.h>
#include <alice/library/analytics/common/product_scenarios.h>

#include <alice/megamind/protos/scenarios/response.pb.h>
#include <alice/protos/data/video/card_detail.pb.h>

namespace NAlice::NHollywoodFw::NVideo {

    TRetContinue TVideoCardDetailScene::Continue(const TCardDetailSceneArgs&, const TContinueRequest& request, TStorage&, const TSource& source) const {
        if (auto jsonResponse = source.GetHttpResponseJson(); !jsonResponse.IsNull()) {
            LOG_DEBUG(request.Debug().Logger()) << "Response from droideka: " << jsonResponse.GetStringRobust();
            NScenarios::TCallbackDirective callbackDirective;
            callbackDirective.SetName(ToString(CALLBACK_DIRECTIVE_NAME));

            (*callbackDirective.MutablePayload()->mutable_fields()) ["grpc_response"].set_string_value (
                JsonStringFromProto(JsonToProto<TTvCardDetailResponse>(jsonResponse, true, true))
            );
            request.AI().OverrideProductScenarioName(NAlice::NProductScenarios::DROIDEKA);
            return TReturnValueRender(&TVideoCardDetailScene::Render, callbackDirective);
        } else {
            TError err(TError::EErrorDefinition::SubsystemError);
            err.Details() << "Droideka request failed, " << jsonResponse.GetStringRobust();
            return err;
        }
    }

    TRetContinue TVideoCardDetailThinScene::Continue(const TCardDetailSceneArgs&, const TContinueRequest& request, TStorage&, const TSource& source) const {
        if (auto jsonResponse = source.GetHttpResponseJson(); !jsonResponse.IsNull()) {
            LOG_DEBUG(request.Debug().Logger()) << "Response from droideka: " << jsonResponse.GetStringRobust();
            NScenarios::TCallbackDirective callbackDirective;
            callbackDirective.SetName(ToString(CALLBACK_DIRECTIVE_NAME));

            (*callbackDirective.MutablePayload()->mutable_fields()) ["grpc_response"].set_string_value (
                JsonStringFromProto(JsonToProto<TTvCardDetailResponse>(jsonResponse, true, true))
            );
            request.AI().OverrideProductScenarioName(NAlice::NProductScenarios::DROIDEKA);
            return TReturnValueRender(&TVideoCardDetailThinScene::Render, callbackDirective);
        } else {
            TError err(TError::EErrorDefinition::SubsystemError);
            err.Details() << "Droideka request failed, " << jsonResponse.GetStringRobust();
            return err;
        }
    }

    TRetResponse TVideoCardDetailScene::Render(const NScenarios::TCallbackDirective& directiveArg, TRender& render) const {
        {
            TString actionId = ToString(CALLBACK_DIRECTIVE_NAME);
            auto& action = *render.GetResponseBody().MutableAnalyticsInfo()->AddActions();
            action.SetId(actionId);
            action.SetName(actionId);
            action.SetHumanReadable("");
        }
        auto callbackDirective = directiveArg;
        render.Directives().AddCallbackDirective(std::move(callbackDirective));
        return TReturnValueSuccess();
    }

    TRetResponse TVideoCardDetailThinScene::Render(const NScenarios::TCallbackDirective& directiveArg, TRender& render) const {
        {
            TString actionId = ToString(CALLBACK_DIRECTIVE_NAME);
            auto& action = *render.GetResponseBody().MutableAnalyticsInfo()->AddActions();
            action.SetId(actionId);
            action.SetName(actionId);
            action.SetHumanReadable("");
        }
        auto callbackDirective = directiveArg;
        render.Directives().AddCallbackDirective(std::move(callbackDirective));
        return TReturnValueSuccess();
    }

    TCardDetailSceneArgs TVideoCardDetailScene::MakeVideoCardDetailSceneArgs(const TFrameGetCardDetails& frame) {
        TCgiParameters params;

        if (frame.ContentId.Value) {
            params.insert(std::make_pair(frame.ContentId.GetName(), *frame.ContentId.Value));
        }
        if (frame.ContentType.Value) {
            params.insert(std::make_pair(frame.ContentType.GetName(), *frame.ContentType.Value));
        }
        if (frame.OntoId.Value) {
            params.insert(std::make_pair(frame.OntoId.GetName(), *frame.OntoId.Value));
        }

        TCardDetailSceneArgs cardDetailArgs;
        cardDetailArgs.SetPath(URI + params.Print());
        return cardDetailArgs;
    }

    TCardDetailSceneArgs TVideoCardDetailThinScene::MakeVideoCardDetailThinSceneArgs(const TFrameGetCardDetailsThin& frame) {
        TCgiParameters params;

        params.insert(std::make_pair(frame.ContentId.GetName(), *frame.ContentId.Value));

        TCardDetailSceneArgs cardDetailArgs;
        cardDetailArgs.SetPath(URI + params.Print());
        return cardDetailArgs;
    }

    THttpHeaders TBaseCardDetailsScene::MakeDroidekaHeaders(const TContinueRequest& request) {
        THttpHeaders result;

        if (const auto& deviceId = request.Client().GetClientInfo().DeviceId) {
            result.AddHeader("X-YaQuasarDeviceId", deviceId);
        }
        if (const auto& quasarPlatform = request.Client().GetClientInfo().DeviceModel) {
            result.AddHeader("X-YaQuasarPlatform", quasarPlatform);
        }
        if (const auto& clientIp = request.GetRequestMeta().GetClientIP()) {
            result.AddHeader("X-Forwarded-For", clientIp);
        }
        if (const auto& appmetrikaUuid = request.Client().GetClientInfo().Uuid) {
            result.AddHeader("X-YaUUID", appmetrikaUuid);
        }

        return result;
    }

} // namespace NAlice::NHollywoodFw::NVideo
