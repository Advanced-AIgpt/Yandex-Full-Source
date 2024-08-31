#include "config.h"

#include <alice/begemot/lib/fresh_options/fresh_options.h>
#include <alice/library/json/json.h>

#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_writer.h>
#include <library/cpp/json/writer/json_value.h>
#include <util/string/cast.h>

using namespace std::literals;

namespace NAlice {

static void AdjustSlotData(NJson::TJsonValue* config) {
    NJson::TJsonValue* frames = config->GetMapSafe().FindPtr("frames"sv);
    if (frames == nullptr) {
        return;
    }
    for (NJson::TJsonValue& frame : frames->GetArraySafe()) {
        NJson::TJsonValue* slots = frame.GetMapSafe().FindPtr("slots"sv);
        if (slots == nullptr) {
            continue;
        }
        for (NJson::TJsonValue& slot : slots->GetArraySafe()) {
            NJson::TJsonValue* value = slot.GetMapSafe().FindPtr("value_json"sv);
            if (value == nullptr) {
                continue;
            }
            slot["value"sv] = NJson::WriteJson(*value, false, true);
            slot.EraseValue("value_json"sv);
        }
    }
}

TTrivialTaggerConfig ReadTrivialTaggerConfigFromJsonString(TStringBuf str, bool permissive) {
    NJson::TJsonValue configJson = JsonFromString(str);
    AdjustSlotData(&configJson);
    return JsonToProto<TTrivialTaggerConfig>(configJson, true, permissive);
}

static THashMap<ELanguage, TTrivialTaggerConfig> LoadTrivialTaggerConfigs(const NBg::TFileSystem& fs) {
    THashMap<ELanguage, TTrivialTaggerConfig> configs;
    for (const TString& dir: fs.List()) {
        const ELanguage lang = LanguageByName(dir);
        if (lang == LANG_UNK) {
            continue;
        }
        const TFsPath path = TFsPath(dir) / "trivial_tagger.json";
        if (!fs.Exists(path)) {
            continue;
        }
        const TString configString = fs.LoadInputStream(path)->ReadAll();
        configs[lang] = ReadTrivialTaggerConfigFromJsonString(configString, true);
    }
    return configs;
}

THashMap<ELanguage, TTrivialTaggerConfig> LoadTrivialTaggerConfigs(const NBg::TFileSystem& fs, const TFsPath& dir) {
    if (!fs.Exists(dir)) {
        return {};
    }
    return LoadTrivialTaggerConfigs(*fs.Subdirectory(dir));
}

void Validate(const TTrivialTaggerConfig& config) {
    for (const TTrivialTaggerConfig::TFrame& frameConfig : config.GetFrames()) {
        Y_ENSURE_EX(!frameConfig.GetName().empty(), TTrivialTaggerConfigValidationException() << "Frame name is not defined");
        for (const TTrivialTaggerConfig::TSlot& slotConfig : frameConfig.GetSlots()) {
            Y_ENSURE_EX(!slotConfig.GetName().empty(), TTrivialTaggerConfigValidationException() << "Slot name is not defined");
            Y_ENSURE_EX(!slotConfig.GetType().empty(), TTrivialTaggerConfigValidationException() << "Slot type is not defined");
            Y_ENSURE_EX(!slotConfig.GetValue().empty(), TTrivialTaggerConfigValidationException() << "Slot value is not defined");
        }
    }
}

TTrivialTaggerConfig MergeTrivialTaggerConfigs(
    const TTrivialTaggerConfig* staticConfig,
    const TTrivialTaggerConfig* freshConfig,
    const TVector<TTrivialTaggerConfig>& configPatches,
    const TFreshOptions& freshOptions)
{
    TTrivialTaggerConfig resultConfig;
    for (const TTrivialTaggerConfig& configPatch : configPatches) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(configPatch.GetLanguage());
        }
        for (const TTrivialTaggerConfig::TFrame& frameConfig : configPatch.GetFrames()) {
            *resultConfig.AddFrames() = frameConfig;
        }
    }
    if (freshConfig != nullptr) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(freshConfig->GetLanguage());
        }
        for (const TTrivialTaggerConfig::TFrame& frameConfig : freshConfig->GetFrames()) {
            if (ShouldUseFreshForForm(freshOptions, frameConfig.GetName())) {
                *resultConfig.AddFrames() = frameConfig;
            }
        }
    }
    if (staticConfig != nullptr) {
        if (resultConfig.GetLanguage().empty()) {
            resultConfig.SetLanguage(staticConfig->GetLanguage());
        }
        for (const TTrivialTaggerConfig::TFrame& frameConfig : staticConfig->GetFrames()) {
            if (freshConfig == nullptr || !ShouldUseFreshForForm(freshOptions, frameConfig.GetName())) {
                *resultConfig.AddFrames() = frameConfig;
            }
        }
    }
    return resultConfig;
}

} // namespace NAlice
