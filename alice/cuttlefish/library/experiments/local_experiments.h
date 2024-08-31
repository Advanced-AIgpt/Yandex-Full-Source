#pragma once
#include <alice/cuttlefish/library/experiments/event_patcher.h>


namespace NVoice::NExperiments {


class TLocalExperiment {
public:
    TLocalExperiment(const TExpContext& expContext, const NJson::TJsonValue::TMapType& expDesc);

    inline bool IsUsable(TStringBuf uuid) const {
        if (Share == 0)
            return false;
        return (Share == 1.0) || (GetRandom(uuid) < Share);
    }

    inline const TVector<TExpPatch>& GetPatches() const {
        return Patches;
    }
    inline TStringBuf GetId() const {
        return Id;
    }

private:
    double GetRandom(TStringBuf uuid) const;

    TString Id;
    TVector<TExpPatch> Patches;
    const uint64_t Hash;
    const double Share;
};


class TLocalExperiments {
public:
    TLocalExperiments(const NJson::TJsonValue& experimentsJson, const NJson::TJsonValue& macrosJson);

    TEventPatcher CreateEventPatcher(const NJson::TJsonValue& initEvent, const NAliceProtocol::TSessionContext& context) const;

private:
    TVector<const TExpPatch*> SelectLocalExperimentsForSession(
        const NAliceProtocol::TSessionContext& sessionContext,
        const NJson::TJsonValue& initialEvent
    ) const;

    const TExpContext Context;
    TVector<TLocalExperiment> Experiments;
};


} // namespace NVoice::NExperiments
