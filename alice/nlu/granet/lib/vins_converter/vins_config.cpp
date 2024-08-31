#include "vins_config.h"
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <library/cpp/json/json_reader.h>
#include <library/cpp/json/json_value.h>
#include <util/stream/file.h>

using namespace NJson;

namespace NGranet {
namespace NVinsConfig {

// forward declarations
TProjectConfig ReadProjectConfig(const TFsPath& basePath, const TFsPath& projectPath);

TVinsConfig ReadVinsConfig(const TFsPath& vinsConfigPath) {
    const TFsPath basePath = vinsConfigPath.Parent().Parent().Parent();

    TVinsConfig vinsConfig;
    vinsConfig.Path = vinsConfigPath;

    TFileInput inputStream(vinsConfigPath);
    const TJsonValue rootJson = ReadJsonTree(&inputStream, true);

    for (const auto& templateJson : rootJson["nlu"]["custom_templates"].GetMap()) {
        const TString name = templateJson.first;
        const TString path = basePath / templateJson.second.GetStringSafe();
        vinsConfig.CustomTemplatesPaths[name] = path;
    }

    for (const TJsonValue& projectJson : rootJson["project"]["includes"].GetArray()) {
        if (projectJson["type"].GetStringSafe() != "file") {
            continue;
        }
        TFsPath projectPath = projectJson["path"].GetStringSafe();
        projectPath = basePath / projectPath;
        TProjectConfig project = ReadProjectConfig(basePath, projectPath);
        const TString name = project.Name;
        vinsConfig.Projects[name] = std::move(project);
    }
    return vinsConfig;
}

TProjectConfig ReadProjectConfig(const TFsPath& basePath, const TFsPath& projectPath) {
    TProjectConfig project;
    project.Path = projectPath;

    TFileInput projectFile(projectPath);
    const TJsonValue projectJson = ReadJsonTree(&projectFile, true);

    project.Name = projectJson["name"].GetStringSafe();

    for (const TJsonValue& entityJson : projectJson["entities"].GetArray()) {
        TEntityConfig entity;
        entity.Name = entityJson["entity"].GetStringSafe();
        entity.Path = basePath / entityJson["path"].GetStringSafe();
        entity.IsNeedInflectNumbers = entityJson["inflect_numbers"].GetBooleanSafe(false);
        project.Entities[entity.Name] = entity;
    }

    for (const TJsonValue& intentJson : projectJson["intents"].GetArray()) {
        TIntentConfig intent;
        intent.Name = intentJson["intent"].GetStringSafe("");
        intent.DmPath = intentJson["dm"]["path"].GetStringSafe("");
        if (intent.Name.empty() || intent.DmPath.empty()) {
            continue;
        }
        intent.DmPath = basePath / intent.DmPath;
        for (const TJsonValue& nluJson : intentJson["nlu"].GetArray()) {
            if (nluJson["source"].GetStringSafe() != "file") {
                continue;
            }
            TNluConfig nlu;
            nlu.RelativePath = nluJson["path"].GetStringSafe();
            nlu.FullPath = basePath / nlu.RelativePath;
            nlu.CanUseToTrainTagger = nluJson["can_use_to_train_tagger"].GetBooleanSafe(true);
            intent.Nlus.push_back(nlu);
        }

        TJsonValue dmJson = NJsonUtils::ReadJsonFileVerbose(intent.DmPath);
        intent.Form = dmJson["form"].GetStringSafe();
        for (const TJsonValue& slotJson : dmJson["slots"].GetArray()) {
            const TString slot = slotJson["slot"].GetStringSafe("");
            if (!slot.empty()) {
                intent.Slots.push_back(slot);
            }
        }

        const TString name = intent.Name;
        project.Intents[name] = std::move(intent);
    }
    return project;
}

} // namespace NVinsConfig
} // namespace NGranet
