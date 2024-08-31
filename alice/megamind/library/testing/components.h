#pragma once

#include <library/cpp/testing/gmock_in_unittest/gmock.h>

#include <alice/megamind/library/request_composite/composite.h>
#include <alice/megamind/library/request_composite/event.h>
#include <alice/megamind/library/request_composite/client/client.h>
#include <alice/megamind/library/request_composite/view.h>
#include <alice/megamind/library/speechkit/request_build.h>
#include <alice/megamind/library/speechkit/request_parts.h>
#include <alice/megamind/library/util/status.h>

#include <alice/megamind/protos/common/device_state.pb.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind {

struct TTestSimpleInitComponentContext {
    TMaybe<TString> ClientIp;
    TMaybe<TString> AuthToken;
};
class TTestInitComponentContext : public TSpeechKitInitContext, public TTestSimpleInitComponentContext {
public:
    using TSpeechKitInitContext::TSpeechKitInitContext;
};

class TTestEventComponent : public TEventComponent {
public:
    TTestEventComponent()
        : Proto_{MakeSimpleShared<TEventComponent::TEventProto>()} {
    }
    explicit TTestEventComponent(TTestInitComponentContext& initCtx)
        : Proto_{initCtx.EventProtoPtr} {
    }
    explicit TTestEventComponent(TSpeechKitInitContext& initCtx)
        : Proto_{initCtx.EventProtoPtr} {
    }

    const TEventProto& Event() const override {
        return *Proto_;
    }

    TEventWrapper EventWrapper() const override {
        return IEvent::CreateEvent(Event()).release();
    }

    void UpdateEvent(const TEventProto& proto) {
        Proto_->CopyFrom(proto);
    }

private:
    TEventProtoPtr Proto_;
};

struct TTestClientComponent : public TClientComponent {
public:
    TTestClientComponent() = default;
    explicit TTestClientComponent(TTestInitComponentContext& initCtx)
        : TTestClientComponent(static_cast<TSpeechKitInitContext&>(initCtx))
    {
        EXPECT_CALL(testing::Const(*this), ClientIp()).WillRepeatedly(testing::Return(initCtx.ClientIp.Get()));
        EXPECT_CALL(testing::Const(*this), AuthToken()).WillRepeatedly(testing::Return(initCtx.AuthToken.Get()));
    }

    explicit TTestClientComponent(TSpeechKitInitContext& initCtx)
        : ExpFlags_{CreateExpFlags(initCtx.Proto->GetRequest().GetExperiments())}
        , ClientFeatures_{CreateClientFeatures(*initCtx.Proto, *ExpFlags_)}
        , DeviceState_{initCtx.Proto->GetRequest().GetDeviceState()}
    {
        EXPECT_CALL(testing::Const(*this), ClientFeatures()).WillRepeatedly(testing::ReturnRef(ClientFeatures_.GetRef()));
        EXPECT_CALL(testing::Const(*this), ExpFlags()).WillRepeatedly(testing::ReturnRef(ExpFlags_.GetRef()));
        EXPECT_CALL(testing::Const(*this), DeviceState()).WillRepeatedly(testing::ReturnRef(DeviceState_.GetRef()));
    }

    MOCK_METHOD(const TString*, ClientIp, (), (const, override));
    MOCK_METHOD(const TString*, AuthToken, (), (const, override));
    MOCK_METHOD(const TClientFeatures&, ClientFeatures, (), (const, override));
    MOCK_METHOD(const TExpFlags&, ExpFlags, (), (const, override));
    MOCK_METHOD(const TDeviceState&, DeviceState, (), (const, override));

private:
    TMaybe<TExpFlags> ExpFlags_;
    TMaybe<TClientFeatures> ClientFeatures_;
    TMaybe<TDeviceState> DeviceState_;
};

class TTestRequestParts : public TRequestParts {
public:
    TTestRequestParts(TTestInitComponentContext& proto, TRequestComponentsView<TEventComponent> view)
        : TRequestParts{proto, view}
    {
    }

    TTestRequestParts(TSpeechKitInitContext& proto, TRequestComponentsView<TEventComponent> view)
        : TRequestParts{proto, view}
    {
    }
};

template <typename... TComponents>
class TTestComponents;

template <>
class TTestComponents<> {
public:
    TTestComponents() = default;
    explicit TTestComponents(TTestInitComponentContext& /* initCtx */) {
    }
    explicit TTestComponents(TSpeechKitInitContext& /* initCtx */) {
    }
};

template <typename TComponent, typename... TComponents>
class TTestComponents<TComponent, TComponents...> : public TTestComponents<TComponents...> {
public:
    TTestComponents() = default;

    template <typename TCtx, std::enable_if_t<std::is_constructible<TComponent, TCtx&>::value, int> = 0>
    explicit TTestComponents(TCtx& initCtx)
        : TTestComponents<TComponents...>{initCtx}
        , Component_{initCtx}
    {
    }

    template <typename TCtx, std::enable_if_t<std::is_constructible<TComponent, TCtx&, std::decay_t<TTestComponents<TComponents...>>>::value, int> = 0>
    explicit TTestComponents(TCtx& initCtx)
        : TTestComponents<TComponents...>{initCtx}
        , Component_{initCtx, *this}
    {
    }

    operator const TComponent& () const {
        return Component_;
    }

    operator TComponent& () {
        return Component_;
    }

    template <typename T>
    T& Get() {
        return Component_;
    }

private:
    TComponent Component_;
};

} // NAlice::NMegamind
