#include "prepare_common_handle.h"

#include <alice/hollywood/library/scenarios/weather/context/context.h>
#include <alice/hollywood/library/scenarios/weather/request_helper/geometasearch.h>
#include <alice/hollywood/library/scenarios/weather/request_helper/reqwizard.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

#include <alice/hollywood/library/global_context/global_context.h>
#include <alice/hollywood/library/nlg/nlg_wrapper.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/hollywood/library/scenarios/weather/proto/weather.pb.h>

#include <util/string/join.h>

#include <apphost/lib/proto_answers/http.pb.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

namespace {

void DropSlotTypePrefixes(TFrame& frame) {
    for (auto& slot: frame.Slots()) {
        TStringBuf type = slot.Type;
        if (type.SkipPrefix("sys.") || type.SkipPrefix("custom.")) {
            // FIXME(sparkle): this is a hack, hacks are bad
            const_cast<TSlot&>(slot).Type = type;
        }
    }
}

TMaybe<std::pair<TStringBuf, TFrame>> ConstructNewFrame(TStringBuf prevFrameName, const TScenarioRunRequestWrapper& request,
                                                        const TMaybe<TString>& frameNameFromUpdateForm) {
    for (const TStringBuf newFrameName: WEATHER_FRAMES_ORDER) {
        if (!IsNewFrameLegal(prevFrameName, newFrameName) ||
            newFrameName == NFrameNames::GET_WEATHER_CHANGE && !request.HasExpFlag(NExperiment::WEATHER_CHANGE))
        {
            continue;
        }
        if (const auto rawFrame = request.Input().FindSemanticFrame(newFrameName)) {
            TFrame frame = TFrame::FromProto(*rawFrame);
            DropSlotTypePrefixes(frame);
            return std::make_pair(newFrameName, std::move(frame));
        }
        if (frameNameFromUpdateForm.Defined() && frameNameFromUpdateForm.GetRef() == newFrameName) {
            TFrame frame{frameNameFromUpdateForm.GetRef()};
            return std::make_pair(newFrameName, std::move(frame));
        }
    }
    return Nothing();
}

void ConstructWeatherDetailsResponse(TWeatherContext& ctx, const TFrame& frame) {
    auto& renderer = ctx.Renderer();
    TMaybe<TString> uriMaybe = GetWeatherForecastUri(frame);

    if (uriMaybe.Empty()) {
        LOG_ERROR(ctx.Logger()) << "Will not init Weather details semantic frame - have no URI to show";

        renderer.Builder().SetIrrelevant();
        renderer.AddTextCard(NNlgTemplateNames::IRRELEVANT, "irrelevant");
    } else {
        const auto& ci = ctx.RunRequest().ClientInfo();
        if (!ci.IsSmartSpeaker() && !ci.IsYaAuto()) {
            LOG_INFO(ctx.Logger()) << "Adding open_uri directive";
            renderer.AddOpenUriDirective(*uriMaybe);

            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER_DETAILS, "render_result");
        } else {
            renderer.AddTextCard(NNlgTemplateNames::GET_WEATHER_DETAILS, "render_unsupported");
        }

        renderer.AddSuggests({ESuggestType::Feedback});

        renderer.SetProductScenarioName("weather");
        renderer.SetIntentName(TString{NFrameNames::GET_WEATHER__DETAILS});
    }

    renderer.Render();
    const auto response = *std::move(renderer.Builder()).BuildResponse();
    ctx->ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
}

} // namespace

void TWeatherPrepareCommonHandle::Do(TScenarioHandleContext& ctx) const {
    TWeatherContext weatherCtx{ctx};
    auto& runRequest = weatherCtx.RunRequest();

    TMaybe<TString> frameNameFromUpdateForm;
    const TCallbackDirective* callback = weatherCtx.GetCallback();
    if (callback) {
        LOG_INFO(weatherCtx.Logger()) << "Got callback: " << callback->GetName().Quote();
        if (callback->GetName() == CALLBACK_FEEDBACK_NAME) {
            auto& renderer = weatherCtx.Renderer();
            renderer.RenderFeedbackAnswer(callback);

            renderer.Render();
            const auto response = *std::move(renderer.Builder()).BuildResponse();
            weatherCtx->ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
            return;
        } else if (callback->GetName() == CALLBACK_UPDATE_FORM_NAME) {
            frameNameFromUpdateForm = GetFrameNameFromCallback(*callback);
        }
    }

    const TWeatherState& state = weatherCtx.WeatherState();
    const TStringBuf prevFrameName = GetFrameName(state);

    if (auto maybeFrame = ConstructNewFrame(prevFrameName, runRequest, frameNameFromUpdateForm)) {
        auto& [frameName, frame] = *maybeFrame;

        if (IsTakeSlotsFromPrevFrame(frameName, runRequest.ExpFlags()) && state.HasSemanticFrame()) {
            FillFromPrevFrame(weatherCtx.Logger(), TFrame::FromProto(state.GetSemanticFrame()), frame);
        }
        FixDayPartSlot(weatherCtx.Logger(), frame.FindSlot("day_part"));

        if (frameName == NFrameNames::GET_WEATHER__DETAILS) {
            ConstructWeatherDetailsResponse(weatherCtx, frame);
        } else {
            weatherCtx->ServiceCtx.AddProtobufItem(frame.ToProto(), TStringBuf("semantic_frame"));

            const auto where = frame.FindSlot(TStringBuf("where"));

            TStringBuf value;
            if (where && (where->Type == "string" || where->Type == "where")) {
                value = where->Value.AsString();
            }

            if (!value.empty()) {
                if (!IsGeoMetaSearchDisabled(runRequest.ExpFlags())) {
                    TGeometasearchRequestHelper<ERequestPhase::Before> geoMetaSearch{ctx};
                    geoMetaSearch.AddRequest(value);
                }

                TReqwizardRequestHelper<ERequestPhase::Before> reqWizard{ctx};
                reqWizard.AddRequest(value);
            }
        }
    } else {
        LOG_INFO(weatherCtx.Logger()) << "Will not init Weather semantic frame, got not allowed intents";

        auto& renderer = weatherCtx.Renderer();
        renderer.Builder().SetIrrelevant();
        renderer.AddTextCard(NNlgTemplateNames::IRRELEVANT, "irrelevant");

        renderer.Render();
        const auto response = *std::move(renderer.Builder()).BuildResponse();
        weatherCtx->ServiceCtx.AddProtobufItem(response, RESPONSE_ITEM);
    }
}

}  // namespace NAlice::NHollywood::NWeather
