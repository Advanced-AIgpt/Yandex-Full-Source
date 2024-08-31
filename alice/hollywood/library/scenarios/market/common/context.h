#pragma once

#include "experiments.h"
#include "fast_data.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/resources/geobase.h>
#include <alice/hollywood/library/response/response_builder.h>
#include <alice/library/geo/user_location.h>

#include <library/cpp/geobase/lookup.hpp>

using namespace NAlice::NScenarios;

namespace NAlice::NHollywood::NMarket {

/*
    Классы обёртки над контекстом голливуда.
    Содержат логику специфичную для всех сценариев маркета.
    Содержат ленивую инициализацию объектов голливуда (при желании можно вынести её в голливудный контекст)

    TODO(bas1330) Сейчас выполняет роли получения входных данных и записи выходных.
    В СветломГолливудеБудущего преполагается разбить класс на 2:
    - константный класс содержащий входные данные
    - класс позволяющий записывать ответ
*/

class TMarketBaseContext {
public:
    TMarketBaseContext(TScenarioHandleContext& ctx)
        : Ctx(&ctx)
    {}

    const TScenarioHandleContext* operator->() const { return Ctx; }
    TScenarioHandleContext* operator->() { return Ctx; }
    const TScenarioHandleContext& operator*() const { return *Ctx; }
    TScenarioHandleContext& operator*() { return *Ctx; }

    virtual const TScenarioBaseRequestWrapper& RequestWrapper() const = 0;
    TRTLogger& Logger() const { return Ctx->Ctx.Logger(); }
    bool HasExpFlag(EMarketExperiment flag) const { return RequestWrapper().HasExpFlag(ToString(flag)); }
    bool CanOpenUri() const; // TODO(bas1330) rm after MEGAMIND-1486
    bool SupportsDivCards() const;

    TNlgData BaseNlgData() { return {Logger(), RequestWrapper()}; } // TODO(bas1330) make lazy
    TNlgWrapper& NlgWrapper();

    const TMarketFastData& FastData() const;
    const NGeobase::TLookup& Geobase() const;
    TUserLocation CreateUserLocation(NGeobase::TId regionId) const;

protected:
    TScenarioHandleContext* Ctx;

private:
    // TODO(bas1330): consider using other lazy member initialization
    mutable TMaybe<TNlgWrapper> NlgWrapper_;
    mutable std::shared_ptr<const TMarketFastData> FastDataPtr;
};


class TMarketRunContext : public TMarketBaseContext {
public:
    using TMarketBaseContext::TMarketBaseContext;

    const TScenarioRunRequestWrapper& RequestWrapper() const override;
    TMaybe<NGeobase::TId> GetUserRegionId() const;
    void AddResponse(TRunResponseBuilder&& builder);

    /*
        !!! Необходимо в nlg папке сценария добавить common_ru.nlg и записать:
        {% ext_nlgimport "alice/hollywood/library/scenarios/market/common/nlg/common_ru.nlg" %}
    */
    void SetIrrelevantResponse(TRunResponseBuilder& builder);

private:
    // TODO(bas1330): consider using other lazy member initialization
    mutable TMaybe<TScenarioRunRequest> RequestProto;
    mutable TMaybe<TScenarioRunRequestWrapper> RequestWrapper_;
};


class TMarketApplyContext : public TMarketBaseContext {
public:
    using TMarketBaseContext::TMarketBaseContext;

    const TScenarioApplyRequestWrapper& RequestWrapper() const override;
    void AddResponse(TApplyResponseBuilder&& builder);

private:
    // TODO(bas1330): consider using other lazy member initialization
    mutable TMaybe<TScenarioApplyRequest> RequestProto;
    mutable TMaybe<TScenarioApplyRequestWrapper> RequestWrapper_;
};

} // namespace NAlice::NHollywood::NMarket
