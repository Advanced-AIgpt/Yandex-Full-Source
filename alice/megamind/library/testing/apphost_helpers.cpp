#include "apphost_helpers.h"

#include <library/cpp/json/json_value.h>

namespace NAlice::NMegamind::NTesting {
namespace {

class TTestAppHostRequestCtx : public TRequestCtx {
public:
    class TInitializer : public TRequestCtx::IInitializer {
    public:
        TInitializer(IAppHostCtx& ahCtx)
            : AhCtx_{ahCtx}
        {
        }

        TCgiParameters StealCgi() override {
            return {};
        }

        THttpHeaders StealHeaders() override {
            return {};
        }

        NUri::TUri StealUri() override {
            return NUri::TUri{};
        }

        TRTLogger& Logger() override {
            return AhCtx_.Log();
        }

    private:
        IAppHostCtx& AhCtx_;
    };

public:
    TTestAppHostRequestCtx(IAppHostCtx& ctx)
        : TRequestCtx{ctx.GlobalCtx(), TInitializer{ctx}}
    {
    }

    // TRequestCtx overrides:
    THolder<IHttpResponse> CreateHttpResponse() const override {
        return {};
    }

    const TString& Body() const override {
        return Default<TString>();
    }

    TStringBuf NodeLocation() const override {
        return "TEST_NODE";
    }
};

} // namespace


TTestAppHostCtx::TTestAppHostCtx(IGlobalCtx& globalCtx)
    : GlobalCtx_{globalCtx}
    , Ctx_{NJson::JSON_ARRAY}
    , ItemProxyAdapter_{Ctx_, TRTLogger::NullLogger(), GlobalCtx_, true}
{
    EXPECT_CALL(*this, GlobalCtx()).WillRepeatedly(testing::ReturnRef(GlobalCtx_));
    EXPECT_CALL(*this, Log()).WillRepeatedly(testing::ReturnRef(TRTLogger::NullLogger()));
}

} // NAlice::NMegamind::NTesting
