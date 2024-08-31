#include "config.h"
#include "proto_fields_visitor.h"

#include <alice/megamind/library/config/protos/extensions.pb.h>
#include <google/protobuf/descriptor.pb.h>

#include <library/cpp/getoptpb/getoptpb.h>
#include <library/cpp/protobuf/util/pb_io.h>

#include <util/stream/file.h>
#include <util/folder/path.h>
#include <util/generic/yexception.h>
#include <util/stream/output.h>
#include <util/system/env.h>

#include <google/protobuf/text_format.h>

#include <cstdlib>

namespace NAlice {

namespace {

struct TFillEnv : NConfig::TProtoFieldsVisitor {
    void operator()(NProtoBuf::Message& message,
                    const NProtoBuf::FieldDescriptor& field,
                    const TString& /* path */) override {
        const auto& reflection = *message.GetReflection();

        const TString& key = field.options().GetExtension(NAlice::Env);
        if (!key)
            return;

        if (field.cpp_type() != google::protobuf::FieldDescriptor::CPPTYPE_STRING) {
            ythrow yexception() << "Failed to fill non-string config field " << field.name() << " from Env";
        }

        if (reflection.HasField(message, &field)) {
            Cerr << "Skipping to fill " << field.name() << " from env because it's already set" << Endl;
            return;
        }

        const auto value = GetEnv(key);
        reflection.SetString(&message, &field, value);
    }
};

template <typename TSourceList>
bool ValidateSources(const TSourceList& sources, IOutputStream& output, TStringBuf prefix = {}) {
    const auto* descr = sources.GetDescriptor();
    Y_ENSURE(descr, "Cannot get a proto descriptor!");
    const auto* reflection = sources.GetReflection();
    Y_ENSURE(reflection, "Cannot get a proto reflection!");

    bool hasErrors = false;
    for (int fieldIdx = 0; fieldIdx < descr->field_count(); ++fieldIdx) {
        const NProtoBuf::FieldDescriptor* field = descr->field(fieldIdx);
        if (reflection->HasField(sources, field)) {
            const auto& config = static_cast<const TConfig::TSource&>(reflection->GetMessage(sources, field));
            hasErrors |= ValidateSource(prefix + field->name(), config, output);
        }
    }
    return hasErrors;
}

bool ValidateSources(const TString& name, const TConfig::TScenarios::TConfig& config, IOutputStream& output) {
    if (config.HasHandlersConfig()) {
        return ValidateSources(config.GetHandlersConfig(), output, name + '/');
    }
    return false;
}

template <typename T>
void ValidateConfig(T& config) {
    NConfig::TCheckRequiredFieldsArePresent checker;
    TString path = "sourceObject";
    NConfig::TDfs dfs(config);
    dfs.VisitProtoFields(config, checker, path);
    if (checker.GetDefect()) {
        ythrow yexception() << "Some Required fields are missing";
    }
}

template <typename T>
T LoadConfigFromFile(const TString& path) {
    NProtoBuf::TextFormat::Parser parser;
    T config;
    const auto data = TFileInput(path).ReadAll();
    Cerr << "LoadConfigFromFile data: " << data << Endl;
    Y_ENSURE(parser.ParseFromString(data, &config), "ParseFromString failed on Parse for " << path);
    return config;
}

} // namespace

bool ValidateSource(TStringBuf sourceName, const TConfig::TSource& config, IOutputStream& output) {
    auto maxAttempts = GetMaxAttempts(config);
    if (maxAttempts <= 1) {
        return false;
    }
    if (!config.HasRetryPeriodMs()) {
        output << "Retry period was not set for " << sourceName << " retriable source" << Endl;
        return true;
    }

    auto timeout = config.GetTimeoutMs();
    auto retryPeriod = config.GetRetryPeriodMs();
    if (retryPeriod * (maxAttempts - 1) >= timeout) {
        output << "Retry period for " << sourceName
               << " source is too large for the specified timeout, some requests will never be sent" << Endl;
        return true;
    }
    if (config.GetEnableFastReconnect() && maxAttempts != 1) {
        output << "EnableFastReconnect won't work for " << sourceName << ", it only works when MaxAttempts=1" << Endl;
        return true;
    }
    return false;
}

bool ValidateSources(TConfig& config, IOutputStream& output) {
    bool hasErrors = false;
    if (config.HasScenarios()) {
        const auto& scenarios = config.GetScenarios();
        if (scenarios.HasDefaultConfig()) {
            hasErrors |= ValidateSources("Default", scenarios.GetDefaultConfig(), output);
        }
        for (const auto& [name, scenarioConfig] : scenarios.GetConfigs()) {
            hasErrors |= ValidateSources(name, scenarioConfig, output);
        }
    }
    return hasErrors;
}

NMegamind::TClassificationConfig LoadClassificationConfig(const TString& path) {
    auto config = LoadConfigFromFile<NMegamind::TClassificationConfig>(path);
    ValidateConfig(config);
    return config;
}

TConfig LoadConfig(int argc, const char** argv) {
    TString errorMsg;
    TConfig config;
    NGetoptPb::TGetoptPbSettings settings{};

    NGetoptPb::TGetoptPb configPathOpt(settings);
    configPathOpt.AddOptions(config);
    Y_ENSURE(configPathOpt.ParseArgs(argc, argv, config, errorMsg),
             "Can not parse command line options and/or prototext config, explanation: " << errorMsg);

    const TFsPath pathToConfig = configPathOpt.GetOptsParseResult().Get(settings.ConfPathLong);
    const TFsPath configFolderPath = pathToConfig.Dirname();

    if (config.GetScenarioConfigsPath().empty()) {
        config.SetScenarioConfigsPath(configFolderPath / "scenarios");
    } else if (!TFsPath(config.GetScenarioConfigsPath()).Exists()) {
        config.SetScenarioConfigsPath(configFolderPath / config.GetScenarioConfigsPath());
    }
    if (config.GetClassificationConfigPath().empty()) {
        config.SetClassificationConfigPath(TFsPath(configFolderPath).Parent() / "common" / "classification.pb.txt");
    } else if (!TFsPath(config.GetClassificationConfigPath()).Exists()) {
        config.SetClassificationConfigPath(configFolderPath / config.GetClassificationConfigPath());
    }
    if (config.GetCombinatorConfigsPath().empty()) {
        config.SetCombinatorConfigsPath(configFolderPath / "combinators");
    } else if (!TFsPath(config.GetCombinatorConfigsPath()).Exists()) {
        config.SetCombinatorConfigsPath(configFolderPath / config.GetCombinatorConfigsPath());
    }

    if (settings.DumpConfig) {
        Cerr << "Using settings:\n"
             << "====================\n";
        configPathOpt.DumpMsg(config, Cerr);
        Cerr << "====================\n";
    }

    NConfig::TCheckRequiredFieldsArePresent checker;
    TString path = "sourceObject";
    NConfig::TDfs dfs(config);
    dfs.VisitProtoFields(config, checker, path);
    if (checker.GetDefect()) {
        ythrow yexception() << "Some Required fields are missing";
    }

    if (ValidateSources(config, Cerr)) {
        ythrow yexception() << "Incorrect retry settings have been found";
    }

    TFillEnv fillerEnv;
    dfs.VisitProtoFields(config, fillerEnv, path);

    // For vins-like logs
    if (config.GetClusterType() == "megamind_man" ||
        config.GetClusterType() == "megamind_sas" ||
        config.GetClusterType() == "megamind_vla")
    {
        config.SetClusterType("stable");
    }

    return config;
}

const TString* GetCurrentDC(const TConfig& config) {
    if (!config.HasCurrentDC()) {
        return nullptr;
    }

    const TString& currentDc = config.GetCurrentDC();
    return currentDc.Empty() ? nullptr : &currentDc;
}


} // namespace NAlice
