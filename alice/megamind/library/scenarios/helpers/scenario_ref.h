#pragma once

#include <alice/megamind/library/scenarios/helpers/interface/scenario_ref.h>

#include <alice/megamind/library/scenarios/interface/protocol_scenario.h>
#include <alice/megamind/library/scenarios/protocol/protocol_scenario.h>

#include <util/generic/ptr.h>

#include <functional>

namespace NAlice {

template<typename TScenarioType>
class TScenarioRef : public IScenarioRef {
public:
    explicit TScenarioRef(const TScenarioType& scenario)
        : Scenario(scenario)
    {
    }

    void Accept(const IScenarioVisitor& visitor) const override {
        visitor.Visit(Scenario);
    }

    const TScenario& GetScenario() const override {
        return Scenario;
    }

private:
    const TScenarioType& Scenario;
};

class TScenarioVisitorBase : public IScenarioVisitor {
public:

    void Visit(const TConfigBasedAppHostProxyProtocolScenario& /* scenario */) const override {}
    void Visit(const TConfigBasedAppHostPureProtocolScenario& /* scenario */) const override {}
};

template<typename ...TScenarioTypes>
class TOnlyVisit;

template<typename TScenarioType>
class TOnlyVisit<TScenarioType> : public TScenarioVisitorBase {
public:
    explicit TOnlyVisit(std::function<void(const TScenarioType& scenario)> fn)
        : Fn(std::move(fn))
    {
    }

    void Visit(const TScenarioType& scenario) const override {
        Fn(scenario);
    }

private:
    std::function<void(const TScenarioType& scenario)> Fn;
};

template<typename TScenarioType, typename ...TOtherScenarioTypes>
class TOnlyVisit<TScenarioType, TOtherScenarioTypes...> : public TOnlyVisit<TOtherScenarioTypes...> {
public:
    using TOnlyVisit<TOtherScenarioTypes...>::Visit;

    TOnlyVisit(
        std::function<void(const TScenarioType& scenario)> fn,
        std::function<void(const TOtherScenarioTypes& scenario)>... otherFns
    )
        : TOnlyVisit<TOtherScenarioTypes...>(otherFns...)
        , Fn(std::move(fn))
    {
    }

    void Visit(const TScenarioType& scenario) const override {
        Fn(scenario);
    }

private:
    std::function<void(const TScenarioType& scenario)> Fn;
};

template<typename ...TScenarioTypes>
void OnlyVisit(TIntrusivePtr<IScenarioRef> ref, std::function<void(const TScenarioTypes&)>... fns) {
    ref->Accept(TOnlyVisit<TScenarioTypes...>(fns...));
}

} // namespace NAlice
