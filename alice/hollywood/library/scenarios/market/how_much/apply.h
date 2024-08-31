#pragma once

#include "scenario.h"
#include <alice/hollywood/library/scenarios/market/common/context.h>
#include <alice/hollywood/library/scenarios/market/common/response_builder.h>
#include <alice/hollywood/library/scenarios/market/common/search_info.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NMarket::NHowMuch {

namespace NImpl {

class TBase {
public:
    TBase(TMarketApplyContext& ctx)
        : Ctx(ctx)
        , Scenario(ctx)
    {}

    virtual void Do() = 0;

protected:
    TMarketApplyContext& Ctx;
    THowMuchScenario Scenario;
};

} // namespace NImpl

class TApplyPrepareImpl : public NImpl::TBase {
public:
    using TBase::TBase;
    void Do() override final;
};

class TApplyRenderImpl : public NImpl::TBase {
public:
    using TBase::TBase;
    void Do() override final;

private:
    TVector<const TReportDocument*> GetSupportedDocs(
        const TVector<TReportDocument>& allDocs,
        const TSearchInfo& searchInfo) const;
    TVector<TGalleryItem> CreateGalleryItems(
        const TVector<const TReportDocument*>& docs,
        const TSearchInfo& searchInfo) const;
};

} // namespace NAlice::NHollywood::NMarket::NHowMuch
