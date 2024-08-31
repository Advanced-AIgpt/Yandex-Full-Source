#include "switch_input.h"

#include <alice/bass/forms/directives.h>
#include <alice/bass/libs/client/experimental_flags.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <util/generic/algorithm.h>

namespace NBASS {

namespace {

constexpr TStringBuf TV_SWITCH_INPUT = "personal_assistant.scenarios.switch_tv_input";
constexpr TStringBuf TV_SWITCH_INPUT_ELLIPSIS = "personal_assistant.scenarios.switch_tv_input__ellipsis";

constexpr TStringBuf SOURCE_ID_SLOT = "source_id";
constexpr TStringBuf NUMBER_SLOT = "number";

bool ActivateSlotAskEvent(TStringBuf name, TContext& ctx) {
    TContext::TSlot* slot = ctx.GetOrCreateSlot(name, "string");
    if (slot->Value.IsNull()) {
        slot->Optional = false; // activate ask event for slot
        return true;
    }
    return false;
}

NSc::TValue GetTvSwitchInputPayload(TStringBuf name) {
    NSc::TValue payload;
    payload["name"] = name;
    return payload;
}

void MakeSwitch(TContext& ctx, TStringBuf id) {
    auto payload = GetTvSwitchInputPayload(id);
    ctx.AddCommand<TTvSwitchInputDirective>("tv_switch_input", payload);
    ctx.AddTextCardBlock("tv_switch_input_succesfully");
}

}

TResultValue TTvSwitchInputFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::TV_SWITCH_INPUT);
    if (!ctx.ClientFeatures().SupportsTvSourcesSwitching() || !ctx.HasExpFlag(EXPERIMENTAL_FLAG_SWITCH_TV_INPUT)) {
        ctx.AddAttention("tv_source_switching_not_supported");
        return TResultValue();
    }

    if (ActivateSlotAskEvent(SOURCE_ID_SLOT, ctx))
        return TResultValue();

    const auto& id = ctx.GetSlot(SOURCE_ID_SLOT)->Value.GetString();

    TVector<TStringBuf> candidates;
    for (const auto& input : ctx.Meta().DeviceState().TvSetState().Inputs()) {
        const auto& name = input.Name();
        if (!name.IsNull() && TString(name).StartsWith(id))
            candidates.push_back(name);
    }

    switch (candidates.size()) {
        case 0:
            ctx.AddTextCardBlock("tv_source_not_found");
            break;
        case 1:
            MakeSwitch(ctx, candidates[0]);
            break;
        default:
            if (ActivateSlotAskEvent(NUMBER_SLOT, ctx))
                return TResultValue();

            TString id = TString{ctx.GetSlot(SOURCE_ID_SLOT)->Value.GetString()};
            id += ToString(ctx.GetSlot(NUMBER_SLOT)->Value.GetNumber());

            if (FindPtr(candidates, id))
                MakeSwitch(ctx, id);
            else
                ctx.AddTextCardBlock("tv_source_not_found");
    }

    return TResultValue();
}

// static
void TTvSwitchInputFormHandler::Register(THandlersMap* handlers) {
    auto cbTvSwitchInputForm = []() {
        return MakeHolder<TTvSwitchInputFormHandler>();
    };
    handlers->emplace(TV_SWITCH_INPUT, cbTvSwitchInputForm);
    handlers->emplace(TV_SWITCH_INPUT_ELLIPSIS, cbTvSwitchInputForm);
}

} // namespace NBASS
