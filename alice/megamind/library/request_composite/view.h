#pragma once

namespace NAlice::NMegamind {

template <typename... TComponents>
class TRequestComponentsView : public TComponents::TView... {
public:
    template <typename TComposite>
    TRequestComponentsView(const TComposite& composite)
        : TComponents::TView{composite}...
    {
    }
};

} // namespace NAlice::NMegamind
