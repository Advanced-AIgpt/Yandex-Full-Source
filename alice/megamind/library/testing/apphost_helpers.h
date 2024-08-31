#pragma once

#include "components.h"
#include "mock_global_context.h"

#include <alice/megamind/library/apphost_request/item_adapter.h>
#include <alice/megamind/library/apphost_request/node.h>
#include <alice/megamind/library/speechkit/request_build.h>

#include <alice/library/logger/logger.h>

#include <apphost/lib/service_testing/service_testing.h>

#include <library/cpp/testing/unittest/registar.h>
#include <library/cpp/testing/gmock_in_unittest/gmock.h>

namespace NAlice::NMegamind::NTesting {

class TTestAppHostCtx : public IAppHostCtx {
public:
    explicit TTestAppHostCtx(IGlobalCtx& globalCtx);

    NAppHost::NService::TTestContext& TestCtx() {
        return Ctx_;
    }

    const NAppHost::NService::TTestContext& TestCtx() const {
        return Ctx_;
    }

    TItemProxyAdapter& ItemProxyAdapter() override {
        return ItemProxyAdapter_;
    }

    MOCK_METHOD(TRTLogger&, Log, (), (override));
    MOCK_METHOD(IGlobalCtx&, GlobalCtx, (), (override));

private:
    IGlobalCtx& GlobalCtx_;
    NAppHost::NService::TTestContext Ctx_;
    TItemProxyAdapter ItemProxyAdapter_;
};

class TAppHostFixture : public NUnitTest::TBaseFixture {
public:
    TAppHostFixture() {
        GlobalCtx.GenericInit();
    }

    TTestAppHostCtx CreateAppHostContext() {
        return TTestAppHostCtx{GlobalCtx};
    }

    TMockGlobalContext GlobalCtx;
};

using TTestAppHostSpeechKitRequest = TTestComponents<TTestEventComponent, TTestClientComponent, TTestRequestParts>;

template <typename T>
inline void AppHostDumpItems(const T& items, TStringBuf title, IOutputStream& out) {
    for (auto it = items.begin(), end = items.end(); it != end; ++it) {
        out << title << ": " << it.GetTag() << " " << it.GetType() << Endl;
    }
}

inline void AppHostDump(NAppHost::IServiceContext& ctx, IOutputStream& out,
                        NAppHost::EContextItemSelection itemSelection = NAppHost::EContextItemSelection::Anything) {
    AppHostDumpItems(ctx.GetRawInputItemRefs(), "Raw", out);
    AppHostDumpItems(ctx.GetItemRefs(itemSelection), "Json", out);
    AppHostDumpItems(ctx.GetProtobufItemRefs(itemSelection), "Proto", out);
}

} // NAlice::NMegamind::NTesting
