#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {


    class TCVAnswerOfficeLens: public IComputerVisionAnswer {
    public:
        TCVAnswerOfficeLens();

        bool TryApplyTo(TComputerVisionContext& ctx, bool force = false, bool shouldAttachAlternativeIntents = true) const override;

        bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

        static inline TStringBuf ForceName() {
            return "computer_vision_force_office_lens";
        }

        bool IsNeedOfficeLensData() const override {
            return true;
        }

        TStringBuf GetAnswerId() const override {
            return TStringBuf("office_lens");
        }

        TStringBuf GetCannotApplyMessage() const override;

        void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

    protected:
        bool IsSuggestible(const TComputerVisionContext& ) const override;

        void Compose(TComputerVisionContext& ctx) const override;
    };

    class TComputerVisionOfficeLensHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionOfficeLensHandler();

        static void Register(THandlersMap* handlers);

        TMaybe <TString> GetForcingString() const;

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this__office_lens");
        };

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_office_lens");
        };
    };

    class TComputerVisionOfficeLensDiskHandler : public TComputerVisionMainHandler {
    public:
        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this__office_lens_disk");
        }
    protected:
        TResultValue WrappedDo(TComputerVisionContext& cvContext) const override;
        bool IsNeedOfficeLensData(const IComputerVisionAnswer*) const override {
            return true;
        }
    };
}
