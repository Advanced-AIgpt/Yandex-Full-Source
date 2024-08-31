#include "remember_address.h"

#include <alice/bass/forms/common/personal_data.h>
#include <alice/bass/forms/geo_resolver.h>
#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

namespace NBASS {

static constexpr TStringBuf SAVE_ADDRESS = "personal_assistant.scenarios.remember_named_location";
static constexpr TStringBuf SAVE_ADDRESS_ELLIPSIS = "personal_assistant.scenarios.remember_named_location__ellipsis";
static constexpr TStringBuf CONNECT_NAMED_LOCATION_TO_DEVICE = "personal_assistant.scenarios.connect_named_location_to_device";
static constexpr TStringBuf CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_YES = "personal_assistant.scenarios.connect_named_location_to_device__confirmation_yes";
static constexpr TStringBuf CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_NO = "personal_assistant.scenarios.connect_named_location_to_device__confirmation_no";
static constexpr size_t MAX_ATTEMPTS = 2;

TResultValue TSaveAddressHandler::Do(TRequestHandler& r) {
    TContext& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::COMMANDS_OTHER);

    TContext::TSlot* confirmation = ctx.GetOrCreateSlot(TStringBuf("confirmation"), TStringBuf("confirmation"));
    bool hasConfirmation = !IsSlotEmpty(confirmation) && confirmation->Value.GetString() == "yes";

    if (ctx.FormName() == SAVE_ADDRESS || ctx.FormName() == SAVE_ADDRESS_ELLIPSIS) {
        return ProcessAddressSaving(ctx, hasConfirmation);
    } else if (ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_YES) {
        if (auto err = SaveDeviceConnectionToSpecialLocation(ctx, true /*hasConfirmation*/)) {
            return TResultValue();
        }
        confirmation->Value = "yes";
        return SwitchToCallbackForm(ctx);
    } else if (ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_NO) {
        if (auto err = SaveDeviceConnectionToSpecialLocation(ctx, false /*hasConfirmation*/)) {
            return TResultValue();
        }
        confirmation->Value = "no";
        return SwitchToCallbackForm(ctx);
    } else if (ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE) {
        ctx.CreateSlot(TStringBuf("confirmation"), TStringBuf("confirmation"), false /* optional */);
        ctx.AddSuggest(TStringBuf("remember_named_location__confirm"));
        ctx.AddSuggest(TStringBuf("remember_named_location__decline"));
    }
    return TResultValue();
}

// static
TResultValue TSaveAddressHandler::SetAsResponse(TContext& ctx, TSpecialLocation name, EModes mode) {
    TIntrusivePtr<TContext> newContext;
    if (mode == EModes::SaveAddress) {
        newContext = ctx.SetResponseForm(SAVE_ADDRESS, true);
        Y_ENSURE(newContext);
        newContext->CreateSlot(TStringBuf("location_name"), TStringBuf("named_location"), false, TStringBuf(name));
    } else if (mode == EModes::ConnectDeviceToAddress) {
        newContext = ctx.SetResponseForm(CONNECT_NAMED_LOCATION_TO_DEVICE, true /* set current form as callback */);
        Y_ENSURE(newContext);
        newContext->GetOrCreateSlot(TStringBuf("location_name"), TStringBuf("named_location"))->Value.SetString(name.AsString());
    }

    auto& block = *newContext->Block();
    block["type"] = "features_data";
    block["data"]["answers_expected_request"].SetBool(true);

    return TResultValue();
}

TResultValue TSaveAddressHandler::SwitchToCallbackForm(TContext& ctx) {
    // this form always has "callback_form" slot (may be empty)
    if (TContext::TPtr restoredContext = ctx.SetCallbackAsResponseForm()) {
        if (ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE ||
            ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_YES ||
            ctx.FormName() == CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_NO)
        {
            auto newConfirmationSlot = restoredContext->GetOrCreateSlot(TStringBuf("connect_named_location_to_device__confirmation"), TStringBuf("confirmation"));
            auto currentConfirmationSlot = ctx.GetOrCreateSlot(TStringBuf("confirmation"), TStringBuf("confirmation"));
            newConfirmationSlot->Value.CopyFrom(currentConfirmationSlot->Value);
        }
        return ctx.RunResponseFormHandler();
    }
    return TResultValue();
}

void TSaveAddressHandler::Register(THandlersMap* handlers) {
    auto cbSaveAddressForm = []() {
        return MakeHolder<TSaveAddressHandler>();
    };
    handlers->emplace(SAVE_ADDRESS, cbSaveAddressForm);
    handlers->emplace(SAVE_ADDRESS_ELLIPSIS, cbSaveAddressForm);
    handlers->emplace(CONNECT_NAMED_LOCATION_TO_DEVICE, cbSaveAddressForm);
    handlers->emplace(CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_YES, cbSaveAddressForm);
    handlers->emplace(CONNECT_NAMED_LOCATION_TO_DEVICE_CONFIRMATION_NO, cbSaveAddressForm);
}

TResultValue TSaveAddressHandler::ResolveAddress(TContext& context, size_t resultIndex) const {
    TGeoPosition userLocation;
    userLocation.Lat = context.Meta().Location().Lat();
    userLocation.Lon = context.Meta().Location().Lon();

    TContext::TSlot* addressSlot = context.GetOrCreateSlot(TStringBuf("location_address"), TStringBuf("string"));

    THolder<TGeoObjectResolver> resolver = THolder<TGeoObjectResolver>(new TGeoObjectResolver(context, addressSlot->Value.GetString(), userLocation,
                                                                  "geo,biz", "", {}, resultIndex));

    NSc::TValue resolvedAddress;
    if (TResultValue geoError = resolver->WaitAndParseResponse(&resolvedAddress)) {
        return geoError;
    }

    if (resolvedAddress.IsNull()) {
        if (resultIndex == 1) {
            // nothing was found at all
            context.AddErrorBlock(TError::EType::NOGEOFOUND, TStringBuf("no geo found for location address"));
        } else {
            // no more geo was found
            context.CreateSlot(TStringBuf("location_address_resolved"), TStringBuf("geo"), true, NSc::Null());
        }
    } else {
        context.CreateSlot(TStringBuf("location_address_resolved"), TStringBuf("geo"), true, resolvedAddress);
        context.CreateSlot(TStringBuf("confirmation"), TStringBuf("confirmation"), false);
        context.AddSuggest(TStringBuf("remember_named_location__confirm"));
        context.AddSuggest(TStringBuf("remember_named_location__decline"));
    }

    return TResultValue();
}

TResultValue TSaveAddressHandler::SaveAddress(TContext& context, const NSc::TValue& address) const {
    TContext::TSlot* locationNameSlot = context.GetOrCreateSlot(TStringBuf("location_name"), TStringBuf("named_location"));
    return context.SaveAddress(TSpecialLocation::GetNamedLocation(locationNameSlot), address);
}

TResultValue TSaveAddressHandler::SaveDeviceConnectionToSpecialLocation(TContext& ctx, bool confirmation) const {
    TContext::TSlot* locationNameSlot = ctx.GetOrCreateSlot(TStringBuf("location_name"), TStringBuf("named_location"));
    TSpecialLocation locationName = TSpecialLocation::GetNamedLocation(locationNameSlot);
    if (!locationName.IsUserAddress()) {
        auto err = TError(TError::EType::INVALIDPARAM, "Error while saving address: invalid address name");
        NSc::TValue errorData;
        errorData["code"].SetString(TStringBuf("invalid_address_name"));
        ctx.AddErrorBlock(err, errorData);
        return err;
    }
    TPersonalDataHelper personalData(ctx);
    TPersonalDataHelper::TUserInfo blackBoxInfo;
    if (!personalData.GetUserInfo(blackBoxInfo)) {
        auto err = TError(TError::EType::INVALIDPARAM, "Error while saving address: user unauthorized");
        NSc::TValue errorData;
        errorData["code"].SetString(TStringBuf("unauthorized"));
        ctx.AddErrorBlock(err, errorData);
        return err;
    }
    TStringBuf locationId = confirmation ? locationNameSlot->Value.GetString() : "";
    return personalData.UpdateDataSyncDeviceGeoPoint(blackBoxInfo.GetUid(), locationId);
}

TResultValue TSaveAddressHandler::ProcessAddressSaving(TContext& ctx, bool hasConfirmation) {
    TContext::TSlot* resolvedAddress = ctx.GetOrCreateSlot(TStringBuf("location_address_resolved"), TStringBuf("geo"));
    bool hasResolvedLocation = !IsSlotEmpty(resolvedAddress);
    if (hasResolvedLocation && hasConfirmation) {
        if (const TResultValue err = SaveAddress(ctx, resolvedAddress->Value)) {
            return err;
        }
        return SwitchToCallbackForm(ctx);
    } else {
        TContext::TSlot* slotResultIndex = ctx.GetOrCreateSlot(TStringBuf("result_index"), TStringBuf("num"));
        size_t resultIndex = (IsSlotEmpty(slotResultIndex) || slotResultIndex->Value.GetIntNumber() < 1) ?
                            1 : slotResultIndex->Value.GetIntNumber();

        if (hasResolvedLocation && !hasConfirmation) {
            ++resultIndex;
        }
        if (resultIndex <= MAX_ATTEMPTS) {
            slotResultIndex->Value.SetIntNumber(resultIndex);
            return ResolveAddress(ctx, resultIndex);
        } else {
            TContext::TSlot* address = ctx.GetOrCreateSlot(TStringBuf("location_address"), TStringBuf("string"));
            if (!IsSlotEmpty(address)) {
                address->Value.SetNull();
            }
            if (!IsSlotEmpty(resolvedAddress)) {
                resolvedAddress->Value.SetNull();
            }
        }
    }
    return TResultValue();
}

}
