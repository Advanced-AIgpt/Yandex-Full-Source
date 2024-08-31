#pragma once

#include <alice/megamind/library/stack_engine/protos/stack_engine.pb.h>

#include <utility>

namespace NAlice::NMegamind {

class IStackEngine {
public:
    using TItem = TStackEngineCore::TItem;

    virtual ~IStackEngine() = default;

    [[nodiscard]] virtual bool IsEmpty() const = 0;
    [[nodiscard]] virtual const TItem& Peek() const = 0;
    virtual TItem Pop() = 0;
    virtual void PopScenario(const TString& scenarioName) = 0;
    virtual void Push(TItem&& item) = 0;
    virtual void Push(const TItem& item) = 0;
    [[nodiscard]] virtual bool IsUpdated() const = 0;
    virtual void StartNewSession(const TString& requestId, const TString& productScenarioName,
                                 const TString& scenarioName) = 0;
    [[nodiscard]] virtual const TString& GetSessionId() const = 0;
    [[nodiscard]] virtual const TString& GetProductScenarioName() const = 0;

    virtual const TStackEngineCore& GetCore() const = 0;
    virtual TStackEngineCore ReleaseCore() && = 0;

    [[nodiscard]] virtual bool IsStackOwner(const TString& scenarioName) const = 0;
};

class TStackEngine final : public IStackEngine {
public:
    explicit TStackEngine(TStackEngineCore core = {})
        : Core(std::move(core)) {
    }

    [[nodiscard]] bool IsEmpty() const override {
        return Core.GetItems().empty();
    }

    [[nodiscard]] const TItem& Peek() const override {
        return *Core.GetItems().rbegin();
    }

    TItem Pop() override {
        auto v = *Core.GetItems().rbegin();
        Core.MutableItems()->RemoveLast();
        Core.SetIsUpdated(true);
        return v;
    }

    void PopScenario(const TString& scenarioName) override {
        while (!IsEmpty() && Peek().GetScenarioName() == scenarioName) {
            Pop();
        }
    }

    void Push(TItem&& item) override {
        *Core.MutableItems()->Add() = std::move(item);
        Core.SetIsUpdated(true);
    }

    void Push(const TItem& item) override {
        *Core.MutableItems()->Add() = item;
        Core.SetIsUpdated(true);
    }

    [[nodiscard]] bool IsUpdated() const override {
        return Core.GetIsUpdated();
    }

    void StartNewSession(const TString& requestId, const TString& productScenarioName,
                         const TString& scenarioName) override {
        Core.ClearSessionId();
        Core.MutableItems()->Clear();
        Core.SetSessionId(requestId);
        Core.SetProductScenarioName(productScenarioName);
        Core.SetStackOwner(scenarioName);
    }

    [[nodiscard]] const TString& GetSessionId() const override {
        return Core.GetSessionId();
    }

    [[nodiscard]] const TString& GetProductScenarioName() const override {
        return Core.GetProductScenarioName();
    }

    [[nodiscard]] const TStackEngineCore& GetCore() const override {
        return Core;
    }

    TStackEngineCore ReleaseCore() && override {
        return std::move(Core);
    }

    [[nodiscard]] bool IsStackOwner(const TString& scenarioName) const override {
        const auto& owner = Core.GetStackOwner();
        if (Y_UNLIKELY(owner.Empty())) { // Usually scenario name is not filled in tests.
            return false;
        }
        return owner == scenarioName;
    }

private:
    TStackEngineCore Core;
};

} // namespace NAlice::NMegamind
