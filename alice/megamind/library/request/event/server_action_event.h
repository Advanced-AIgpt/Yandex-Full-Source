#pragma once

#include <alice/megamind/library/request/event/event.h>

#include <library/cpp/json/json_value.h>

#include <google/protobuf/struct.pb.h>

#include <util/generic/hash_set.h>
#include <util/generic/string.h>

namespace NAlice {

constexpr auto MM_STACK_ENGINE_GET_NEXT_CALLBACK_NAME = "@@mm_stack_engine_get_next";
constexpr auto MM_STACK_ENGINE_SESSION_ID = "stack_session_id";
constexpr auto MM_STACK_ENGINE_PRODUCT_SCENARIO_NAME = "stack_product_scenario_name";
constexpr auto MM_STACK_ENGINE_MEMENTO_KEY = "__megamind__@stack_engine";
constexpr auto MM_STACK_ENGINE_RECOVERY_CALLBACK_FIELD_NAME = "@recovery_callback";

constexpr auto MM_CALLBACK_PARENT_PRODUCT_SCENARIO_NAME = "@parent_product_scenario_name";

enum class ECallbackType {
    None,
    OnSuggest,
    OnCardAction,
    OnExternalButton,
    SemanticFrame,
    GetNext,
};

class TServerActionEvent: public IEvent {
public:
    using IEvent::IEvent;

    const TServerActionEvent* AsServerActionEvent() const override {
        return this;
    }

    void FillScenarioInput(const TMaybe<TString>& normalizedUtterance, NScenarios::TInput* input) const override;

    ECallbackType GetCallbackType() const;

    const TString& GetName() const;

    const google::protobuf::Struct& GetPayload() const;
};

} // namespace NAlice
