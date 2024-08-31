#include "composite.h"
#include "view.h"

#include <library/cpp/testing/unittest/registar.h>

#include <util/generic/vector.h>

using namespace NAlice;
using namespace NAlice::NMegamind;
template <typename... TComponents>
using TComposite = TRequestComposite<TComponents...>;

namespace {

struct TTestContext {
    TVector<TString> InitComponent;
};

class TBaseFooComponent {
public:
    struct TView {
        explicit TView(const TBaseFooComponent& ref)
            : Ref{ref}
        {
        }

        const TString& GetFooValue() const {
            return Ref.GetFooValue();
        }

        const TBaseFooComponent& Ref;
    };

public:
    virtual ~TBaseFooComponent() = default;
    virtual const TString& GetFooValue() const = 0;
};

class TBaseBazComponent {
public:
    struct TView {
        explicit TView(const TBaseBazComponent& ref)
            : Ref{ref}
        {
        }

        const TString& GetBazValue() const {
            return Ref.GetBazValue();
        }

        const TBaseBazComponent& Ref;
    };

public:
    virtual ~TBaseBazComponent() = default;
    virtual const TString& GetBazValue() const = 0;
};

class TFooComponentImpl : public TBaseFooComponent {
public:
    static TErrorOr<TFooComponentImpl> Create(TTestContext& ctx) {
        ctx.InitComponent.emplace_back("foo");
        return TFooComponentImpl{"foo"};
    }

    const TString& GetFooValue() const override {
        return Value_;
    }

private:
    TFooComponentImpl(TString value)
        : Value_{value}
    {
    }

    TString Value_;
};

class TFooComponentImplFail : public TBaseFooComponent {
public:
    static TErrorOr<TFooComponentImplFail> Create(TTestContext& ctx) {
        ctx.InitComponent.emplace_back("foo fail");
        return TError{TError::EType::Logic};
    }

    const TString& GetFooValue() const override {
        return Default<TString>();
    }
};

class TBazComponentImpl : public TBaseBazComponent {
public:
    static TErrorOr<TBazComponentImpl> Create(TTestContext& ctx) {
        ctx.InitComponent.emplace_back("baz");
        return TBazComponentImpl{"baz"};
    }

    const TString& GetBazValue() const override {
        return Value_;
    }

private:
    TBazComponentImpl(TString value)
        : Value_{value}
    {
    }

    TString Value_;
};

class TBazWithDepsComponentImpl : public TBaseBazComponent {
public:
    static TErrorOr<TBazWithDepsComponentImpl> Create(TTestContext& ctx, TRequestComponentsView<TBaseFooComponent> view) {
        ctx.InitComponent.emplace_back("baz with deps");
        return TBazWithDepsComponentImpl{"baz2 " + view.GetFooValue()};
    }

    const TString& GetBazValue() const override {
        return Value_;
    }

private:
    TBazWithDepsComponentImpl(TString value)
        : Value_{value}
    {
    }

    TString Value_;
};

Y_UNIT_TEST_SUITE(Composite) {
    Y_UNIT_TEST(CreateSmoke) {

        { // simple one component creation
            TTestContext ctx;
            TStatus status;
            TComposite<TFooComponentImpl> skrComposite{ctx, status};
            UNIT_ASSERT(!status);
            TRequestComponentsView<TBaseFooComponent> view{skrComposite};
            UNIT_ASSERT_VALUES_EQUAL(view.GetFooValue(), "foo");
            UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent.size(), 1);
            UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[0], "foo");
        }
        { // simple two component creation
            TTestContext ctx;
            TStatus status;
            TComposite<TBazComponentImpl, TFooComponentImpl> skrComposite{ctx, status};
            UNIT_ASSERT(!status);
            TRequestComponentsView<TBaseFooComponent, TBaseBazComponent> view(skrComposite);
            UNIT_ASSERT_VALUES_EQUAL(view.GetBazValue(), "baz");
            UNIT_ASSERT_VALUES_EQUAL(view.GetFooValue(), "foo");
            UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent.size(), 2);
            UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[0], "foo");
            UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[1], "baz");
        }
    } // Y_UNIT_TEST(CreateSmoke)

    Y_UNIT_TEST(CreateFail) {
        TTestContext ctx;
        TStatus status;
        TComposite<TBazComponentImpl, TFooComponentImplFail> skrComposite{ctx, status};
        UNIT_ASSERT(status);
        UNIT_ASSERT_EXCEPTION(TRequestComponentsView<TBaseBazComponent>(skrComposite), yexception);
        UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent.size(), 1);
        UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[0], "foo fail");
    } // Y_UNIT_TEST(CreateFail)

    Y_UNIT_TEST(CreateWithDeps) {
        TTestContext ctx;
        TStatus status;
        TComposite<TBazWithDepsComponentImpl, TFooComponentImpl> skrComposite{ctx, status};
        UNIT_ASSERT(!status);
        TRequestComponentsView<TBaseFooComponent, TBaseBazComponent> view(skrComposite);
        UNIT_ASSERT_VALUES_EQUAL(view.GetFooValue(), "foo");
        UNIT_ASSERT_VALUES_EQUAL(view.GetBazValue(), "baz2 foo");
        UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent.size(), 2);
        UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[0], "foo");
        UNIT_ASSERT_VALUES_EQUAL(ctx.InitComponent[1], "baz with deps");
    } // Y_UNIT_TEST(CreateWithDeps)
} // Y_UNIT_TEST(Composite)

} // namespace
