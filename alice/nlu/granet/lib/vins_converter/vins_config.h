#pragma once

#include <util/folder/path.h>
#include <util/generic/map.h>
#include <util/generic/set.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NGranet {
namespace NVinsConfig {

struct TVinsConfig;
struct TProjectConfig;
struct TEntityConfig;
struct TIntentConfig;
struct TNluConfig;

// ~~~~ TVinsConfig ~~~~

struct TVinsConfig {
    // Path to VINS main config file.
    // Example: alice/vins/apps/personal_assistant/personal_assistant/config/Vinsfile.json
    TString Path;

    // Map from: name of template
    //       to: path to dictionary txt-file
    // Example: launch -> alice/vins/apps/personal_assistant/personal_assistant/config/nlu_templates/music/launch.txt
    TMap<TString, TString> CustomTemplatesPaths;

    // Map from: name of project (example: scenarios)
    //       to: project config (in case of scenarios read from:
    //           alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json)
    TMap<TString, TProjectConfig> Projects;
};

// ~~~~ TProjectConfig ~~~~

struct TProjectConfig {
    // Project name.
    // Example: scenarios
    TString Name;

    // Path to config of project.
    // Example: alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json
    TString Path;

    // Map from: name of entity (example: mood)
    //       to: config of entity
    // Source: VinsProjectfile.json > entities
    TMap<TString, TEntityConfig> Entities;

    // Map from: name of intent (example: music_play)
    //       to: config of intent
    // Source: VinsProjectfile.json > intents
    TMap<TString, TIntentConfig> Intents;
};

// ~~~~ TEntityConfig ~~~~

struct TEntityConfig {
    // Name of entity.
    // Example: mood
    // Source: VinsProjectfile.json > entities > entity
    TString Name;

    // Path to config of entity.
    // Example: alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/entities/music_ex/mood.json
    // Source: VinsProjectfile.json > entities > path
    TString Path;

    // Source: VinsProjectfile.json > entities > inflect_numbers
    bool IsNeedInflectNumbers = false;
};

// ~~~~ TIntentConfig ~~~~

struct TIntentConfig {
    // Name of intent.
    // Example: music_play
    // Source: VinsProjectfile.json > intents > intent
    TString Name;

    // Path to json-config of intent.
    // Example: alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/intents/music/music_play.json
    // Source: VinsProjectfile.json > intents > dm > path
    TString DmPath;

    // Map from: relative path to nlu-file from VinsProjectfile.json (example: intents/music/music_play.nlu)
    //       to: config of nlu-dictionary
    TVector<TNluConfig> Nlus;

    // Name of form.
    // Example: music_play
    TString Form;

    // Slots of form.
    // Source: field 'slots' of (for example):
    //   alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/intents/music/music_play.json
    TVector<TString> Slots;
};

// ~~~~ TNluConfig ~~~~

// Source: field 'intents' > 'nlu' of (for example):
//   alice/vins/apps/personal_assistant/personal_assistant/config/scenarios/VinsProjectfile.json
struct TNluConfig {
    TString RelativePath;
    TString FullPath;
    bool CanUseToTrainTagger = true;
};

// ~~~~ Read config ~~~~

TVinsConfig ReadVinsConfig(const TFsPath& vinsConfigPath);

} // namespace NVinsConfig
} // namespace NGranet
