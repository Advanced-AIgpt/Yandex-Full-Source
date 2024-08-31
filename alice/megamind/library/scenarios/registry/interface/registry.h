#pragma once

#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>

#include <util/generic/hash_set.h>
#include <util/generic/ptr.h>
#include <util/generic/string.h>
#include <util/generic/vector.h>

namespace NAlice {

class TConfigBasedAppHostProxyProtocolScenario;
class TConfigBasedAppHostPureProtocolScenario;
class TConfigBasedHttpProtocolScenario;

class IScenarioRegistry {
public:
    using TFramesToScenarios = THashMap<TString, TVector<TString>>;

    virtual ~IScenarioRegistry() = default;

    virtual void RegisterConfigBasedAppHostProxyProtocolScenario(THolder<TConfigBasedAppHostProxyProtocolScenario> scenario) = 0;
    virtual void RegisterConfigBasedAppHostPureProtocolScenario(THolder<TConfigBasedAppHostPureProtocolScenario> scenario) = 0;

    virtual const TFramesToScenarios& GetFramesToScenarios() const = 0;
};

} // namespace NAlice
