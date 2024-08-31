#pragma once

#include <alice/bass/forms/computer_vision/context.h>
#include <alice/bass/forms/computer_vision/handler.h>

namespace NBASS {
class TCVAnswerSimilarLike: public IComputerVisionAnswer {
public:
    TCVAnswerSimilarLike();

    bool TryApplyTo(TComputerVisionContext& ctx, bool force = false, bool shouldAttachAlternativeIntents = true) const override;

    bool IsSuitable(const TComputerVisionContext& ctx, bool force) const override;

    static inline TStringBuf ForceName() {
        return TStringBuf("computer_vision_force_similar");
    };

    TStringBuf GetAnswerId() const override {
        return TStringBuf("similar_like");
    }

protected:
    bool IsSuggestible(const TComputerVisionContext& ) const override {
        return true;
    }
    ECaptureMode AnswerType() const override {
        return ECaptureMode::SIMILAR_LIKE;
    }

    void Compose(TComputerVisionContext& ctx) const override;
};

class TComputerVisionSimilarHandler : public TComputerVisionMainHandler {
public:
    TComputerVisionSimilarHandler();

    static void Register(THandlersMap* handlers);

    static inline TStringBuf FormShortName() {
        return TStringBuf("image_what_is_this_similar");
    }

    static inline TStringBuf FormName() {
        return TStringBuf("personal_assistant.scenarios.image_what_is_this_similar");
    }
};

class TComputerVisionEllipsisSimilarLikeHandler : public TComputerVisionMainHandler {
public:
    TComputerVisionEllipsisSimilarLikeHandler();

    static void Register(THandlersMap* handlers);

    static inline TStringBuf FormShortName() {
        return TStringBuf("image_what_is_this__similar");
    };

    static inline TStringBuf FormName() {
        return TStringBuf("personal_assistant.scenarios.image_what_is_this__similar");
    };

protected:
    bool MakeBestAnswer(TComputerVisionContext& cvContext) const;

protected:
    TCVAnswerSimilarLike AnswerLike;
};
}
