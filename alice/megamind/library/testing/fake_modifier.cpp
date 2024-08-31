#include "fake_modifier.h"

#include <alice/megamind/library/modifiers/context.h>
#include <alice/megamind/library/modifiers/utils.h>
#include <alice/megamind/library/experiments/flags.h>

namespace NAlice::NMegamind {

namespace {

constexpr TStringBuf MODIFIER_NAME = "Fake";

struct TFakeResponseModifier : public TResponseModifier {
    TFakeResponseModifier()
        : TResponseModifier{MODIFIER_NAME}
    {
    }

    // Adds a single phrase to all non-error answers
    TApplyResult TryApply(TResponseModifierContext& ctx, TScenarioResponse& response) override {
        if (!ctx.HasExpFlag(EXP_DEBUG_RESPONSE_MODIFIERS)) {
            return NonApply(TNonApply::EType::DisabledByFlag);
        }

        const TString textToAdd = "Я всё сказала.";
        AddTextToResponse(response, textToAdd, /* appendTts= */ true);
        return ApplySuccess();
    }
};

} // namespace

TModifierPtr CreateFakeModifier() {
    return MakeHolder<TFakeResponseModifier>();
}

} // NAlice::NMegamind
