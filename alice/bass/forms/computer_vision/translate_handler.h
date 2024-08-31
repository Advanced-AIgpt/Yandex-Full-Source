#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {
    class TCVAnswerTranslate: public IComputerVisionAnswer {
    public:
        bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

        static inline TStringBuf ForceName() {
            return "computer_vision_force_translate";
        }

        TStringBuf GetAnswerId() const override {
            return TStringBuf("translate");
        }

        void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

    protected:
        void Compose(TComputerVisionContext& ctx) const override;
        ECaptureMode AnswerType() const override {
            return ECaptureMode::TRANSLATE;
        }
    };

    class TComputerVisionTranslateHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionTranslateHandler();

        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this_translate");
        }

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_translate");
        }
    };
}
