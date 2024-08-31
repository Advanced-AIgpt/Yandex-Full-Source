#pragma once

#include "vins.h"
#include "market/types.h"
#include "parallel_handler.h"

namespace NBASS {

class IGlobalContext;

namespace NMarket {

class TMarketContext;

} //namespace NMarket

////////////////////////////////////////////////////////////////////////////////

class TMarketFormHandlerBase: public IHandler {
public:
    TMarketFormHandlerBase();
    virtual ~TMarketFormHandlerBase();

    TResultValue Do(TRequestHandler& r) override;

protected:
    virtual TResultValue DoImpl(NMarket::TMarketContext& context) = 0;
    virtual NMarket::EScenarioType GetScenarioType() const = 0;
    virtual bool IsProtocolActivationByVins(const NMarket::TMarketContext& ctx) const;
    virtual TResultValue ProcessProtocolActivation(NMarket::TMarketContext& ctx);
    virtual bool IsEnabled(const NMarket::TMarketContext& ctx) const = 0;
    virtual TResultValue ProcessScenarioDisabled(NMarket::TMarketContext& ctx);

    TResultValue ProcessError(TRequestHandler& r, TStringBuf errMsg, const TBackTrace* trace = nullptr);
};

////////////////////////////////////////////////////////////////////////////////

class TMarketHowMuchFormHandler: public TMarketFormHandlerBase {
public:
    static void Register(THandlersMap* handlers, IGlobalContext& globalCtx);
    static bool MustHandle(TStringBuf query);
    static void SetAsResponse(TContext& ctx);

protected:
    virtual TResultValue DoImpl(NMarket::TMarketContext& context);
    virtual NMarket::EScenarioType GetScenarioType() const;
    virtual bool IsProtocolActivationByVins(const NMarket::TMarketContext& ctx) const;
    virtual bool IsEnabled(const NMarket::TMarketContext& ctx) const;
};

////////////////////////////////////////////////////////////////////////////////

class TMarketChoiceFormHandler: public TMarketFormHandlerBase, public IParallelHandler {
public:
    static void Register(THandlersMap* handlers, IGlobalContext& globalCtx);

    // IParallelHandler overrides:
    TTryResult TryToHandle(TContext& ctx) override;
    TString Name() const override { return "market"; }

protected:
    TResultValue DoImpl(NMarket::TMarketContext& context) override;
    NMarket::EScenarioType GetScenarioType() const override;
    bool IsEnabled(const NMarket::TMarketContext& ctx) const override;
    TResultValue ProcessScenarioDisabled(NMarket::TMarketContext& ctx) override;

    TTryResult TryDoImpl(NMarket::TMarketContext& context, bool isParallelMode);
};

////////////////////////////////////////////////////////////////////////////////

class TMarketRecurringPurchaseFormHandler: public TMarketFormHandlerBase, public IParallelHandler {
public:
    static void Register(THandlersMap* handlers, IGlobalContext& globalCtx);

    // IParallelHandler overrides:
    TTryResult TryToHandle(TContext& ctx) override;
    TString Name() const override { return "market_rp"; }

protected:
    TResultValue DoImpl(NMarket::TMarketContext& context) override;
    NMarket::EScenarioType GetScenarioType() const override;
    bool IsEnabled(const NMarket::TMarketContext& ctx) const override;
    TResultValue ProcessScenarioDisabled(NMarket::TMarketContext& ctx) override;
};

////////////////////////////////////////////////////////////////////////////////

class TMarketOrdersStatusFormHandler: public TMarketFormHandlerBase {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual TResultValue DoImpl(NMarket::TMarketContext& context);
    virtual NMarket::EScenarioType GetScenarioType() const;
    virtual bool IsEnabled(const NMarket::TMarketContext& ctx) const;
};
////////////////////////////////////////////////////////////////////////////////

class TMarketBeruMyBonusesListFormHandler: public TMarketFormHandlerBase {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual TResultValue DoImpl(NMarket::TMarketContext& context);
    virtual NMarket::EScenarioType GetScenarioType() const;
    virtual bool IsEnabled(const NMarket::TMarketContext& ctx) const;
};

////////////////////////////////////////////////////////////////////////////////

class TShoppingListFormHandler: public TMarketFormHandlerBase {
public:
    static void Register(THandlersMap* handlers);

protected:
    virtual TResultValue DoImpl(NMarket::TMarketContext& context);
    virtual NMarket::EScenarioType GetScenarioType() const;
    virtual bool IsEnabled(const NMarket::TMarketContext& ctx) const;
};

////////////////////////////////////////////////////////////////////////////////

} // namespace NBASS
