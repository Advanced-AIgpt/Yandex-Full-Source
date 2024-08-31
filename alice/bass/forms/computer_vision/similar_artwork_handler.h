#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {

    class TCVAnswerSimilarArtwork: public IComputerVisionAnswer {
    public:
        TCVAnswerSimilarArtwork();

        bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;
        bool IsSuggestible(const TComputerVisionContext&) const override {
            return true;
        }

        bool TryApplyTo(TComputerVisionContext& ctx, bool force, bool shouldAttachAlternativeIntents = true) const override;

        static inline TStringBuf ForceName() {
            return "computer_vision_force_similar_artwork";
        }

        const TStringBuf AdditionalFlag() const override {
            return TStringBuf("artwork_filter");
        }

        TStringBuf GetAnswerId() const override {
            return TStringBuf("similar_artwork");
        }

        TStringBuf GetCannotApplyMessage() const override;
        void AttachAlternativeIntentsSuggest(TComputerVisionContext& ctx) const override;

    protected:
        void Compose(TComputerVisionContext& ctx) const override;
    };

    class TComputerVisionSimilarArtworkHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionSimilarArtworkHandler();

        static void Register(THandlersMap* handlers);

        static inline TStringBuf FormShortName() {
            return TStringBuf("image_what_is_this_similar_artwork");
        }

        static inline TStringBuf FormName() {
            return TStringBuf("personal_assistant.scenarios.image_what_is_this_similar_artwork");
        }
    };

    class TComputerVisionEllipsisSimilarArtworkHandler : public TComputerVisionMainHandler {
    public:
        TComputerVisionEllipsisSimilarArtworkHandler()
        {}

        static void Register(THandlersMap* handlers);
        const TStringBuf GetAdditionalFlag(const IComputerVisionAnswer*) const override {
            return AnswerSimilarArtwork.AdditionalFlag();
        }

    protected:
        bool MakeBestAnswer(TComputerVisionContext& cvContext) const override;

    private:
        TCVAnswerSimilarArtwork AnswerSimilarArtwork;
    };

}

