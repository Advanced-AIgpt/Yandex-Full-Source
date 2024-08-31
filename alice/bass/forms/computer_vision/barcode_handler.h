#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {
    class TCVAnswerBarcode: public IComputerVisionAnswer {
    public:
        bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

        static inline TStringBuf ForceName() {
            return "computer_vision_force_barcode";
        }

        TStringBuf GetAnswerId() const override {
            return TStringBuf("barcode");
        }

        void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

    protected:
        void Compose(TComputerVisionContext& ctx) const override;

        ECaptureMode AnswerType() const override {
            return ECaptureMode::BARCODE;
        }
    };

    class TComputerVisionBarcodeHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionBarcodeHandler();

        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this_barcode");
        }

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_barcode");
        }
    };
}
