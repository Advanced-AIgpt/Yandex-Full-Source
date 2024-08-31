#include "food_run.h"

#include <alice/hollywood/library/scenarios/food/backend/proto/requests.pb.h>
#include <alice/hollywood/library/scenarios/food/backend/get_address.h>
#include <alice/hollywood/library/scenarios/food/backend/get_last_order_pa.h>
#include <alice/hollywood/library/scenarios/food/backend/get_menu_pa.h>
#include <alice/hollywood/library/scenarios/food/proto/apply_arguments.pb.h>
#include <alice/hollywood/library/scenarios/food/proto/address.pb.h>
#include <alice/hollywood/library/scenarios/food/proto/cart.pb.h>
#include <alice/hollywood/library/scenarios/food/proto/state.pb.h>

#include <alice/megamind/protos/blackbox/blackbox.pb.h>
#include <alice/megamind/protos/common/app_type.pb.h>
#include <alice/megamind/protos/common/location.pb.h>

#include <alice/library/json/json.h>
#include <alice/library/parsed_user_phrase/parsed_sequence.h>
#include <alice/library/proto/proto.h>
#include <alice/megamind/library/util/wildcards.h>
#include <alice/nlu/granet/lib/utils/json_utils.h>
#include <alice/nlu/granet/lib/utils/utils.h>
#include <alice/nlu/libs/request_normalizer/request_normalizer.h>
#include <library/cpp/iterator/enumerate.h>
#include <util/charset/wide.h>
#include <util/generic/adaptor.h>
#include <util/generic/algorithm.h>
#include <util/random/shuffle.h>
#include <util/string/cast.h>
#include <util/string/join.h>

namespace NAlice::NHollywood::NFood {

using namespace NAlice::NScenarios;
using namespace std::literals;

namespace {

const TString EXP_HW_FOOD_HARDCODED_MENU = "hw_food_hardcoded_menu";

constexpr TStringBuf EDA_APP_CART_URI = "https://eda.yandex/cart";

THashMap<TString, TString> ToSlotMap(const TSemanticFrame& frame) {
    THashMap<TString, TString> slots;
    for (const TSemanticFrame::TSlot& slot : frame.GetSlots()) {
        slots[slot.GetName()] = slot.GetValue();
    }
    return slots;
}

// Add elements of Container to protobuf repeated field
template<class Container, class Element>
void ExtendRepeatedField(const Container& src, ::google::protobuf::RepeatedPtrField<Element>* dest) {
    Y_ASSERT(dest);
    for (const auto& item : src) {
        *dest->Add() = item;
    }
}

// Copy Container to protobuf repeated field
template<class Container, class Element>
void CopyToRepeatedField(const Container& src, ::google::protobuf::RepeatedPtrField<Element>* dest) {
    Y_ASSERT(dest);
    dest->Clear();
    ExtendRepeatedField(src, dest);
}

} // namespace

// ~~~~ TRunProcessor ~~~~

TRunProcessor::TRunProcessor(bool isPreparing, const TMenuMatcher& menuMatcher, TScenarioHandleContext* ctx)
    : IsPreparing(isPreparing)
    , MenuMatcher(menuMatcher)
    , Ctx(*ctx)
    , RequestProto(GetOnlyProtoOrThrow<TScenarioRunRequest>(Ctx.ServiceCtx, REQUEST_ITEM))
    , Request(RequestProto, Ctx.ServiceCtx)
    , RedirectByLink(Request.Interfaces().GetCanOpenLink())
    , State(Request.LoadState<TState>())
    , Response(Ctx, Request, "food"sv)
{
    Response.NlgCtx()["redirect_by_link"] = RedirectByLink;
}

void TRunProcessor::Process() {
    DumpRequest();
    ClearShortMemoryIfNeeded();
    ClearPlaceSlugIfNeeded();

    const TEventKey event = FindBestEvent();

    if (!event.IsDefined()) {
        ProcessFallback();
        return;
    }

    const auto frameProto = Request.Input().FindSemanticFrame(event.FrameName);
    Y_ENSURE(frameProto);
    LOG_INFO(Ctx.Ctx.Logger()) << "Best frame: " << JsonFromProto(*frameProto);

    const TFrame frame = TFrame::FromProto(*frameProto);

    if (!TryGetMenu(event)) {
        return;
    }

    // Group 'main'
    if (Matches(event, "main"sv, "alice.food.main.reset_scenario"sv)) {
        ProcessResetScenario();
    }
    if (Matches(event, "main"sv, "alice.food.main.start_mc"sv)) {
        ProcessStartMc();
    }
    if (Matches(event, "main"sv, "alice.food.main.start_mc_add_item"sv)) {
        ProcessStartMc(frameProto.Get());
    }
    if (Matches(event, "main"sv, "alice.food.main.can_order"sv)) {
        WriteNormalResponse("nlg_how_to_order_outside"sv);
    }
    if (Matches(event, "main"sv, "alice.food.main.can_order_mc"sv)) {
        WriteNormalResponse("nlg_how_to_order_outside"sv);
    }
    if (Matches(event, "main"sv, "alice.food.main.repeat_last_order"sv)) {
        ProcessRepeatLastOrder();
    }

    // Group 'exit'
    if (Matches(event, "exit"sv, "alice.food.exit.exit_scenario"sv)) {
        WriteNormalResponse("nlg_cancel_order"sv);
    }

    // Group 'cart'
    if (Matches(event, "cart"sv, "alice.food.main.start_mc_add_item"sv)) {
        ProcessAddItem(*frameProto, false);
    }
    if (Matches(event, "cart"sv, "alice.food.cart.what_you_can"sv) && IsRequestActive()) {
        WriteNormalResponse("nlg_how_to_order_inside"sv);
    }
    if (Matches(event, "cart"sv, "alice.food.cart.clear_cart"sv) && IsRequestActive()) {
        ProcessClearCart();
    }
    if (Matches(event, "cart"sv, "alice.food.cart.show_cart"sv) && IsRequestActive()) {
        ProcessShowCart();
    }
    if (Matches(event, "cart"sv, "alice.food.cart.add_item"sv) && IsRequestActive()) {
        ProcessAddItem(*frameProto, false);
    }
    if (Matches(event, "cart"sv, "alice.food.cart.remove_item"sv) && IsRequestActive()) {
        ProcessRemoveItem(*frameProto);
    }
    if (Matches(event, "cart"sv, "alice.food.main.can_order"sv)) {
        WriteNormalResponse("nlg_how_to_order_inside"sv);
    }
    if (Matches(event, "cart"sv, "alice.food.main.can_order_mc"sv)) {
        WriteNormalResponse("nlg_how_to_order_inside"sv);
    }

    if (Matches(event, "cart"sv, "alice.food.cart.where_from_order"sv) && IsRequestActive()) {
        WriteNormalResponse("nlg_order_from_mcdonalds"sv);
    }

    // Group 'what_you_wish'
    if (Matches(event, "what_you_wish"sv, "alice.food.common.nothing"sv)) {
        ProcessNothingElse();
    }

    // Group 'what_you_wish_no_fallback'
    if (Matches(event, "what_you_wish_no_fallback"sv, "alice.food.common.nothing"sv)) {
        WriteNormalResponse("silent_exit_scenario"sv);
    }

    // Group 'something_else'
    if (Matches(event, "something_else"sv, "alice.food.common.agree"sv)) {
        WriteNormalResponse("nlg_what_you_wish"sv);
    }
    if (Matches(event, "something_else"sv, "alice.food.common.decline"sv)) {
        ProcessNothingElse();
    }
    if (Matches(event, "something_else"sv, "alice.food.common.nothing"sv)) {
        ProcessNothingElse();
    }

    // Group 'keep_old_cart'
    if (Matches(event, "keep_old_cart"sv, "alice.food.keep_old_cart.agree"sv)) {
        ProcessResumeOrder();
    }
    if (Matches(event, "keep_old_cart"sv, "alice.food.keep_old_cart.decline"sv)) {
        ProcessDeclineKeepOldCart();
    }
    if (Matches(event, "keep_old_cart"sv, "alice.food.keep_old_cart.show_cart"sv)) {
        ProcessShowCart();
    }

    // Group 'begin_new_cart'
    if (Matches(event, "begin_new_cart"sv, "alice.food.common.agree"sv)) {
        ProcessBeginNewCart();
    }
    if (Matches(event, "begin_new_cart"sv, "alice.food.common.decline"sv)) {
        WriteNormalResponse("silent_exit_scenario"sv);
    }

    // Group 'form_order'
    if (Matches(event, "form_order"sv, "alice.food.form_order.agree"sv)) {
        ProcessFormOrderAgree();
    }
    if (Matches(event, "form_order"sv, "alice.food.form_order.decline"sv)) {
        WriteNormalResponse("nlg_form_order_decline"sv);
    }

    // Group 'confirm_order'
    if (Matches(event, "confirm_order"sv, "alice.food.confirm_order.agree"sv)) {
        ProcessConfirmOrderAgree();
    }
    if (Matches(event, "confirm_order"sv, "alice.food.confirm_order.decline"sv)) {
        WriteNormalResponse("nlg_confirm_order_decline"sv);
    }

    // Group 'go_to_app'
    if (Matches(event, "go_to_app"sv, "alice.food.common.agree"sv)) {
        ProcessGoToApp();
    }
    if (Matches(event, "go_to_app"sv, "alice.food.common.decline"sv)) {
        WriteNormalResponse("nlg_ok_what_you_wish"sv);
    }

    Y_IF_DEBUG(CheckScannedEventHandlers());

    if (!IsFrameHandled) {
        Response.SetIsIrrelevant(true);
        WriteInternalErrorResponse("Unknown event");
    }
}

void TRunProcessor::DumpRequest() const {
    LOG_INFO(Ctx.Ctx.Logger()) << "Utterance: " << Request.Input().Utterance();
    LOG_INFO(Ctx.Ctx.Logger()) << "State: " << JsonFromProto(State);
    TVector<TString> frameNames;
    for (const TSemanticFrame& frame : RequestProto.GetInput().GetSemanticFrames()) {
        frameNames.push_back(frame.GetName());
    }
    LOG_INFO(Ctx.Ctx.Logger()) << "Frames in request: " << JoinSeq(", ", frameNames);
}

void TRunProcessor::ClearShortMemoryIfNeeded() {
    if (CalcResponseTimeDelta() >= GetDialogConfig().ShortMemoryTtl) {
        LOG_INFO(Ctx.Ctx.Logger()) << "Clear short memory";
        State.ClearShortMemory();
    }
}

void TRunProcessor::ClearPlaceSlugIfNeeded() {
    if (CalcResponseTimeDelta() >= GetDialogConfig().PlaceSlugTtl) {
        LOG_INFO(Ctx.Ctx.Logger()) << "Clear place slug";
        ShortMemory().ClearPlaceSlug();
    }
}

TDuration TRunProcessor::CalcResponseTimeDelta() const {
    const TInstant prevTime = TInstant::MilliSeconds(ShortMemory().GetResponseInfo().GetServerTimeMs());
    const TInstant currTime = TInstant::MilliSeconds(RequestProto.GetBaseRequest().GetServerTimeMs());
    return currTime - prevTime;
}

bool TRunProcessor::IsRequestActive() {
    return CalcResponseTimeDelta() <= GetDialogConfig().RequestTtl;
}

TEventKey TRunProcessor::FindBestEvent() const {
    TEventKey best;
    for (const TString& groupName : Reversed(ShortMemory().GetResponseInfo().GetExpectedFrameGroups())) {
        FindBestFrameFromGroup(groupName, &best);
    }
    if (!best.IsDefined()) {
        FindBestFrameFromGroup("main", &best);
    }
    LOG_INFO(Ctx.Ctx.Logger()) << "Best event: " << best.GroupName << "/" << best.FrameName;
    return best;
}

void TRunProcessor::FindBestFrameFromGroup(const TString& groupName, TEventKey* best) const {
    Y_ENSURE(best);
    const TDialogConfig& config = GetDialogConfig();
    const TFrameGroupConfig* group = config.FrameGroups.FindPtr(groupName);
    if (group == nullptr) {
        return;
    }
    for (const TString& frameName : group->Frames) {
        const auto frame = Request.Input().FindSemanticFrame(frameName);
        if (frame == nullptr) {
            continue;
        }
        if (!best->IsDefined() || (config.IsFrameWeak(best->FrameName) && !config.IsFrameWeak(frameName))) {
            *best = {
                .GroupName = groupName,
                .FrameName = frameName,
            };
        }
    }
}

bool TRunProcessor::Matches(const TEventKey& event, TStringBuf groupName, TStringBuf frameName) {
    Y_IF_DEBUG(ScannedEventHandlers.push_back(TString::Join(groupName, "/", frameName)));
    if (!event.IsEqual(groupName, frameName)) {
        return false;
    }
    Y_ENSURE(!IsFrameHandled, "Duplicated event handler for " << groupName << "/" << frameName);
    IsFrameHandled = true;
    return true;
}

void TRunProcessor::CheckScannedEventHandlers() {
    TVector<TString> eventsInConfig;
    for (const auto& [groupName, groupConfig] : GetDialogConfig().FrameGroups) {
        for (const auto& frameName : groupConfig.Frames) {
            eventsInConfig.push_back(TString::Join(groupName, "/", frameName));
        }
    }
    Sort(eventsInConfig);
    Sort(ScannedEventHandlers);
    if (ScannedEventHandlers == eventsInConfig) {
        return;
    }
    LOG_ERROR(Ctx.Ctx.Logger()) << "List of event handlers does not correspond to list of events in config.\n"
        << "  List of events in config:\n    \n" << JoinSeq("    \n", eventsInConfig) << "\n"
        << "  List of event handlers:\n    \n" << JoinSeq("    \n", ScannedEventHandlers);
}

void TRunProcessor::ProcessFallback() {
    if (IsPreparing) {
        return;
    }
    if (!TryWriteFallbackResponse()) {
        Response.SetIsIrrelevant(true);
        WriteNormalResponse("silent_irrelevant"sv);
    }
}

bool TRunProcessor::TryWriteFallbackResponse() {
    if (CalcResponseTimeDelta() >= GetDialogConfig().FallbackTtl) {
        return false;
    }

    const TState::TShortMemory::TResponseInfo& prev = ShortMemory().GetResponseInfo();

    const TVector<TString> expectedFrameGroups = NGranet::ToVector<TString>(prev.GetExpectedFrameGroups());
    for (const TString& groupName : expectedFrameGroups) {
        if (!GetDialogConfig().FrameGroups.contains(groupName)) {
            return false; // State is not compatible
        }
    }

    TString fallbackNlg;
    for (const TString& groupName : Reversed(expectedFrameGroups)) {
        fallbackNlg = GetDialogConfig().GetFrameGroup(groupName).FallbackNlg;
        if (!fallbackNlg.empty()) {
            break;
        }
    }
    if (fallbackNlg.empty()) {
        return false;
    }

    const i32 fallbackCounter = prev.GetFallbackCounter() + 1;
    if (fallbackCounter > 2) {
        return false;
    }

    const TVector<TString> suggests = GetDialogConfig().GetResponse(prev.GetResponseName()).Suggests;

    Response.RenderNlg(fallbackNlg);
    Response.ResponseBody().SetExpectsRequest(true);
    Response.SetShouldListen(true);

    WriteBasicResponse(fallbackNlg, expectedFrameGroups, suggests, fallbackCounter);
    return true;
}

void TRunProcessor::ProcessResetScenario() {
    if (IsPreparing) {
        return;
    }
    State = {};
    WriteNormalResponse("nlg_reset_scenario_ok"sv);
}

void TRunProcessor::ProcessStartMc(const TSemanticFrame* frame) {
    if (TryFindPlace().empty()) {
        return;
    }
    if (frame) {
        StartMakeOrderScenarioWithNewItems(*frame);
    } else {
        StartMakeOrderScenario();
    }
}

TString TRunProcessor::TryFindPlace() {
    if (Request.HasExpFlag(EXP_HW_FOOD_HARDCODED_MENU)) {
        return IsPreparing ? "" : "mcdonalds_komsomolskyprospect";
    }
    if (IsPreparing) {
        const TBlackBoxUserInfo* userInfo = GetUserInfoProto(Request);
        Y_ENSURE(userInfo);
        NApiFindPlacePA::AddRequest(
            Ctx,
            Request.Location(),
            {
                .Phone = userInfo->GetPhone(),
                .YandexUid = userInfo->GetUid(),
                .TaxiUid = ShortMemory().GetAuth().GetTaxiUid()
            }
        );
        return "";
    }

    NApiFindPlacePA::EError error;
    TString placeSlug;
    const NApiFindPlacePA::TResponseData placeResponse = NApiFindPlacePA::ReadResponse(Ctx);
    UpdateTaxiUid(placeResponse.TaxiUid);
    error = placeResponse.Error;
    placeSlug = placeResponse.PlaceSlug;

    if (!placeSlug.empty()) {
        return placeSlug;
    }
    if (error == NApiFindPlacePA::EError::SUCCESS) {
        WriteInternalErrorResponse("Empty place slug in successful response."sv);
    } else if (error == NApiFindPlacePA::EError::AUTHORIZATION_FAILED) {
        RedirectToAppOnError();
        WriteNormalResponse("nlg_no_response_from_eda"sv);
    } else if (error == NApiFindPlacePA::EError::NO_RESPONSE) {
        if (TryRedirectToAppToContinue()) {
            WriteNormalResponse("nlg_no_response_from_eda"sv);
        }
    } else if (error == NApiFindPlacePA::EError::PLACE_NOT_FOUND) {
        WriteNormalResponse("nlg_mcdonalds_not_found"sv);
    } else {
        WriteInternalErrorResponse("Invalid NApiFindPlacePA::EError."sv);
    }
    return {};
}

void TRunProcessor::StartMakeOrderScenario() {
    IncrementOnboardingCounter();
    const auto& cart = ShortMemory().GetCart();
    if (!cart.GetItems().empty()) {
        AddCartToNlgCtx(cart);
        WriteNormalResponse("nlg_keep_old_cart"sv);
    } else if (State.GetLongMemory().GetOnboardingCounter() > 2) {
        WriteNormalResponse("nlg_what_you_wish"sv);
    } else {
        WriteNormalResponse("nlg_start_onboarding"sv);
    }
}

void TRunProcessor::StartMakeOrderScenarioWithNewItems(const TSemanticFrame& frame) {
    if (IsPreparing) {
        return;
    }

    IncrementOnboardingCounter();
    const auto& cart = ShortMemory().GetCart();
    if (!cart.GetItems().empty()) {
        AddCartToNlgCtx(cart);
        (*ShortMemory().MutableLastSemanticFrame()) = frame;
        WriteNormalResponse("nlg_keep_old_cart"sv);
    } else {
        ProcessAddItem(frame, true);
    }
}

void TRunProcessor::IncrementOnboardingCounter() {
    State.MutableLongMemory()->SetOnboardingCounter(State.GetLongMemory().GetOnboardingCounter() + 1);
}

void TRunProcessor::ProcessResumeOrder() {
    if (IsPreparing) {
        return;
    }

    if (ShortMemory().HasLastSemanticFrame()) {
        ProcessAddItem(ShortMemory().GetLastSemanticFrame(), false);
        ShortMemory().ClearLastSemanticFrame();
    } else {
        WriteNormalResponse("nlg_cart_resume_order"sv);
    }
}

void TRunProcessor::ProcessAddItem(const TSemanticFrame& frame, bool isStart) {
    if (IsPreparing) {
        return;
    }

    NApi::TCart newItems;
    TVector<TString> unknownItems;
    TVector<TString> unavailableItems;
    BuildApiCartFromSlots(MenuMatcher, Menu, ToSlotMap(frame), &newItems, &unknownItems, &unavailableItems);

    NApi::TCart& cart = *ShortMemory().MutableCart();
    const bool wasCartEmpty = cart.GetItems().empty();
    for (const auto& item : newItems.GetItems()) {
        if (!InsertIfRepeated(item, cart)) {
            *cart.AddItems() = item;
        }
    }

    AddCartToNlgCtx(cart);
    NJson::TJsonValue& nlgCtx = Response.NlgCtx();
    nlgCtx["new_items"] = JsonFromProto(newItems);
    nlgCtx["unknown_items"] = NGranet::NJsonUtils::ToJson(unknownItems);
    nlgCtx["unavailable_items"] = NGranet::NJsonUtils::ToJson(unavailableItems);

    if (!unknownItems.empty()) {
        if (RedirectByLink) {
            if (TryRedirectToAppToContinue()) {
                WriteNormalResponse("nlg_cart_add_unknown_go_to_app"sv);
            }
        } else {
            WriteNormalResponse("nlg_cart_add_unknown"sv);
        }
    } else if (!newItems.GetItems().empty()) {
        if (wasCartEmpty || isStart) {
            WriteNormalResponse("nlg_cart_add_first_items"sv);
        } else {
            WriteNormalResponse("nlg_cart_add_new_items"sv);
        }
    } else if (!unavailableItems.empty()) {
        WriteNormalResponse("nlg_cart_add_unavailable"sv);
    } else if (isStart) {
        StartMakeOrderScenario();
    } else {
        WriteNormalResponse("nlg_what_you_wish"sv);
    }
}

bool TRunProcessor::TryGetMenu(const TEventKey& event) {
    if (IsFrameClean(event)) {
        return true;
    }

    if (Request.HasExpFlag(EXP_HW_FOOD_HARDCODED_MENU)) {
        if (IsPreparing) {
            return true;
        }
        Menu = ReadHardcodedMenuSample();
        DumpMenu();
        return true;
    }

    // TODO(samoylovboris) Cache menu in state
    // TODO(samoylovboris) Error message for timeout

    if (IsPreparing) {
        const TBlackBoxUserInfo* userInfo = GetUserInfoProto(Request);
        Y_ENSURE(userInfo);
        NApiGetMenuPA::AddRequest(
            Ctx,
            Request.Location(),
            {
                .Phone = userInfo->GetPhone(),
                .YandexUid = userInfo->GetUid(),
                .TaxiUid = ShortMemory().GetAuth().GetTaxiUid()
            },
            ShortMemory().GetPlaceSlug()
        );
        return true;
    }

    NApiGetMenuPA::EError error;
    const NApiGetMenuPA::TResponseData menuResponse = NApiGetMenuPA::ReadResponse(Ctx);
    UpdateTaxiUid(menuResponse.TaxiUid);
    UpdatePlaceSlug(menuResponse.PlaceSlug);
    error = menuResponse.Error;
    Menu = std::move(menuResponse.Menu["payload"]);

    if (Menu.IsDefined()) {
        DumpMenu();
        return true;
    }

    if (error == NApiGetMenuPA::EError::SUCCESS) {
        WriteInternalErrorResponse("Empty menu in successful response."sv);
    } else if (error == NApiGetMenuPA::EError::AUTHORIZATION_FAILED) {
        RedirectToAppOnError();
        WriteNormalResponse("nlg_no_response_from_eda"sv);
    } else if (error == NApiGetMenuPA::EError::NO_RESPONSE) {
        if (TryRedirectToAppToContinue()) {
            WriteNormalResponse("nlg_no_response_from_eda"sv);
        }
    } else if (error == NApiGetMenuPA::EError::PLACE_NOT_FOUND) {
        WriteNormalResponse("nlg_mcdonalds_not_found"sv);
    } else {
        WriteInternalErrorResponse("Invalid NApiGetMenuPA::EError."sv);
    }
    return false;
}

void TRunProcessor::DumpMenu() {
    if (!Ctx.Ctx.Logger().IsSuitable(TLOG_INFO)) {
        return;
    }
    TVector<TString> categories;
    for (const NJson::TJsonValue& category : Menu["categories"].GetArray()) {
        categories.push_back(category["name"].GetString());
    }
    LOG_INFO(Ctx.Ctx.Logger()) << "Menu categories: " << JoinSeq(", ", categories);
}

void TRunProcessor::ProcessRemoveItem(const TSemanticFrame& frame) {
    if (IsPreparing) {
        return;
    }
    NApi::TCart& cart = *ShortMemory().MutableCart();
    if (cart.GetItems().empty()) {
        WriteNormalResponse("nlg_remove_item_from_empty_cart"sv);
        return;
    }
    THashMap<TString, TString> slots = ToSlotMap(frame);
    Response.NlgCtx()["item_to_remove"] = slots.Value("item_text", "");
    const size_t indexToRemove = FindCartItemToRemove(cart, slots);
    if (indexToRemove == NPOS) {
        WriteNormalResponse("nlg_remove_item_not_found"sv);
        return;
    }
    Response.NlgCtx()["removed_item"] = cart.GetItems(indexToRemove).GetName();
    cart.MutableItems()->DeleteSubrange(indexToRemove, 1);
    AddCartToNlgCtx(cart);
    if (cart.GetItems().empty()) {
        WriteNormalResponse("nlg_remove_item_ok_last_item"sv);
        return;
    }
    WriteNormalResponse("nlg_remove_item_ok"sv);
}

size_t TRunProcessor::FindCartItemToRemove(const NApi::TCart& cart, const THashMap<TString, TString>& slots) {
    const TString itemText = slots.Value("item_text", "");
    const size_t indexToRemove = FindCartItemToRemove(cart, itemText);
    if (indexToRemove != NPOS) {
        return indexToRemove;
    }

    const TVector<TNluCartItem> nluCartItems = {{
        .SpokenName = itemText,
        .NameId = slots.Value("item_name", ""),
        .IsQuantityDefinedByUser = false,
        .Quantity = 1,
    }};
    TVector<TMatcherCartItem> knownItems;
    TVector<TString> unknownItems;
    MenuMatcher.Convert(nluCartItems, Menu, &knownItems, &unknownItems);
    if (knownItems.empty()) {
        return NPOS;
    }
    return FindCartItemToRemove(cart, knownItems[0].Name);
}

size_t TRunProcessor::FindCartItemToRemove(const NApi::TCart& cart, TStringBuf itemToRemove) {
    LOG_INFO(Ctx.Ctx.Logger()) << "FindCartItemToRemove. Try to find item \"" << itemToRemove << "\"";
    const NParsedUserPhrase::TParsedSequence etalon(itemToRemove);
    size_t bestIndex = NPOS;
    float bestScore = 0;
    for (const auto& [i, item] : Enumerate(cart.GetItems())) {
        const NParsedUserPhrase::TParsedSequence parsedItemName(item.GetName());
        const float score = NParsedUserPhrase::ComputeIntersectionScore(etalon, parsedItemName);
        LOG_INFO(Ctx.Ctx.Logger()) << "FindCartItemToRemove."
            << " Searched text: \"" << itemToRemove  << "\"."
            << " Hypothesis " << i
            << " text: \"" << item.GetName() << "\","
            << " score: " << score;
        if (score < 0.3) {
            continue;
        }
        if (score > bestScore) {
            bestScore = score;
            bestIndex = i;
        }
    }
    return bestIndex;
}

void TRunProcessor::ProcessDeclineKeepOldCart() {
    if (IsPreparing) {
        return;
    }
    ShortMemory().ClearCart();
    WriteNormalResponse("nlg_begin_new_cart"sv);
}

void TRunProcessor::ProcessClearCart() {
    if (IsPreparing) {
        return;
    }
    const bool wasCartEmpty = ShortMemory().GetCart().GetItems().empty();
    ShortMemory().ClearCart();
    if (wasCartEmpty) {
        WriteNormalResponse("nlg_cart_clear_empty"sv);
    } else {
        WriteNormalResponse("nlg_cart_clear_ok"sv);
    }
}

void TRunProcessor::ProcessShowCart() {
    if (IsPreparing) {
        return;
    }
    const NApi::TCart& cart = ShortMemory().GetCart();
    AddCartToNlgCtx(cart);

    if (cart.GetItems().empty()) {
        WriteNormalResponse("nlg_cart_show_empty"sv);
    } else {
        WriteCartUpdatedResponse("nlg_cart_show_ok"sv);
    }
}

void TRunProcessor::AddCartToNlgCtx(const NApi::TCart& cart) {
    Response.NlgCtx()["cart"] = JsonFromProto(cart);
    Response.NlgCtx()["subtotal"] = CalculateSubtotal(cart);
}

void TRunProcessor::ProcessGoToApp() {
    if (IsPreparing || !TryRedirectToAppToContinue()) {
        return;
    }
    WriteNormalResponse("nlg_cart_was_pushed_to_app"sv);
}

bool TRunProcessor::TryPushCartForApplication() {
    if (IsPreparing) {
        return false;
    }

    const NApi::TCart& cart = ShortMemory().GetCart();
    const TString taxiUid = ShortMemory().GetAuth().GetTaxiUid();
    if (taxiUid.empty()) {
        WriteInternalErrorResponse("Got empty TaxiUid."sv);
        return false;
    }
    LOG_INFO(Ctx.Ctx.Logger()) << "Cart: " << SerializeProtoText(cart);

    TApplyArguments args;
    *args.MutablePostOrderData()->MutableCart() = cart;
    args.MutablePostOrderData()->SetTaxiUid(taxiUid);
    Response.SetCommitArguments(args);

    return true;
}

void TRunProcessor::RedirectToApp(const TString& pushTitle, const TString& pushBody, const TLocation& deliveryLocation) {
    const TString path = TStringBuilder{}
        << EDA_APP_CART_URI
        << "?" << "lat=" << deliveryLocation.GetLat()
        << "&" << "lon=" << deliveryLocation.GetLon();
    if (RedirectByLink) {
        Response.AddOpenUriDirective(path);
    } else {
        TPushMessageDirective& push = *Response.ResponseBody().AddServerDirectives()->MutablePushMessageDirective();
        push.SetTitle(pushTitle);
        push.SetBody(pushBody);
        push.SetLink(path);
        push.SetPushId("alice.food");
        push.SetPushTag("alice.food");
        push.SetThrottlePolicy("eddl-unlimitted");
        push.AddAppTypes(AT_SEARCH_APP);
    }
}

void TRunProcessor::RedirectToAppOnError(const TLocation& deliveryLocation) {
    RedirectToApp("Ошибка в Еде", "Продолжите в приложении", deliveryLocation);
}

void TRunProcessor::RedirectToAppOnError() {
    RedirectToAppOnError(Request.Location());
}

bool TRunProcessor::TryRedirectToAppWithCart(const TString& pushTitle, const TString& pushBody, const TLocation& deliveryLocation) {
    if (!TryPushCartForApplication()) {
        return false;
    }
    LOG_INFO(Ctx.Ctx.Logger()) << "Clear cart";
    ShortMemory().ClearCart();

    RedirectToApp(pushTitle, pushBody, deliveryLocation);
    return true;
}

bool TRunProcessor::TryRedirectToAppToPay(const TLocation& deliveryLocation) {
    return TryRedirectToAppWithCart("Оплата заказа", "Оплатите заказ в приложении", deliveryLocation);
}

bool TRunProcessor::TryRedirectToAppToContinue(const TLocation& deliveryLocation) {
    return TryRedirectToAppWithCart("Оформление заказа", "Продолжите заказ в приложении", deliveryLocation);
}

bool TRunProcessor::TryRedirectToAppToPay() {
    return TryRedirectToAppToPay(Request.Location());
}

bool TRunProcessor::TryRedirectToAppToContinue() {
    return TryRedirectToAppToContinue(Request.Location());
}

void TRunProcessor::ProcessNothingElse() {
    if (IsPreparing) {
        return;
    }
    if (ShortMemory().GetCart().GetItems().empty()) {
        WriteNormalResponse("silent_exit_scenario"sv);
    } else {
        WriteCartUpdatedResponse("nlg_order_details"sv);
    }
}

void TRunProcessor::ProcessBeginNewCart() {
    if (IsPreparing) {
        return;
    }
    ShortMemory().ClearCart();
    WriteNormalResponse("nlg_what_you_wish"sv);
}

void TRunProcessor::ProcessFormOrderAgree() {
    if (IsPreparing) {
        return;
    }
    const NApi::TCart& cart = ShortMemory().GetCart();
    AddCartToNlgCtx(cart);

    if (cart.GetItems().empty()) {
        WriteNormalResponse("nlg_cart_show_empty"sv);
    } else {
        WriteCartUpdatedResponse("nlg_order_details"sv);
    }
}

void TRunProcessor::ProcessConfirmOrderAgree() {
    if (IsPreparing) {
        const TBlackBoxUserInfo* userInfo = GetUserInfoProto(Request);
        Y_ENSURE(userInfo);
        NApiGetAddress::AddRequest(
            Ctx,
            Request.Location(),
            {
                .Phone = userInfo->GetPhone(),
                .YandexUid = userInfo->GetUid(),
                .TaxiUid = ShortMemory().GetAuth().GetTaxiUid(),
            }
        );
        return;
    }
    auto deliveryLocation = Request.Location();

    NApiGetAddress::TResponseData addressResponse = NApiGetAddress::ReadResponse(Ctx, Request);
    if (addressResponse.Error == NApiGetAddress::EError::SUCCESS) {
        const auto userAddress = JsonToProto<NApi::TAddress>(addressResponse.Address);
        deliveryLocation.SetLat(userAddress.GetLocation().GetLatitude());
        deliveryLocation.SetLon(userAddress.GetLocation().GetLongitude());
    } else if (addressResponse.Error == NApiGetAddress::EError::AUTHORIZATION_FAILED) {
        RedirectToAppOnError();
        WriteNormalResponse("nlg_no_response_from_eda"sv);
        return;
    }
    if (TryRedirectToAppToPay(deliveryLocation)) {
        WriteNormalResponse("nlg_order_checkout_push"sv);
    }
}

void TRunProcessor::ProcessRepeatLastOrder() {
    NJson::TJsonValue lastOrder = TryGetLastOrder();
    if (!lastOrder.IsDefined()) {
        return;
    }

    const NJson::TJsonValue& lastDeliveryLocation = lastOrder["address"]["location"];
    LOG_INFO(Ctx.Ctx.Logger()) << "Last order delivery location: " << JsonToString(lastDeliveryLocation);
    LOG_INFO(Ctx.Ctx.Logger()) << "Request location: " << SerializeProtoText(Request.Location());
    if (!IsNear(lastDeliveryLocation)) {
        WriteNormalResponse("nlg_last_order_too_far"sv);
        return;
    }

    NJson::TJsonValue cartJson;
    cartJson["items"] = ProcessCartOptions(lastOrder["cart"]["items"]);
    *ShortMemory().MutableCart() = JsonToProto<NApi::TCart>(cartJson, /* validateUtf8= */ true, /* ignoreUnknownFields= */ true);

    NJson::TJsonValue& nlgCtx = Response.NlgCtx();
    nlgCtx = lastOrder;
    nlgCtx["cart"]["items"] = cartJson["items"];
    nlgCtx["subtotal"] = CalculateSubtotal(ShortMemory().GetCart());
    WriteNormalResponse("nlg_last_order_content"sv);
}

NJson::TJsonValue TRunProcessor::TryGetLastOrder() {
    if (IsPreparing) {
        const TBlackBoxUserInfo* userInfo = GetUserInfoProto(Request);
        Y_ENSURE(userInfo);
        NApiGetLastOrderPA::AddRequest(
            Ctx,
            {
                .Phone = userInfo->GetPhone(),
                .YandexUid = userInfo->GetUid(),
                .TaxiUid = ShortMemory().GetAuth().GetTaxiUid()
            }
        );
        return {};
    }

    NApiGetLastOrderPA::EError error;
    NJson::TJsonValue lastOrder;
    NApiGetLastOrderPA::TResponseData apiResponse = NApiGetLastOrderPA::ReadResponse(Ctx);
    UpdateTaxiUid(apiResponse.TaxiUid);
    error = apiResponse.Error;
    lastOrder = std::move(apiResponse.LastOrder);

    if (lastOrder.IsDefined()) {
        return std::move(lastOrder);
    }
    if (error == NApiGetLastOrderPA::EError::SUCCESS) {
        WriteInternalErrorResponse("Empty NApiGetLastOrderPA::TResponseData::LastOrder in successful response."sv);
    } else if (error == NApiGetLastOrderPA::EError::AUTHORIZATION_FAILED) {
        RedirectToAppOnError();
        WriteNormalResponse("nlg_no_response_from_eda"sv);
    } else if (error == NApiGetLastOrderPA::EError::NO_RESPONSE) {
        if (TryRedirectToAppToContinue()) {
            WriteNormalResponse("nlg_no_response_from_eda"sv);
        }
    } else if (error == NApiGetLastOrderPA::EError::NO_ORDER) {
        WriteNormalResponse("nlg_last_order_not_found"sv);
    } else {
        WriteInternalErrorResponse("Invalid NApiGetLastOrderPA::EError."sv);
    }
    return {};
}

bool TRunProcessor::IsNear(const NJson::TJsonValue& lastDeliveryLocationJson) const {
    return std::fabs(Request.Location().GetLon() - lastDeliveryLocationJson["longitude"].GetDouble()) <= 0.01 &&
           std::fabs(Request.Location().GetLat() - lastDeliveryLocationJson["latitude"].GetDouble()) <= 0.01;
}

// static
NJson::TJsonValue TRunProcessor::ProcessCartOptions(NJson::TJsonValue json) {
    for (NJson::TJsonValue& item : json.GetArraySafe()) {
        if (!item["item_options"].IsDefined()) {
            continue;
        }
        NJson::TJsonValue resultOptions{NJson::JSON_ARRAY};
        NJson::TJsonValue resultOptionNames{NJson::JSON_ARRAY};
        for (const auto& itemOption : item["item_options"].GetArray()) {
            NJson::TJsonValue resultOption;
            resultOption["group_id"] = itemOption["group_id"];
            for (const auto& groupOption : itemOption["group_options"].GetArray()) {
                resultOption["group_options"].AppendValue(groupOption["id"]);
                NJson::TJsonValue resultModifier;
                resultModifier["option_id"] = groupOption["id"];
                resultModifier["quantity"] = groupOption["quantity"];
                resultModifier["name"] = groupOption["name"];
                resultOption["modifiers"].AppendValue(resultModifier);
                resultOptionNames.AppendValue(groupOption["name"]);
            }
            resultOptions.AppendValue(resultOption);
        }
        item["item_options"] = resultOptions;
        item["item_option_names"] = resultOptionNames;
    }
    return json;
}

// ~~~~ Write response ~~~~

void TRunProcessor::WriteInternalErrorResponse(TStringBuf errorMsg) {
    LOG_ERROR(Ctx.Ctx.Logger()) << "Internal error. " << errorMsg;
    if (IsPreparing) {
        return;
    }
    WriteNormalResponse("nlg_internal_error"sv);
}

void TRunProcessor::WriteNormalResponse(TStringBuf responseName) {
    if (IsPreparing) {
        return;
    }
    const TResponseConfig& config = GetDialogConfig().GetResponse(responseName);

    if (!config.Flags.HasFlags(RCF_SILENT)) {
        Response.RenderNlg(responseName);
    }
    Response.ResponseBody().SetExpectsRequest(HasFallback(config));
    Response.SetShouldListen(config.Flags.HasFlags(RCF_LISTEN));
    if (config.UseLastExpectedFrameGroups) {
        TVector<TString> expectedFrameGroups(ShortMemory().GetResponseInfo().GetExpectedFrameGroups().begin(), ShortMemory().GetResponseInfo().GetExpectedFrameGroups().end());
        TVector<TString> suggests(ShortMemory().GetResponseInfo().GetSuggests().begin(), ShortMemory().GetResponseInfo().GetSuggests().end());
        WriteBasicResponse(responseName, expectedFrameGroups, suggests, 0);
    } else {
        WriteBasicResponse(responseName, config.ExpectedFrameGroups, config.Suggests, 0);
    }
}

void TRunProcessor::WriteCartUpdatedResponse(TStringBuf responseName) {
    TVector<TString> unavailableItems;
    NApi::TCart& cart = *ShortMemory().MutableCart();
    MenuMatcher.UpdateCart(cart, unavailableItems, Menu);

    AddCartToNlgCtx(cart);
    NJson::TJsonValue& nlgCtx = Response.NlgCtx();
    nlgCtx["unavailable_items"] = NGranet::NJsonUtils::ToJson(unavailableItems);
    if (cart.GetItems().empty()) {
        WriteNormalResponse("nlg_cart_show_outdated");
    } else {
        WriteNormalResponse(responseName);
    }
}

// static
bool TRunProcessor::HasFallback(const TResponseConfig& config) {
    for (const TString& groupName : config.ExpectedFrameGroups) {
        if (!GetDialogConfig().GetFrameGroup(groupName).FallbackNlg.empty()) {
            return true;
        }
    }
    return false;
}

void TRunProcessor::WriteBasicResponse(TStringBuf responseName,
    const TVector<TString>& expectedFrameGroups, const TVector<TString>& suggests, i32 fallbackCounter)
{
    if (IsPreparing) {
        return;
    }
    WriteStateResponseInfo(responseName, expectedFrameGroups, suggests, fallbackCounter);
    AddActionsToResponse(expectedFrameGroups);
    AddSuggestsToResponse(suggests);
    Response.SetState(State);
    std::move(Response).WriteResponse();
    LOG_INFO(Ctx.Ctx.Logger()) << "State in response: " << JsonFromProto(State);
}

void TRunProcessor::WriteStateResponseInfo(TStringBuf responseName,
    const TVector<TString>& expectedFrameGroups, const TVector<TString>& suggests, i32 fallbackCounter)
{
    TState::TShortMemory::TResponseInfo& info = *ShortMemory().MutableResponseInfo();
    info.SetResponseName(TString(responseName));
    info.SetFallbackCounter(fallbackCounter);
    info.ClearSuggests();
    for (const TString& suggest : suggests) {
        *info.AddSuggests() = suggest;
    }
    CopyToRepeatedField(expectedFrameGroups, info.MutableExpectedFrameGroups());
    info.SetServerTimeMs(RequestProto.GetBaseRequest().GetServerTimeMs());
    LOG_INFO(Ctx.Ctx.Logger()) << "Response info: " << JsonFromProto(info);
}

void TRunProcessor::AddActionsToResponse(const TVector<TString>& expectedFrameGroups) {
    for (const TString& groupName : expectedFrameGroups) {
        for (const TString& frameName : GetDialogConfig().GetFrameGroup(groupName).Frames) {
            if (!GetDialogConfig().IsFrameAction(frameName)) {
                continue;
            }
            NScenarios::TFrameAction action;
            action.MutableNluHint()->SetFrameName(frameName);
            Response.AddAction(frameName, action);
        }
    }
}

TVector<TString> TRunProcessor::GetShuffledPopularDishes() {
    for (const NJson::TJsonValue& category : Menu["categories"].GetArray()) {
        if (category["dynamicId"] == "popular") {
            TVector<TString> popularDishes;
            for (const NJson::TJsonValue& item : category["items"].GetArray()) {
                popularDishes.push_back(item["name"].GetString());
            }
            ShuffleRange(popularDishes, Ctx.Rng);
            return popularDishes;
        }
    }
    
    const auto& nlgHardcodedItems = GetNlgHardcodedItems();
    TVector<TString> popularDishes;
    popularDishes.reserve(nlgHardcodedItems.size());
    for (const auto& nlgItem : nlgHardcodedItems) {
        popularDishes.push_back(Response.NlgWrapper().RenderPhrase("suggests"sv, nlgItem, Response.NlgData()).Text);
    }

    return popularDishes;
}

void TRunProcessor::AddSuggestsToResponse(const TVector<TString>& suggests) {
    const auto popularDishes = GetShuffledPopularDishes();
    size_t dishIdx = 0;

    for (const TString& suggest : suggests) {
        if (IsDishSuggest(suggest)) {
            if (dishIdx >= popularDishes.size()) {
                continue;
            }
            Response.NlgCtx()["dish_name"] = popularDishes[dishIdx++];
        }
        const TString suggestText = Response.NlgWrapper().RenderPhrase("suggests"sv, suggest, Response.NlgData()).Text;
        Response.AddTypeTextSuggest(suggestText);
    }
}

const TState::TShortMemory& TRunProcessor::ShortMemory() const {
    return State.GetShortMemory();
}

TState::TShortMemory& TRunProcessor::ShortMemory() {
    return *State.MutableShortMemory();
}

void TRunProcessor::UpdateTaxiUid(const TString& taxiUid) {
    LOG_INFO(Ctx.Ctx.Logger()) << "TAXIUID: " << taxiUid;
    ShortMemory().MutableAuth()->SetTaxiUid(taxiUid);
    if (!taxiUid.empty()) {
        ShortMemory().MutableAuth()->SetAuthOk(true);
    } else {
        ShortMemory().MutableAuth()->SetAuthOk(false);
    }
}

void TRunProcessor::UpdatePlaceSlug(const TString& placeSlug) {
    LOG_INFO(Ctx.Ctx.Logger()) << "Place slug: " << placeSlug;
    ShortMemory().SetPlaceSlug(placeSlug);
}

bool TRunProcessor::InsertIfRepeated(const NApi::TCart::TItem& item, NApi::TCart& cart) {
    for (auto& cartItem : *cart.MutableItems()) {
        if (item == cartItem) {
            cartItem.SetPrice(cartItem.GetPrice() + item.GetPrice());
            cartItem.SetQuantity(cartItem.GetQuantity() + item.GetQuantity());
            return true;
        }
    }
    return false;
}

// ~~~~ TRunProcessorHandle ~~~~

TRunProcessorHandle::TRunProcessorHandle(TStringBuf name, bool isPreparing)
    : Name_(name)
    , IsPreparing(isPreparing)
{
    NNlu::TRequestNormalizer::WarmUpSingleton();
    MenuMatcher.AddMenuToNormalizationCache(ReadHardcodedMenuSample());
}

TString TRunProcessorHandle::Name() const {
    return Name_;
}

void TRunProcessorHandle::Do(TScenarioHandleContext& ctx) const {
    TRunProcessor(IsPreparing, MenuMatcher, &ctx).Process();
}

} // namespace NAlice::NHollywood::NFood
