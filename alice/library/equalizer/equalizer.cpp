#include "equalizer.h"

#include <alice/hollywood/library/environment_state/endpoint.h>

#include <alice/megamind/protos/scenarios/directives.pb.h>
#include <alice/protos/endpoint/capability.pb.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/resource/resource.h>

#include <array>

namespace NAlice {

namespace {

constexpr size_t GAINS_LENGTH = 5;

struct TEqualizerSettings {
    TVector<double> Gains;
};

enum struct EEqualizerSupportStatus {
    NotSupported = 0,
    Fixed = 1,
    Adjustable = 2,
};

NJson::TJsonValue LoadJsonFromResource(TStringBuf filename) {
    TString raw = NResource::Find(filename);
    NJson::TJsonValue json;
    NJson::ReadJsonTree(raw, &json);
    return json;
}

TEqualizerSettings ConstructEqualizerSettingsFromJson(const NJson::TJsonValue& value) {
    TEqualizerSettings settings;
    for (const auto& gain : value["bands_gain"].GetArray()) {
        settings.Gains.push_back(gain.GetDoubleSafe());
    }
    return settings;
}

const THashMap<TString, TString> SEED_TO_PRESET_MAPPING = []{
    NJson::TJsonValue json = LoadJsonFromResource("seed_to_preset_mapping.json");

    THashMap<TString, TString> mapping;
    for (const auto& [key, value] : json.GetMap()) {
        mapping[key] = value.GetStringSafe();
    }
    return mapping;
}();

const THashMap<TString, TEqualizerSettings> PRESETS = []{
    NJson::TJsonValue json = LoadJsonFromResource("presets.json");

    THashMap<TString, TEqualizerSettings> presets;
    for (const auto& [key, value] : json.GetMap()) {
        presets[key] = ConstructEqualizerSettingsFromJson(value);
    }
    return presets;
}();

TMaybe<TStringBuf> TryGetPresetName(TStringBuf seed) {
    if (const auto iter = SEED_TO_PRESET_MAPPING.find(seed); iter != SEED_TO_PRESET_MAPPING.end()) {
        return iter->second;
    }
    return Nothing();
}

const TEqualizerSettings& FindEqualizerSettings(TStringBuf seed) {
    if (const auto preset = TryGetPresetName(seed)) {
        const auto iter = PRESETS.find(*preset);
        Y_ENSURE(iter != PRESETS.end());
        return iter->second;
    }

    static const TEqualizerSettings DEFAULT_EQUALIZER_SETTINGS{
        .Gains = {0, 0, 0, 0, 0},
    };
    return DEFAULT_EQUALIZER_SETTINGS;
}

bool SupportsDirective(const TEqualizerCapability& capability, TCapability_EDirectiveType directiveType) {
    return AnyOf(capability.GetMeta().GetSupportedDirectives(), [directiveType](const auto supportedDirectiveType) {
        return supportedDirectiveType == directiveType;
    });
}

void BuildFixedDirective(const TEqualizerSettings& settings, TEqualizerCapability_TSetFixedEqualizerBandsDirective& directive) {
    for (const double gain : settings.Gains) {
        directive.AddGains(gain);
    }
}

void BuildAdjustableDirective(const TEqualizerSettings& settings, TEqualizerCapability_TSetAdjustableEqualizerBandsDirective& directive) {
    static const std::array FREQUENCIES = {60, 230, 910, 3600, 14'000};
    static const std::array WIDTHS = {90, 340, 1340, 5200, 13'000};

    for (size_t i = 0; i < GAINS_LENGTH; ++i) {
        auto& band = *directive.AddBands();
        band.SetGain(settings.Gains[i]);
        band.SetFrequency(FREQUENCIES[i]);
        band.SetWidth(WIDTHS[i]);
    }
}

EEqualizerSupportStatus GetDeviceEqualizerSupportStatus(const TEnvironmentState& environmentState, TStringBuf deviceId,
                                                        TEqualizerCapability_EPresetMode presetMode)
{
    // should have Endpoint for device
    const auto* endpoint = NHollywood::FindEndpoint(environmentState, deviceId);
    if (!endpoint) {
        return EEqualizerSupportStatus::NotSupported;
    }

    // should have equalizer capability
    TEqualizerCapability capability;
    if (!NHollywood::ParseTypedCapability(capability, *endpoint)) {
        return EEqualizerSupportStatus::NotSupported;
    }

    // should have right preset mode
    if (capability.GetState().GetPresetMode() != presetMode) {
        return EEqualizerSupportStatus::NotSupported;
    }

    // OK if supports fixed directive
    if (SupportsDirective(capability, TCapability_EDirectiveType_SetFixedEqualizerBandsDirectiveType) &&
            capability.GetParameters().HasFixed()) {
        return EEqualizerSupportStatus::Fixed;
    }

    // OK if supports adjustable directive
    if (SupportsDirective(capability, TCapability_EDirectiveType_SetAdjustableEqualizerBandsDirectiveType) &&
            capability.GetParameters().HasAdjustable()) {
        return EEqualizerSupportStatus::Adjustable;
    }

    // Not OK
    return EEqualizerSupportStatus::NotSupported;
}

} // namespace

TMaybe<NScenarios::TDirective> TryBuildEqualizerBandsDirective(
    const TEnvironmentState& environmentState,
    TStringBuf seed,
    TStringBuf deviceId,
    TEqualizerCapability_EPresetMode presetMode)
{
    const auto status = GetDeviceEqualizerSupportStatus(environmentState, deviceId, presetMode);
    if (status == EEqualizerSupportStatus::NotSupported) {
        return Nothing();
    }

    const auto& settings = FindEqualizerSettings(seed);

    NScenarios::TDirective directive;

    switch (status) {
        case EEqualizerSupportStatus::NotSupported:
            Y_UNREACHABLE();
        case EEqualizerSupportStatus::Fixed:
            BuildFixedDirective(settings, *directive.MutableSetFixedEqualizerBandsDirective());
            break;
        case EEqualizerSupportStatus::Adjustable:
            BuildAdjustableDirective(settings, *directive.MutableSetAdjustableEqualizerBandsDirective());
            break;
    }

    return directive;
}

} // namespace NAlice
