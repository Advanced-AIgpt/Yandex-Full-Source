#include "market.h"

#include <alice/bass/libs/globalctx/globalctx.h>
#include <alice/bass/libs/scheduler/scheduler.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include "market/context.h"
#include "market/clear_request.h"
#include "market/client/base_client.h" // THttpTimeoutException
#include "market/dynamic_data.h"
#include "market/forms.h"
#include "market/market_checkout.h"
#include "market/market_choice.h"
#include "market/market_how_much_impl.h"
#include "market/market_orders_status.h"
#include "market/market_recurring_purchase.h"
#include "market/market_beru_my_bonuses_list.h"
#include "market/shopping_list.h"
#include "market/util/suggests.h"
#include "search/search.h"

#include <util/charset/utf8.h>
#include <util/generic/serialized_enum.h>
#include <util/string/builder.h>


namespace NBASS {

namespace {

const TString& GetChoiceProductScenarioName(const TString& form) {
    if (form == "personal_assistant.scenarios.market__beru_order" ||
        form == "personal_assistant.scenarios.market_beru" ||
        form == "personal_assistant.scenarios.market_native_beru" ||
        form == "personal_assistant.scenarios.market_product_details__beru_order")
    {
        return NAlice::NProductScenarios::BERU;
    }
    return NAlice::NProductScenarios::MARKET;
}

} // namespace

////////////////////////////////////////////////////////////////////////////////

TMarketFormHandlerBase::TMarketFormHandlerBase()
{
}

TMarketFormHandlerBase::~TMarketFormHandlerBase()
{
}

TResultValue TMarketFormHandlerBase::Do(TRequestHandler& r)
{
    try {
        auto& requestContext = r.Ctx();
        NMarket::TMarketContext marketCtx(requestContext, GetScenarioType());
        if (IsProtocolActivationByVins(marketCtx)) {
            return ProcessProtocolActivation(marketCtx);
        }
        if (!IsEnabled(marketCtx)) {
            return ProcessScenarioDisabled(marketCtx);
        }
        try {
            return DoImpl(marketCtx);
        } catch (const NMarket::THttpTimeoutException& e) {
            marketCtx.AddDebugTraces();
            return ProcessError(r, e.what(), e.BackTrace());
        }
    } catch (const yexception& e) {
        return ProcessError(r, e.what(), e.BackTrace());
    } catch (...) {
        return ProcessError(r, CurrentExceptionMessage());
    }
}

bool TMarketFormHandlerBase::IsProtocolActivationByVins(const NMarket::TMarketContext& ctx) const
{
    Y_UNUSED(ctx);
    return false;
}

TResultValue TMarketFormHandlerBase::ProcessProtocolActivation(NMarket::TMarketContext& ctx)
{
    ctx.RenderYandexSearch();
    return TResultValue();
}

TResultValue TMarketFormHandlerBase::ProcessScenarioDisabled(NMarket::TMarketContext& ctx)
{
    ctx.Ctx().AddTextCardBlock(TStringBuf("market_common__scenario_disabled"));
    return TResultValue();
}

TResultValue TMarketFormHandlerBase::ProcessError(TRequestHandler& r, TStringBuf errMsg, const TBackTrace* trace)
{
    LOG(ERR) << "Caught exception in market scenario: " << errMsg << Endl;
    const bool isDebug = r.Ctx().HasExpFlag("market_debug");
    if (isDebug) {
        NSc::TValue data;
        data["error"] = errMsg;
        if (trace != nullptr) {
            LOG(ERR) << "Stack: " << trace->PrintToString() << Endl;
        }
        r.Ctx().AddTextCardBlock("error", data);
        return TResultValue();
    }
    return TError(TError::EType::MARKETERROR, TStringBuilder() << "Caught exception: " << errMsg);
}

////////////////////////////////////////////////////////////////////////////////

TResultValue TMarketChoiceFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    context.Ctx().GetAnalyticsInfoBuilder().SetProductScenarioName(GetChoiceProductScenarioName(context.FormName()));
    const auto result = TryDoImpl(context, false /* isParallelMode */);
    if (auto tryError = std::get_if<TError>(&result)) {
        return *tryError;
    }
    return TResultValue();
}

NMarket::EScenarioType TMarketChoiceFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::CHOICE;
}

bool TMarketChoiceFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    const auto form = FromString<NMarket::EChoiceForm>(ctx.FormName());
    if (EqualToOneOf(
            form,
            NMarket::EChoiceForm::ProductDetailsExternal,
            NMarket::EChoiceForm::ProductDetailsExternal_BeruOrder)) {
        return true;
    }
    switch (ctx.GetPreviousChoiceMarketType()) {
        case NMarket::EMarketType::GREEN:
            return ctx.GetExperiments().Market();
        case NMarket::EMarketType::BLUE:
            return ctx.GetExperiments().MarketBeru();
    }
}

TResultValue TMarketChoiceFormHandler::ProcessScenarioDisabled(NMarket::TMarketContext& ctx)
{
    ctx.SetState(NMarket::EChoiceState::Exit);
    return TMarketFormHandlerBase::ProcessScenarioDisabled(ctx);
}

void TMarketChoiceFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx)
{
    NMarket::TPromotionsDynamicData::Start(globalCtx);
    NMarket::TPhrases::Start(globalCtx);
    NMarket::TDeniedCategories::Start(globalCtx);
    NMarket::TAllowedCategories::Start(globalCtx);
    NMarket::TDeniedCategoriesOnMarket::Start(globalCtx);
    NMarket::TAllowedCategoriesOnMarket::Start(globalCtx);

    auto cbMarketForm = []() {
        return MakeHolder<TMarketChoiceFormHandler>();
    };

    const size_t threadsCount = globalCtx.Config().HasMarket() ? globalCtx.Config().Market().SearchThreads() : 1;

    if (threadsCount <= 1) {
        for (const auto& form: GetEnumAllValues<NMarket::EChoiceForm>()) {
            handlers->emplace(ToString(form), cbMarketForm);
        }
        return;
    }

    auto threadPoolPtr = &globalCtx.MarketThreadPool();
    auto cbSearchForm = [=]() {
        return MakeHolder<TSearchFormHandler>(*threadPoolPtr);
    };
    auto cbMarketSearchParallelForm = [=]() {
        return THolder<TParallelHandlerExecutor>(new TParallelHandlerExecutor(*threadPoolPtr, {cbMarketForm, cbSearchForm}));
    };
    for (const auto& form: GetEnumAllValues<NMarket::EChoiceForm>()) {
        switch (form) {
            case NMarket::EChoiceForm::Native:
            case NMarket::EChoiceForm::NativeBeru:
                handlers->emplace(ToString(form), cbMarketSearchParallelForm);
                break;
            default:
                handlers->emplace(ToString(form), cbMarketForm);
                break;
        }
    }
}

IParallelHandler::TTryResult TMarketChoiceFormHandler::TryToHandle(TContext& ctx)
{
    NMarket::TMarketContext marketCtx(ctx, GetScenarioType());
    return TryDoImpl(marketCtx, true /* isParallelMode */);
}

IParallelHandler::TTryResult TMarketChoiceFormHandler::TryDoImpl(NMarket::TMarketContext& context, bool isParallelMode)
{
    const auto& clientInfo = context.MetaClientInfo();
    if (clientInfo.IsElariWatch()
        || clientInfo.IsSmartSpeaker()
        || clientInfo.IsNavigator()
        || clientInfo.IsYaAuto())
    {
        context.RenderNoActivationPhrase();
        context.SetState(NMarket::EChoiceState::Exit);
        return IParallelHandler::ETryResult::NonSuitable;
    }

    auto result = context.IsCheckoutStep()
        ? NMarket::TMarketCheckout(context).TryDo()
        : NMarket::TMarketChoice(context, isParallelMode).TryDo();

    if (!(result == IParallelHandler::TTryResult(IParallelHandler::ETryResult::NonSuitable))) {
        NMarket::AddCancelSuggest(context);
        NMarket::AddStartChoiceAgainSuggest(context);
    }
    NMarket::DeleteCancelSuggestIfNeeded(context);
    NMarket::AddOnboardingSuggestIfNeeded(context);

    return result;
}

////////////////////////////////////////////////////////////////////////////////

class TMarketChoiceFormHandlerExternal: public TMarketChoiceFormHandler {
public:
    // IParallelHandler overrides:
    TTryResult TryToHandle(TContext& ctx) override;
    TString Name() const override { return "market_from_rp"; }
};

IParallelHandler::TTryResult TMarketChoiceFormHandlerExternal::TryToHandle(TContext& ctx)
{
    NMarket::TMarketContext marketCtx(ctx, GetScenarioType());
    if (!marketCtx.GetExperiments().MarketNative()) {
        return IParallelHandler::ETryResult::NonSuitable;
    }
    const TString request(marketCtx.Request(true));
    if (request.empty()) {
        return IParallelHandler::ETryResult::NonSuitable;
    }
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::MARKET);
    marketCtx.SetResponseForm(ToString(NMarket::EChoiceForm::Native));
    marketCtx.SetState(ToString(NMarket::EChoiceState::Null));
    marketCtx.SetRequest(request);

    return TryDoImpl(marketCtx, true /* isParallelMode */);
}

////////////////////////////////////////////////////////////////////////////////

TResultValue TMarketRecurringPurchaseFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    const auto result = NMarket::TMarketRecurringPurchase(context, false /* isParallelMode */).TryDo();
    if (auto tryError = std::get_if<TError>(&result)) {
        return *tryError;
    }
    return TResultValue();
}

IParallelHandler::TTryResult TMarketRecurringPurchaseFormHandler::TryToHandle(TContext& ctx)
{
    NMarket::TMarketContext marketCtx(ctx, GetScenarioType());
    return NMarket::TMarketRecurringPurchase(marketCtx, true /* isParallelMode */).TryDo();
}

NMarket::EScenarioType TMarketRecurringPurchaseFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::RECURRING_PURCHASE;
}

bool TMarketRecurringPurchaseFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().RecurringPurchase();
}

TResultValue TMarketRecurringPurchaseFormHandler::ProcessScenarioDisabled(NMarket::TMarketContext& ctx)
{
    ctx.SetState(NMarket::ERecurringPurchaseState::Exit);
    return TMarketFormHandlerBase::ProcessScenarioDisabled(ctx);
}

void TMarketRecurringPurchaseFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx)
{
    auto cbMarketForm = []() {
        return MakeHolder<TMarketRecurringPurchaseFormHandler>();
    };

    const size_t threadsCount = globalCtx.Config().HasMarket() ? globalCtx.Config().Market().SearchThreads() : 1;

    if (threadsCount <= 1) {
        for (const auto& form: GetEnumAllValues<NMarket::ERecurringPurchaseForm>()) {
            handlers->emplace(ToString(form), cbMarketForm);
        }
        return;
    }

    auto threadPoolPtr = &globalCtx.MarketThreadPool();
    auto cbSearchForm = [=]() {
        return MakeHolder<TSearchFormHandler>(*threadPoolPtr);
    };
    auto cbMarketChoiceForm = []() {
        return MakeHolder<TMarketChoiceFormHandlerExternal>();
    };
    auto cbMarketSearchParallelForm = [=]() {
        return THolder<TParallelHandlerExecutor>(
                new TParallelHandlerExecutor(*threadPoolPtr, {cbMarketForm, cbMarketChoiceForm, cbSearchForm}));
    };
    for (const auto& form: GetEnumAllValues<NMarket::ERecurringPurchaseForm>()) {
        switch (form) {
            case NMarket::ERecurringPurchaseForm::Activation:
                handlers->emplace(ToString(form), cbMarketSearchParallelForm);
                break;
            default:
                handlers->emplace(ToString(form), cbMarketForm);
                break;
        }
    }
}

////////////////////////////////////////////////////////////////////////////////
// how much implementation

TResultValue TMarketHowMuchFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    NMarket::TMarketHowMuchRequestImpl impl(context);
    TResultValue res = impl.Do();
    context.AddSearchSuggest();
    context.AddOnboardingSuggest();
    return res;
}

NMarket::EScenarioType TMarketHowMuchFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::HOW_MUCH;
}

bool TMarketHowMuchFormHandler::IsProtocolActivationByVins(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().HowMuchActivateProtocolScenarioByVins()
                && ctx.GetCalledFrom() != "hollywood";
}

bool TMarketHowMuchFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().HowMuch();
}

void TMarketHowMuchFormHandler::Register(THandlersMap* handlers, IGlobalContext& globalCtx)
{
    // initialize and update stop words and categories by scheduler
    NMarket::TStopWords::Start(globalCtx);
    NMarket::TStopCategories::Start(globalCtx);

    auto cbMarketForm = []() {
        return MakeHolder<TMarketHowMuchFormHandler>();
    };
    for (const auto& form: GetEnumAllValues<NMarket::EHowMuchForm>()) {
        handlers->emplace(ToString(form), cbMarketForm);
    }
}


void TMarketHowMuchFormHandler::SetAsResponse(TContext& ctx)
{
    const TString& previousFormName = ctx.FormName();
    TContext::TPtr newCtx = ctx.SetResponseForm(ToString(NMarket::EHowMuchForm::HowMuch), false);
    Y_ENSURE(newCtx);
    NMarket::TMarketContext marketCtx(*newCtx, NMarket::EScenarioType::HOW_MUCH);
    marketCtx.Log(TStringBuilder() << "Change form to \""
                  << newCtx->FormName() << "\" from \"" << previousFormName << "\"");

    if (marketCtx.GetExperiments().AdvBeruScenarioInHowMuch()) {
        marketCtx.SetCalledFrom(TStringBuf("search"));
    }

    static const TVector<TString> HOW_MUCH_PHRASES {
        "сколько стоит",
        "сколько стоят",
        "почем сейчас",
        "почем щас",
        "почем цена",
        "почем стоит",
        "почем",
        "сколько денег стоит",
        "сколько денег",
        "сколько просят за",
        "за сколько можно купить",
        "какая цена у",
        "какая цена",
        "в какую цену",
        "по какой цене",
        "цена на",
        "цена",
        "цены на",
        "цены",
        "какая стоимость у",
        "какая стоимость",
        "какова стоимость y",
        "какова стоимость",
        "в какую стоимость",
        "стоимость",
        "стоит",
        "?",
    };
    marketCtx.SetRequest(NMarket::ClearRequest(marketCtx.Request(), HOW_MUCH_PHRASES));
}


bool TMarketHowMuchFormHandler::MustHandle(TStringBuf query)
{
    static const TVector<TStringBuf> HOW_MUCH_PHRASES {
        TStringBuf("сколько"),
        TStringBuf("почем"),
        TStringBuf("цена"),
        TStringBuf("цену"),
        TStringBuf("цене"),
        TStringBuf("цены"),
        TStringBuf("стоимость"),
        TStringBuf("стоит"),
        TStringBuf("стоят"),
    };

    for (const auto& phrase: HOW_MUCH_PHRASES) {
        if (query.Contains(phrase))
            return true;
    }
    return false;
}

////////////////////////////////////////////////////////////////////////////////
void TMarketOrdersStatusFormHandler::Register(THandlersMap* handlers)
{
    auto cbMarketForm = []() {
        return MakeHolder<TMarketOrdersStatusFormHandler>();
    };

    for (const auto& currForm: GetEnumAllValues<NMarket::EMarketOrdersStatusForm>()) {
        handlers->emplace(ToString(currForm), cbMarketForm);
    }
}

TResultValue TMarketOrdersStatusFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    NMarket::TMarketOrdersStatusImpl statusImpl(context);

    return statusImpl.Do();
}

NMarket::EScenarioType TMarketOrdersStatusFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::ORDERS_STATUS;
}

bool TMarketOrdersStatusFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().OrdersStatus();
}

////////////////////////////////////////////////////////////////////////////////
void TMarketBeruMyBonusesListFormHandler::Register(THandlersMap* handlers)
{
    auto cbMarketForm = []() {
        return MakeHolder<TMarketBeruMyBonusesListFormHandler>();
    };

    for (const auto& currForm: GetEnumAllValues<NMarket::EMarketBeruBonusesForm>()) {
        handlers->emplace(ToString(currForm), cbMarketForm);
    }
}

NMarket::EScenarioType TMarketBeruMyBonusesListFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::BERU_BONUSES;
}

bool TMarketBeruMyBonusesListFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().BeruMyBonusesList();
}

TResultValue TMarketBeruMyBonusesListFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    NMarket::TMarketBeruMyBonusesListImpl bonusesImpl(context);
    TResultValue res = bonusesImpl.Do();

    return res;
}

////////////////////////////////////////////////////////////////////////////////
// shopping list

void TShoppingListFormHandler::Register(THandlersMap* handlers)
{
    auto cbMarketForm = []() { return MakeHolder<TShoppingListFormHandler>(); };

    for (const auto& currForm: GetEnumAllValues<NMarket::EShoppingListForm>()) {
        handlers->emplace(ToString(currForm), cbMarketForm);
    }
}

NMarket::EScenarioType TShoppingListFormHandler::GetScenarioType() const
{
    return NMarket::EScenarioType::SHOPPING_LIST;
}

bool TShoppingListFormHandler::IsEnabled(const NMarket::TMarketContext& ctx) const
{
    return ctx.GetExperiments().ShoppingList();
}

TResultValue TShoppingListFormHandler::DoImpl(NMarket::TMarketContext& context)
{
    return NMarket::TShoppingListImpl(context).Do();
}

////////////////////////////////////////////////////////////////////////////////

}  // namespace NBASS
