#pragma once

#include <alice/megamind/library/util/status.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {
namespace NImpl {

/** SFINAE. Struct for checking if static Component::Create(Args...) function exists.
 */
template <typename TComponent, typename... TArgs>
struct TCreateChecker {
    template <typename C>
    static constexpr decltype(C::Create(std::declval<TArgs>()...), bool()) Test(int) {
        return true;
    }

    template <typename>
    static constexpr bool Test(...) {
        return false;
    }

    static constexpr bool Value = Test<TComponent>(int());
};

} // namespace NImpl

/** This class allows to aggregate different components in one entity with ability to control
  * of Creating/Initialization process.
  * All the components are created in the reverse order as they written in template.
  * Moreover, each component can use any of the left one as a dependency (see the exmaple below).
  * Each component is usually has a base class and its implementation (apphost, http, ...).
  * A base class must have inner struct/class ```TView``` which constructor has const ref to its base
  * component class. Any implementation must have one static method ```Create()``` with the following
  * signature```static TErrorOr<TComponentImpl> Create(TCtx&& ctx, TRequestComponentsView<...> view)```.
  * ```ctx``` argument is a initialization context, it can be anything!
  * ```view``` is not mandatory argument but if this component depends on others you can pass
  * them through this argument.
  * Component example: @code
// Base class, used in Views
struct TFooComponent {
    struct TView {
        TView(const TBaseComponent& r) : R{r} {}

        const TString& GetValue() const { return R.GetValue(); }

        const TBaseComponent& R;
    }

    virtual ~TFooComponent() = default;

    virtual const TString& GetValue() const = 0;
};

// Implementation
struct TAppHostFooComponent : public TFooComponent {
    ....
    const TString& GetValue() const override {
        return Ctx.GetItem().GetValue();
    }
    TAppHostContext Ctx;

    static TErrorOr<TAppHostFooComponent> Create(TSomeInitContext& ctx) {
        if (!ctx.HasSomething()) {
            return TError{TError::EType::Critcal};
        }

        return TAppHostFooComponent{ctx.GetSomething()};
    }
};

// Both base and implementation in one class
struct TAllInOneComponent {
    struct TView {
        TView(const TAllInOneComponent& r) : R{r} {}

        const TString& GetValue() const { return R.GetValue(); }

        const TAllInOneComponent& R;
    }

    const TString& GetValue() const {
        return ...;
    }

    static TErrorOr<TAllInOneComponent> Create(TSomeInitContext& ctx) {
        if (!ctx.HasSomething()) {
            return TError{TError::EType::Critcal};
        }

        return TAllInOneComponent{ctx.GetSomething()};
    }
};
@endcode
  * Usually composite class creates and initializes all the components in one specific place (apphost, http)
  * and then is used ONLY through view/proxy class TRequestComponentsView<Combination_of_components>() which is
  * lightweight (consists of refs to each component).
  * Composite and View example: @code
struct TBarComponent {...};
struct TBazComponent {...};

struct TAppHostBarComponent : public TBarComponent {...};
struct TAppHostBazComponent : public TBarComponent {...};

strict TAppHostInitContext {...};
TStatus status;
// Components creation order: Baz, Bar, Foo
TRequestComposite<TAppHostFooComposite, TAppHostBarComponent, TAppHostBazComponent> composite{TAppHostInitContext{}, status};
if (status.Defined()) {
    // Error during components creation process.
    return;
}

void Func1(TRequestComponentsView<TBazComponent> view) {
    view.AnyBazTViewMethod();
}

void Func2(TRequestComponentsView<TBazComponent, TBarComponent> view) {
    view.AnyBazTViewMethod();
    view.AnyBarTViewMethod();
    Func1(view); // Usefull ;)
}
@endcode
  */
template <typename... TComponents>
class TRequestComposite;

template <>
class TRequestComposite<> {
public:
    template <typename TCtx>
    TRequestComposite(TCtx& /* ctx */, TStatus& /* status */) {
    }
};

template <typename TComponent, typename... TComponents>
class TRequestComposite<TComponent, TComponents...> : public TRequestComposite<TComponents...> {
public:
    template <typename TCtx>
    TRequestComposite(TCtx& ctx, TStatus& status)
        : TRequestComposite<TComponents...>{ctx, status}
    {
        if (status.Defined()) {
            return;
        }

        if constexpr (NImpl::TCreateChecker<TComponent, TCtx&>::Value) {
            status = TComponent::Create(ctx).MoveTo(Component_);
        } else {
            status = TComponent::Create(ctx, *this).MoveTo(Component_);
        }
    }

    operator const TComponent& () const {
        return Component_.GetRef();
    }

    operator TComponent& () {
        return Component_.GetRef();
    }

private:
    TMaybe<TComponent> Component_;
};

} // namespace NAlice::NMegamind
