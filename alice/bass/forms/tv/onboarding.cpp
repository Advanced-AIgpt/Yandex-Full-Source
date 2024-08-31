#include "onboarding.h"

#include <alice/bass/forms/context/context.h>

namespace NBASS::NTvCommon {

namespace {

constexpr TStringBuf MODE_SLOT_NAME = "mode";
constexpr TStringBuf MODE_SLOT_TYPE = "string";

constexpr TStringBuf NUMBER_SLOT_NAME = "set_number";
constexpr TStringBuf NUMBER_SLOT_TYPE = "num";

TResultValue DoStartOnboarding(TContext& ctx, const TStringBuf mode) {
    TSlot* slot = ctx.GetOrCreateSlot(MODE_SLOT_NAME, MODE_SLOT_TYPE);
    slot->Value.SetString(TString::Join("tv_", mode));
    return TResultValue();
}

} // namespace

TResultValue StartOnboarding(TContext& ctx, const TStringBuf mode) {
    TSlot* number = ctx.GetOrCreateSlot(NUMBER_SLOT_NAME, NUMBER_SLOT_TYPE);
    number->Value.SetIntNumber(0);
    return DoStartOnboarding(ctx, mode);
}

TResultValue ContinueOnboarding(TContext& ctx) {
    TSlot* number = ctx.GetOrCreateSlot(NUMBER_SLOT_NAME, NUMBER_SLOT_TYPE);
    ++number->Value.GetIntNumberMutable(); // VINS will cut remainder
    return TResultValue();
}

} // namespace NBASS::NTvCommon
