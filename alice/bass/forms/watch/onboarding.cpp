#include "onboarding.h"

#include <alice/bass/forms/context/context.h>

namespace NBASS{
namespace NWatch {

namespace {

TResultValue DoStartOnboarding(TStringBuf mode, TContext& ctx) {
    TSlot* slot = ctx.GetOrCreateSlot(TStringBuf("mode"), TStringBuf("string"));
    slot->Value.SetString(TStringBuilder() << TStringBuf("elari_watch_") << mode);
    return TResultValue();
}

} // namespace

TResultValue StartOnboarding(TContext& ctx, TStringBuf mode) {
    TSlot* number = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    number->Value.SetIntNumber(0);

    return DoStartOnboarding(mode, ctx);
}

TResultValue ContinueOnboarding(TContext& ctx) {
    TSlot* number = ctx.GetOrCreateSlot(TStringBuf("set_number"), TStringBuf("num"));
    ++number->Value.GetIntNumberMutable(); // VINS will cut remainder
    return TResultValue();
}

} // namespace NWatch
} // namespace NBASS
