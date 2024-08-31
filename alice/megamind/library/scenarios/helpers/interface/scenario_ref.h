#pragma once

#include <alice/megamind/library/scenarios/interface/scenario.h>

#include <util/generic/ptr.h>
#include <util/generic/vector.h>

namespace NAlice {

class TConfigBasedAppHostProxyProtocolScenario;
class TConfigBasedAppHostPureProtocolScenario;
class TConfigBasedHttpProtocolScenario;
class TEffectfulScenario;
class THeavyScenario;


class IScenarioVisitor {
public:
    virtual ~IScenarioVisitor() = default;

    virtual void Visit(const TConfigBasedAppHostProxyProtocolScenario& scenario) const = 0;
    virtual void Visit(const TConfigBasedAppHostPureProtocolScenario& scenario) const = 0;
};

class IScenarioRef : public TThrRefBase {
public:
    virtual void Accept(const IScenarioVisitor& visitor) const = 0;
    virtual const TScenario& GetScenario() const = 0;
};

using TScenarioRefs = TVector<TIntrusivePtr<IScenarioRef>>;

} // namespace NAlice
