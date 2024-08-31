#pragma once

#include "dialog_config.h"
#include "handy_response_builder.h"

#include <alice/hollywood/library/base_scenario/scenario.h>
#include <alice/hollywood/library/request/request.h>
#include <alice/hollywood/library/scenarios/food/menu_matcher/matcher.h>
#include <alice/hollywood/library/scenarios/food/proto/state.pb.h>
#include <alice/megamind/protos/scenarios/request.pb.h>

namespace NAlice::NHollywood::NFood {

// ~~~~ TRunProcessor ~~~~

class TRunProcessor : public TMoveOnly {
public:
    // Stages:
    //   isPreparing == true:  Creates needed requests to backend. Nothing more.
    //   isPreparing == false: Main work.
    TRunProcessor(bool isPreparing, const TMenuMatcher& menuMatcher, TScenarioHandleContext* ctx);

    void Process();

private:
    void DumpRequest() const;
    void ClearShortMemoryIfNeeded();
    TDuration CalcResponseTimeDelta() const;
    TEventKey FindBestEvent() const;
    void FindBestFrameFromGroup(const TString& groupName, TEventKey* best) const;
    bool Matches(const TEventKey& event, TStringBuf groupName, TStringBuf frameName);
    void CheckScannedEventHandlers();
    void ProcessFallback();
    bool TryWriteFallbackResponse();
    void ProcessResetScenario();
    void ProcessStartMc(const TSemanticFrame* frame = nullptr);
    TString TryFindPlace();
    void StartMakeOrderScenario();
    void StartMakeOrderScenarioWithNewItems(const TSemanticFrame& frame);
    void ProcessResumeOrder();
    void IncrementOnboardingCounter();
    void ProcessAddItem(const TSemanticFrame& frame, bool isStart);
    bool TryGetMenu(const TEventKey& event);
    void DumpMenu();
    void ProcessRemoveItem(const TSemanticFrame& frame);
    size_t FindCartItemToRemove(const NApi::TCart& cart, const THashMap<TString, TString>& slots);
    size_t FindCartItemToRemove(const NApi::TCart& cart, TStringBuf itemToRemove);
    void ProcessDeclineKeepOldCart();
    void ProcessClearCart();
    void ProcessShowCart();
    void AddCartToNlgCtx(const NApi::TCart& cart);
    void ProcessGoToApp();
    bool TryPushCartForApplication();
    void RedirectToApp(const TString& pushTitle, const TString& pushBody, const TLocation& deliveryLocation);
    void RedirectToAppOnError(const TLocation& deliveryLocation);
    void RedirectToAppOnError();
    bool TryRedirectToAppWithCart(const TString& pushTitle, const TString& pushBody, const TLocation& deliveryLocation);
    bool TryRedirectToAppToPay(const TLocation& deliveryLocation);
    bool TryRedirectToAppToContinue(const TLocation& deliveryLocation);
    bool TryRedirectToAppToPay();
    bool TryRedirectToAppToContinue();
    void ProcessNothingElse();
    void ProcessBeginNewCart();
    void ProcessFormOrderAgree();
    void ProcessConfirmOrderAgree();
    void ProcessRepeatLastOrder();
    NJson::TJsonValue TryGetLastOrder();
    bool IsNear(const NJson::TJsonValue& lastDeliveryLocationJson) const;
    static NJson::TJsonValue ProcessCartOptions(NJson::TJsonValue json);
    void WriteInternalErrorResponse(TStringBuf errorMsg);
    void WriteNormalResponse(TStringBuf responseName);
    void WriteCartUpdatedResponse(TStringBuf responseName);
    static bool HasFallback(const TResponseConfig& config);
    void WriteBasicResponse(TStringBuf responseName, const TVector<TString>& expectedFrameGroups, const TVector<TString>& suggests, i32 fallbackCounter);
    void WriteStateResponseInfo(TStringBuf responseName, const TVector<TString>& expectedFrameGroups, const TVector<TString>& suggests, i32 fallbackCounter);
    void AddActionsToResponse(const TVector<TString>& expectedFrameGroups);
    TVector<TString> GetShuffledPopularDishes();
    void AddSuggestsToResponse(const TVector<TString>& suggests);
    const TState::TShortMemory& ShortMemory() const;
    TState::TShortMemory& ShortMemory();
    void UpdateTaxiUid(const TString& taxiUid);
    void UpdatePlaceSlug(const TString& placeSlug);
    bool InsertIfRepeated(const NApi::TCart::TItem& item, NApi::TCart& cart);
    void ClearPlaceSlugIfNeeded();
    bool IsRequestActive();

private:
    const bool IsPreparing = false;
    const TMenuMatcher& MenuMatcher;
    TScenarioHandleContext& Ctx;
    const NScenarios::TScenarioRunRequest RequestProto;
    const TScenarioRunRequestWrapper Request;
    const bool RedirectByLink = false;
    TState State;
    THandyResponseBuilder Response;
    bool IsFrameHandled = false;
    NJson::TJsonValue Menu;
    TVector<TString> ScannedEventHandlers; // For debug
};

// ~~~~ TRunProcessorHandle ~~~~

class TRunProcessorHandle : public TScenario::THandleBase {
public:
    TRunProcessorHandle(TStringBuf name, bool isPreparing);

    TString Name() const override;
    void Do(TScenarioHandleContext& ctx) const override;

private:
    TString Name_;
    bool IsPreparing = false;
    TMenuMatcher MenuMatcher;
};

// ~~~~ TRunPrepareHandle ~~~~

class TRunPrepareHandle : public TRunProcessorHandle {
public:
    TRunPrepareHandle()
        : TRunProcessorHandle("run_prepare", true)
    {
    }
};

// ~~~~ TRunRenderHandle ~~~~

class TRunRenderHandle : public TRunProcessorHandle {
public:
    TRunRenderHandle()
        : TRunProcessorHandle("run_render", false)
    {
    }
};

} // namespace NAlice::NHollywood::NFood
