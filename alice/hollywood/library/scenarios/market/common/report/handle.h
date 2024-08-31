#pragma once

#include <alice/hollywood/library/scenarios/market/common/context.h>

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/response/response_builder.h>

namespace NAlice::NHollywood::NMarket {

/*
    Хэндлер для обработки редиректов репорта.
    При получении редиректа выставляет флаг "redirect" и отправляет новый запрос в репорт.
    Иначе ничего не делает.
    В случае отсутствия редиректа, app_host может перенаправить ответ репорта, опираясь на наличие
    флага "redirect".
*/
class TReportResponseHandleImpl {
public:
    TReportResponseHandleImpl(TScenarioHandleContext& ctx, bool allowRedirects);
    void Do();
private:
    TMarketApplyContext Ctx;
    bool AllowRedirects;
};

class TReportResponseHandle : public TScenario::THandleBase {
public:
    TString Name() const override
    {
        return "handle_report_response";
    }

    void Do(TScenarioHandleContext& ctx) const override final
    {
        TReportResponseHandleImpl impl { ctx , true /* = allowRedirects */ };
        impl.Do();
    }
};

class TReportResponseHandleNoRedir : public TScenario::THandleBase {
public:
    TString Name() const override
    {
        return "handle_report_response_noredir";
    }

    void Do(TScenarioHandleContext& ctx) const override final
    {
        TReportResponseHandleImpl impl { ctx , false /* = allowRedirects */ };
        impl.Do();
    }
};

}  // namespace NAlice::NHollywood::NMarket
