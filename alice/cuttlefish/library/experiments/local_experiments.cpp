#include "flags_json.h"
#include "local_experiments.h"
#include "patch_functions.h"
#include "session_context_proxy.h"
#include "utils.h"

#include <library/cpp/json/json_reader.h>

#include <util/digest/murmur.h>
#include <util/generic/guid.h>
#include <util/generic/map.h>
#include <util/generic/yexception.h>
#include <util/random/common_ops.h>
#include <util/stream/file.h>


namespace NVoice::NExperiments {

namespace {

TVector<TExpPatch> CreatePatches(const TExpContext& expContext, const NJson::TJsonValue::TMapType& expDesc)
{
    TVector<TExpPatch> patches;
    for (const NJson::TJsonValue& patchJson : expDesc.at("flags").GetArraySafe()) {
        patches.push_back(NExperiments::TExpPatch(expContext, patchJson.GetArraySafe()));
    }
    return patches;
}

void AddExperimentPatches(TVector<const TExpPatch*>& patches, const TLocalExperiment& experiment)
{
    for (const TExpPatch& p : experiment.GetPatches())
        patches.push_back(&p);
}

TString GetIdFromDesc(const NJson::TJsonValue::TMapType& expDesc)
{
    if (const auto* node = expDesc.FindPtr("id"))
        return node->GetStringSafe();
    return CreateGuidAsString();
}

double GetShareFomDesc(const NJson::TJsonValue::TMapType& expDesc)
{
    double share = 1.0;
    if (const auto* node = expDesc.FindPtr("share")) {
        share = node->GetDoubleSafe();
        Y_ENSURE((share >= 0.0 && share <= 1.0), "Share MUST be in [0, 1]");
    }
    return share;
}

} // anonymous namespace


TLocalExperiment::TLocalExperiment(const TExpContext& expContext, const NJson::TJsonValue::TMapType& expDesc)
    : Id(GetIdFromDesc(expDesc))
    , Patches(CreatePatches(expContext, expDesc))
    , Hash(MurmurHash<uint64_t>(Id.data(), Id.size()))
    , Share(GetShareFomDesc(expDesc))
{ }

double TLocalExperiment::GetRandom(TStringBuf uuid) const
{
    const uint64_t hash = MurmurHash<uint64_t>(uuid.data(), uuid.size(), Hash);
    const double val = ::NPrivate::ToRandReal4(hash);
    DLOG("Random value for UUID=" << uuid << " ExpId=" << Id << " is " << val);
    return val;
}


TLocalExperiments::TLocalExperiments(const NJson::TJsonValue& experimentsJson, const NJson::TJsonValue& macrosJson)
    : Context{macrosJson.GetMapSafe()}
{
    for (const NJson::TJsonValue& expJson : experimentsJson.GetArraySafe()) {
        const NJson::TJsonValue::TMapType& expDesc = expJson.GetMapSafe();
        Y_ENSURE(expDesc.find("pool") == expDesc.end());
        Y_ENSURE(expDesc.find("control_share") == expDesc.end());

        Experiments.emplace_back(Context, expDesc);
    }
}

TEventPatcher TLocalExperiments::CreateEventPatcher(
    const NJson::TJsonValue& initialEvent,
    const NAliceProtocol::TSessionContext& sessionContext
) const
{
    TVector<const TExpPatch*> patches = SelectLocalExperimentsForSession(sessionContext, initialEvent);
    DLOG("Use " << patches.size() << " common patches for a session");
    return TEventPatcher(std::move(patches));
}

TVector<const TExpPatch*> TLocalExperiments::SelectLocalExperimentsForSession(
    const NAliceProtocol::TSessionContext& sessionContext,
    const NJson::TJsonValue&
) const
{
    TVector<const TExpPatch*> patches;

    const TStringBuf uuid = GetUuid(sessionContext);
    for (const TLocalExperiment& exp : Experiments) {
        if (!exp.IsUsable(uuid))
            continue;
        AddExperimentPatches(patches, exp);
    }

    return patches;
}

} // namespace NVoice::NExperiments
