#pragma once

#include <alice/bass/forms/context/context.h>

#include <alice/bass/libs/logging_v2/logger.h>

namespace NBASS {

struct TInvalidParamException : public yexception {};

inline TSlot* GetSlotTyped(TContext& ctx, TStringBuf name, NSc::TValue::EType type) {
    auto* const slot = ctx.GetSlot(name);

    if (IsSlotEmpty(slot))
        return slot;

    if (slot->Value.GetType() != type) {
        // throw instead of ythrow because there's no need in exact
        // exception location knowledge.
        TStringBuilder msg;
        msg << "Slot " << name << " is of invalid type, expected: " << type << ", actual: " << slot->Value.GetType();
        throw TInvalidParamException() << msg;
    }

    return slot;
}

inline void NullifySlot(TSlot* slot) {
    if (IsSlotEmpty(slot))
        return;
    slot->Value.SetNull();
}

inline void SetError(TContext& ctx, const TStringBuf& message, bool addErrorBlock) {
    LOG(ERR) << "SetError with message: " << message << Endl;
    if (addErrorBlock) {
        ctx.AddErrorBlock(TError::EType::SYSTEM, message);
    }
}

inline void SetError(TContext& ctx, const TStringBuf& messagePrefix, TError error, bool addErrorBlock) {
    TStringBuilder message;
    message << messagePrefix << ", error type: " << error.Type;
    if (!error.Msg.empty())
        message << ", error message: " << error.Msg;
    SetError(ctx, message, addErrorBlock);
}

class TEchoHandler : public IHandler {
public:
    TResultValue Do(TRequestHandler& /* r */) override {
        return ResultSuccess();
    }
};

} // namespace NBASS
