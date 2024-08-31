#include "context.h"

#include <alice/hollywood/library/scenarios/weather/util/translations.h>
#include <alice/hollywood/library/scenarios/weather/util/util.h>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NWeather {

namespace {

constexpr TStringBuf WEATHER_ERROR_ITEM = "weather_error";
constexpr auto STATE_STORAGE_DURATION = TDuration::Minutes(5);

TMaybe<TWeatherPlace> ConstructWeatherPlace(const NAppHost::IServiceContext& ctx) {
    if (!ctx.HasProtobufItem("weather_place")) {
        return {};
    }
    return ctx.GetOnlyProtobufItem<TWeatherPlace>("weather_place");
}

TMaybe<TFrame> ConstructFrame(const NAppHost::IServiceContext& ctx) {
    if (!ctx.HasProtobufItem("semantic_frame")) {
        return {};
    }
    return TFrame::FromProto(ctx.GetOnlyProtobufItem<TSemanticFrame>("semantic_frame"));
}

std::unique_ptr<TForecast> ConstructForecast(TWeatherContext& ctx, bool forecastReady) {
    if (forecastReady) {
        try {
            return std::make_unique<TForecast>(ctx);
        } catch (const yexception& e) {
            LOG_ERROR(ctx.Logger()) << "Failed to parse Forecast. Error occured: " << e.what();
        }
    }
    return nullptr;
}

std::unique_ptr<TNowcast> ConstructNowcast(TWeatherContext& ctx, bool forecastReady) {
    if (forecastReady) {
        if (ctx.Protos().Alert().Defined()) {
            try {
                return std::make_unique<TNowcast>(ctx);
            } catch (const yexception& e) {
                LOG_ERROR(ctx.Logger()) << "Failed to parse Nowcast. Error occured: " << e.what();
            }
        }
    }
    return nullptr;
}

bool ShouldDropWeatherState(const TWeatherState& state, TInstant clientTime) {
    if (state.GetClientTimeMs() == 0) {
        // the field is not filled
        return false;
    }
    const auto prevClientTime = TInstant::MilliSeconds(state.GetClientTimeMs());
    return (clientTime - prevClientTime) >= STATE_STORAGE_DURATION;
}

TWeatherState ConstructWeatherState(const TScenarioRunRequestWrapper& runRequest, TInstant clientTime) {
    TWeatherState state;
    const auto& rawState = runRequest.BaseRequestProto().GetState();
    if (rawState.Is<TWeatherState>() && !runRequest.IsNewSession()) {
        rawState.UnpackTo(&state);

        if (ShouldDropWeatherState(state, clientTime)) {
            // the state is too old, drop it!
            state.Clear();
        }
    }
    return state;
}

} // namespace

TWeatherContext::TWeatherContext(TScenarioHandleContext& ctx, bool forecastReady)
    : TCapabilityWrapper{RunRequest_}
    , Ctx_{&ctx}
    , RunRequestProto_{GetOnlyProtoOrThrow<TScenarioRunRequest>(Ctx_->ServiceCtx, REQUEST_ITEM)}
    , RunRequest_{RunRequestProto_, ctx.ServiceCtx}
    , WeatherState_{ConstructWeatherState(RunRequest_, ClientTime())}
    , AvatarsMap_{RunRequestProto_.GetBaseRequest().GetOptions().GetScreenScaleFactor()}
    , UserLocation_{GetUserLocation(RunRequest_)}
    , WeatherPlace_{ConstructWeatherPlace(ctx.ServiceCtx)}
    , Frame_{ConstructFrame(ctx.ServiceCtx)}
    , Renderer_{std::make_unique<TRenderer>(ctx, RunRequest_, Frame_.Get())}
    , Protos_{std::make_unique<TWeatherProtos>(ctx.ServiceCtx)}
    , Forecast_{ConstructForecast(*this, forecastReady)}
    , Nowcast_{ConstructNowcast(*this, forecastReady)}
{
}

const TScenarioRunRequestWrapper& TWeatherContext::RunRequest() const {
    return RunRequest_;
}

const NGeobase::TLookup& TWeatherContext::GeobaseLookup() const {
    return Ctx_->Ctx.GlobalContext().CommonResources().Resource<TGeobaseResource>().GeobaseLookup();
}

const TUserLocation& TWeatherContext::UserLocation() const {
    return UserLocation_;
}

const TMaybe<TWeatherPlace>& TWeatherContext::WeatherPlace() const {
    return WeatherPlace_;
}

const std::pair<double, double> TWeatherContext::GetLatLon() const {
    if (!WeatherPlace_.Defined()) {
        ythrow yexception() << "TWeatherPlace not defined";
    }

    if (WeatherPlace_->HasUserLatLon()) {
        const auto& userLL = WeatherPlace_->GetUserLatLon();
        return {userLL.GetLat(), userLL.GetLon()};
    } else {
        const NGeobase::TId geoId = WeatherPlace_->GetCityGeoId();

        NGeobase::TRegion region = GeobaseLookup().GetRegionById(geoId);
        return {region.GetLatitude(), region.GetLongitude()};
    }
}

const TAvatarsMap& TWeatherContext::AvatarsMap() const {
    return AvatarsMap_;
}

const TWeatherProtos& TWeatherContext::Protos() const {
    return *Protos_;
}

const std::unique_ptr<TForecast>& TWeatherContext::Forecast() const {
    return Forecast_;
}

const std::unique_ptr<TNowcast>& TWeatherContext::Nowcast() const {
    return Nowcast_;
}

TRenderer& TWeatherContext::Renderer() {
    return *Renderer_;
}

TMaybe<TFrame>& TWeatherContext::Frame() {
    return Frame_;
}

const TMaybe<TFrame>& TWeatherContext::Frame() const {
    return Frame_;
}

bool TWeatherContext::IsSlotEmpty(const TStringBuf name) const {
    if (Frame_.Empty()) {
        return true;
    }
    const auto slot = Frame_->FindSlot(name);
    return !slot || slot->Value.AsString() == "null";
}

TPtrWrapper<TSlot> TWeatherContext::FindOrAddSlot(const TString& name, const TString& type) {
    if (!Frame_) {
        LOG_ERROR(Logger()) << "Trying to find or add slot \"" << name << "\" to empty frame";
        ythrow yexception() << "Can't use slot \"" << name << "\"";
    }

    const auto slot = Frame_->FindSlot(name);
    if (!slot) {
        Frame_->AddSlot(TSlot{.Name = name, .Type = type, .Value = TSlot::TValue{"null"}});
        return Frame_->FindSlot(name);
    }
    return slot;
}

TPtrWrapper<TSlot> TWeatherContext::FindSlot(const TString& name) {
    if (!Frame_) {
        LOG_ERROR(Logger()) << "Trying to find slot \"" << name << "\" to empty frame";
        ythrow yexception() << "Can't use slot \"" << name << "\"";
    }
    return Frame_->FindSlot(name);
}

TPtrWrapper<TSlot> TWeatherContext::FindSlot(const TString& name) const {
    if (!Frame_) {
        ythrow yexception() << "Can't use slot \"" << name << "\"";
    }
    return Frame_->FindSlot(name);
}

void TWeatherContext::AddSlot(const TString& name, const TString& type, TString value) {
    auto slot = FindOrAddSlot(name, type);
    const_cast<TSlot*>(slot.Get())->Value = TSlot::TValue{std::move(value)};

}

void TWeatherContext::AddSlot(const TString& name, const TString& type, const NJson::TJsonValue& value) {
    auto slot = FindOrAddSlot(name, type);
    const_cast<TSlot*>(slot.Get())->Value = TSlot::TValue{NJson::WriteJson(value)};
}

void TWeatherContext::RemoveSlot(const TString& name) {
    if (!Frame_) {
        LOG_INFO(Logger()) << "Trying to remove slot \"" << name << "\" in empty frame which is not necessary";
        return;
    }

    Frame_->RemoveSlots(name);
}

void TWeatherContext::AddError(const TWeatherError& error) {
    NJson::TJsonValue errorJson;
    errorJson["message"] = error.Message();
    errorJson["code"] = ToString(error.Code());
    Ctx_->ServiceCtx.AddItem(std::move(errorJson), WEATHER_ERROR_ITEM);
}

TMaybe<TWeatherError> TWeatherContext::GetError(const TStringBuf itemName) const {
    if (!Ctx_->ServiceCtx.HasItem(itemName)) {
        return Nothing();
    }
    const NJson::TJsonValue errorJson = Ctx_->ServiceCtx.GetOnlyItem(itemName);
    return TWeatherError{FromString<EWeatherErrorCode>(errorJson["code"].GetString())} << errorJson["message"].GetString();
}

const TCallbackDirective* TWeatherContext::GetCallback() const {
    return RunRequest_.Input().GetCallback();
}

const TWeatherState& TWeatherContext::WeatherState() const {
    return WeatherState_;
}

TInstant TWeatherContext::ClientTime() const {
    return TInstant::ParseIso8601(RunRequest_.Proto().GetBaseRequest().GetClientInfo().GetClientTime());
}

bool TWeatherContext::IsCollectCardRequest() const {
    if (!Frame_) {
        return false;
    }
    return Frame_->Name() == NFrameNames::COLLECT_CENTAUR_CARDS;
}

bool TWeatherContext::IsCollectMainScreenRequest() const {
    if (!Frame_) {
        return false;
    }
    return Frame_->Name() == NFrameNames::COLLECT_CENTAUR_MAIN_SCREEN;
}

bool TWeatherContext::IsCollectWidgetGalleryRequest() const {
    if (!Frame_) {
        return false;
    }
    return Frame_->Name() == NFrameNames::COLLECT_CENTAUR_WIDGET_GALLERY;
}

bool TWeatherContext::IsCollectTeasersPreviewRequest() const {
    if (!Frame_) {
        return false;
    }
    return Frame_->Name() == NFrameNames::COLLECT_CENTAUR_TEASERS_PREVIEW;
}

bool TWeatherContext::SupportsCloudUi() const {
    return TCapabilityWrapper::SupportsCloudUi()
        && RunRequest_.HasExpFlag(NExperiment::WEATHER_USE_CLOUD_UI);
}

} // namespace NAlice::NHollywood::NWeather
