#pragma once

#include <alice/hollywood/library/base_hw_service/base_hw_service_handle.h>

namespace NAlice::NHollywood::NResponseMerger {

enum class EDirectiveType {
    ShowView,
    AddCard,
    SetMainScreen,
    SetUpperShutter,
    PatchView
};

inline constexpr TStringBuf RENDERED_RESPONSE_ITEM = "rendered_response_item";
inline constexpr TStringBuf RUN_RESPONSE_ITEM = "mm_scenario_run_response";
inline constexpr TStringBuf CONTINUE_RESPONSE_ITEM = "mm_scenario_continue_response";
inline constexpr TStringBuf APPLY_RESPONSE_ITEM = "mm_scenario_apply_response";

class TResponseMergerHandle : public IHwServiceHandle {
public:
    void Do(THwServiceContext& ctx) const override;
    const TString& Name() const override;
};

} // namespace NAlice::NHollywood::NResponserMerger
