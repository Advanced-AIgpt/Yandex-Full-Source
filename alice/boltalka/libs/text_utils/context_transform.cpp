#include "context_transform.h"

namespace NNlgTextUtils {

TUtteranceWiseTransform::TUtteranceWiseTransform(IUtteranceTransformPtr transform)
    : UtteranceTransform(transform)
{
}

TVector<TString> TUtteranceWiseTransform::Transform(TVector<TString> context) const {
    for (auto& turn : context) {
        turn = UtteranceTransform->Transform(turn);
    }
    return context;
}

TSetContextNumTurns::TSetContextNumTurns(size_t numTurns)
    : NumTurns(numTurns)
{
}

TVector<TString> TSetContextNumTurns::Transform(TVector<TString> context) const {
    context.resize(NumTurns);
    return context;
}

const THashSet<TString> TCutAliceFromUser::Alices = {
    "алиса", "алис"
};

TVector<TString> TCutAliceFromUser::Transform(TVector<TString> context) const {
    for (size_t i = 0; i < context.size(); i += 2) {
        auto& turn = context[i];
        for (const auto& prefix : Alices) {
            if (turn.StartsWith(prefix + " ")) {
                turn = turn.substr(prefix.size() + 1);
                break;
            }
        }
        for (const auto& suffix : Alices) {
            if (turn.EndsWith(" " + suffix)) {
                turn = turn.substr(0, turn.size() - suffix.size() - 1);
                break;
            }
        }
    }
    return context;
}

TCompoundContextTransform::TCompoundContextTransform(TVector<IContextTransformPtr> transforms)
    : Transforms(transforms)
{
}

TVector<TString> TCompoundContextTransform::Transform(TVector<TString> context) const {
    for (const auto& transform : Transforms) {
        context = transform->Transform(context);
    }
    return context;
}

TNlgSearchContextTransform::TNlgSearchContextTransform(ELanguage lang)
    : UtteranceTransform(lang)
{
}

TVector<TString> TNlgSearchContextTransform::Transform(TVector<TString> context) const {
    for (auto& turn : context) {
        turn = UtteranceTransform.Transform(turn);
    }
    context = CutAlice.Transform(context);
    return context;
}

}
