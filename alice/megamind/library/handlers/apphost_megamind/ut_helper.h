#pragma once

#include "components.h"
#include "skr.h"

#include <alice/megamind/library/testing/apphost_helpers.h>
#include <alice/megamind/library/testing/components.h>
#include <alice/megamind/library/testing/speechkit.h>

#include <util/generic/maybe.h>

namespace NAlice::NMegamind::NTesting {

class TAppHostTestInitSkr : public TAppHostInitSkr {
public:
    TAppHostTestInitSkr(TTestAppHostCtx& ahCtx)
        : AhCtx_{ahCtx}
    {
    }

protected:
    TStatus OnSuccess(const IContext& /* ctx */) override {
        return Success();
    }

    TSpeechKitRequest CreateSkr(TSpeechKitInitContext& initCtx) override final {
        Composite_.ConstructInPlace(MakeSimpleShared<TTestSpeechKitRequest::TComposite>(initCtx));
        return TSpeechKitRequest{*Composite_};
    }

protected:
    IAppHostCtx& AhCtx() override final {
        return AhCtx_;
    }

private:
    TTestAppHostCtx& AhCtx_;
    TMaybe<TTestSpeechKitRequest> Composite_;
};

/** Put SKR items in apphost context.
 * Very useful for non SKR nodes.
 */
void FakeSkrInit(TTestAppHostCtx& ahCtx, const TSpeechKitRequestBuilder& skrBuilder);

class TAppHostWalkerTestFixture : public TAppHostFixture {
public:
    // FIXME (petrk) Bad class must be rewritten.
    class TFakeStorage final : public ISharedFormulasAdapter {
    public:
        TBaseCalcerPtr GetSharedFormula(const TStringBuf /* name */) const override {
            return {};
        }
    };

public:
    TAppHostWalkerTestFixture();
    void RegisterScenario(const TString& name);
    void InitBegemot(TStringBuf resourceId);
    void InitSkr(const TSpeechKitRequestBuilder& skrBuilder);

public:
    TTestAppHostCtx AhCtx;

private:
    NGeobase::TLookup GeoLookup_;
    TScenarioConfigRegistry ScenarioRegistry_;
    TFakeStorage Storage_;
    TFormulasStorage FormulasStorage_;
};

} // namespace NAlice::NMegamind::NTesting
