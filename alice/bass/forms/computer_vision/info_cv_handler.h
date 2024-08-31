#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {
class TCVAnswerInfo: public IComputerVisionAnswer {
public:
    TCVAnswerInfo();

    bool TryApplyTo(TComputerVisionContext& ctx, bool force = false, bool shouldAttachAlternativeIntents = true) const override;

    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

    static inline TStringBuf ForceName() {
        return TStringBuf("computer_vision_force_info");
    };

    TStringBuf GetAnswerId() const override {
        return TStringBuf("info");
    }

protected:
    bool IsSuggestible(const TComputerVisionContext& ) const override {
        return true;
    }

    void Compose(TComputerVisionContext& ctx) const override;
};

class TComputerVisionEllipsisInfoHandler : public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisInfoHandler();

    static void Register(THandlersMap* handlers);

    static inline TStringBuf FormShortName() {
        return TStringBuf("image_what_is_this__info");
    };

    static inline TStringBuf FormName() {
        return TStringBuf("personal_assistant.scenarios.image_what_is_this__info");
    };

protected:
    TResultValue WrappedDo(TComputerVisionContext& cvContext) const;

protected:
    TCVAnswerInfo AnswerInfo;
};
}
