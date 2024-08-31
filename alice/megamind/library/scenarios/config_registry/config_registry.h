#pragma once

#include <alice/megamind/library/config/scenario_protos/combinator_config.pb.h>
#include <alice/megamind/library/config/scenario_protos/config.pb.h>

#include <library/cpp/resource/resource.h>

#include <google/protobuf/text_format.h>
#include <google/protobuf/util/field_mask_util.h>

#include <util/folder/iterator.h>
#include <util/folder/path.h>
#include <util/generic/hash.h>
#include <util/generic/strbuf.h>
#include <util/stream/file.h>

namespace NAlice {

template <typename TConfigProto>
TConfigProto ParseConfig(const TString& data) {
    NProtoBuf::TextFormat::Parser parser;
    TConfigProto config;
    Y_ENSURE(parser.ParseFromString(data, &config), "ParseFromString failed on Parse for " << data);
    return config;
}

THashMap<TString, TScenarioConfig> LoadScenarioConfigs(const TString& folderPath, bool validateConfigs = false);
THashMap<TString, NMegamind::TCombinatorConfigProto> LoadCombinatorConfigs(const TString& folderPath, bool validateConfigs = false);

class TScenarioConfigRegistry {
public:
    void AddScenarioConfig(const TScenarioConfig& config);

    [[nodiscard]] const THashMap<TString, TScenarioConfig>& GetScenarioConfigs() const;

    [[nodiscard]] const TScenarioConfig& GetScenarioConfig(const TString& scenarioName) const;

private:
    THashMap<TString, TScenarioConfig> ScenarioConfigs;
};

class TCombinatorConfig {
public:
    TCombinatorConfig() = default;

    TCombinatorConfig(NMegamind::TCombinatorConfigProto config)
        : ConfigProto{config}
    {}

    const TString& GetName() const {
        return ConfigProto.GetName();
    }

    bool GetEnabled() const {
        return ConfigProto.GetEnabled();
    }

    bool GetAcceptsAllScenarios() const {
        return ConfigProto.GetAcceptsAllScenarios();
    }

    bool GetAcceptsAllFrames() const {
        return ConfigProto.GetAcceptsAllFrames();
    }

    const google::protobuf::RepeatedPtrField<TString>& GetAcceptedScenarios() const {
        return ConfigProto.GetAcceptedScenarios();
    }

    const google::protobuf::RepeatedPtrField<TString>& GetAcceptedFrames() const {
        return ConfigProto.GetAcceptedFrames();
    }

    const NMegamind::TResponsibles& GetResponsibles() const {
        return ConfigProto.GetResponsibles();
    }

    static const TCombinatorConfig& GetDefaultInstance() {
        return *Singleton<TCombinatorConfig>(NMegamind::TCombinatorConfigProto::default_instance());
    }

private:
    NMegamind::TCombinatorConfigProto ConfigProto;
};

class TCombinatorConfigRegistry {
public:
    void AddCombinatorConfig(const TCombinatorConfig& config);

    [[nodiscard]] const THashMap<TString, TCombinatorConfig>& GetCombinatorConfigs() const;

    [[nodiscard]] const TCombinatorConfig& GetCombinatorConfig(const TString& combinatorName) const;

private:
    THashMap<TString, TCombinatorConfig> CombinatorConfigs;
};

} // namespace NAlice
