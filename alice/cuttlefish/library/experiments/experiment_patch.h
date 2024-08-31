#pragma once
#include <alice/cuttlefish/library/protos/session.pb.h>
#include <library/cpp/json/json_value.h>


namespace NVoice::NExperiments {

struct TExpContext {
    const NJson::TJsonValue::TMapType Macros;
};


class IPatchFunction {
public:
    virtual ~IPatchFunction() = default;
    virtual bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& context) const = 0;
};


// Atomic patch for incoming events
class TExpPatch {
public:
    using TPatchFunctions = TVector<THolder<IPatchFunction>>;

public:
    TExpPatch(const TExpContext& expContext, TPatchFunctions&& funcs);
    TExpPatch(const TExpContext& expContext, const NJson::TJsonValue::TArray& array);

    bool Apply(NJson::TJsonValue& event, const NAliceProtocol::TSessionContext& context) const;

private:
    const TExpContext& ExpContext;
    TPatchFunctions Funcs;
};

} // namespace NVoice::NExperiments
