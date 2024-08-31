#include "experiments.h"
#include <library/cpp/json/json_reader.h>
#include <util/stream/file.h>

namespace NVoice::NExperiments {

namespace {

NJson::TJsonValue ReadJson(const TString& fileName)
{
    TFileInput input(fileName);
    return NJson::ReadJsonTree(&input, /*throwOnError = */ true);
}

} // anonymous namespace


TExperiments::TExperiments(const TConfig& cfg)
    : TExperiments(
        ReadJson(cfg.ExperimentsFileName),
        ReadJson(cfg.MacrosFileName)
    )
{ }

TExperiments::TExperiments(IInputStream& experimentsDataIn, IInputStream& macrosDataIn)
    : TExperiments(
        NJson::ReadJsonTree(&experimentsDataIn, /*throwOnError = */ true),
        NJson::ReadJsonTree(&macrosDataIn, /*throwOnError = */ true)
    )
{ }

TExperiments::TExperiments(const NJson::TJsonValue& experimentsJson, const NJson::TJsonValue& macrosJson)
    : ExperimentsStorage(experimentsJson, macrosJson)
{ }

TEventPatcher TExperiments::CreatePatcherForSession(const NAliceProtocol::TSessionContext& context, const NJson::TJsonValue& event) const
{
    return ExperimentsStorage.CreateEventPatcher(event, context);
}

}  // namespace NVoice::NExperiments
