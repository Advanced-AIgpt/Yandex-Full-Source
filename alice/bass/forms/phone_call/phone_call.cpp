#include "phone_call.h"

#include <alice/bass/forms/common/contacts/contacts_finder.h>
#include <alice/bass/forms/directives.h>
#include <alice/bass/forms/geodb.h>
#include <alice/bass/forms/urls_builder.h>

#include <alice/bass/libs/logging_v2/logger.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/resource/resource.h>

namespace {

// call types
constexpr TStringBuf CALL_TYPE_EMERGENCY = "emergency";

constexpr TStringBuf PHONE_CALL = "personal_assistant.scenarios.call";
constexpr TStringBuf PHONE_CALL_ELLIPSIS = "personal_assistant.scenarios.call__ellipsis";

constexpr TStringBuf PHONEBOOK_FILENAME = "emergency_phones.json";

constexpr TStringBuf SOS_SERVICE_ID = "sos";

constexpr TStringBuf REASON_FLAG_ONE_CONTACT_ONE_NUMBER = "one_contact_one_number";

} // anonymous namespace

namespace NBASS {

const THashSet<TStringBuf> TPhoneCallHandler::EmergencyServices = {
    TStringBuf("ambulance"),
    TStringBuf("fire_department"),
    TStringBuf("police"),
    TStringBuf("sos")
};

TPhoneCallHandler::TPhoneBook::TPhoneBook() {
    TString content;
    if (!NResource::FindExact(PHONEBOOK_FILENAME, &content)) {
        ythrow yexception() << "Unable to load built-in resource " << PHONEBOOK_FILENAME;
    }

    TStringBuilder validationErrorMessage;
    validationErrorMessage << "Incorrect data in " << PHONEBOOK_FILENAME << ": ";
    NSc::TValue json = NSc::TValue::FromJson(content);
    const NSc::TDict& dict = json.GetDict();
    for (const auto& record : dict) {
        NGeobase::TId country = FromString<NGeobase::TId>(record.first.data(), record.first.length(), NGeobase::UNKNOWN_REGION);
        if (!NAlice::IsValidId(country)) {
            ythrow yexception() << validationErrorMessage
                                << record.first << " is not valid country geo id";
        }
        // every country record should contain SOS_SERVICE_ID phone
        if (!record.second.Has(SOS_SERVICE_ID)) {
            ythrow yexception() << validationErrorMessage
                                << "<" << SOS_SERVICE_ID << "> phone is not defined for the country " << country;
        }
        EmergencyPhoneBook[country] = record.second;
    }

    // phonebook should contain EARTH_ID record (we use that record as default value)
    if (!EmergencyPhoneBook.contains(NGeobase::EARTH_REGION_ID)) {
        ythrow yexception() << validationErrorMessage
                            << "default record with EARTH_ID was not found" << Endl;
    }
}

const THashMap<NGeobase::TId, NSc::TValue>& TPhoneCallHandler::GetEmergencyPhoneBook() {
    static const TPhoneBook phoneBook;
    return phoneBook.EmergencyPhoneBook;
};

NSc::TValue TPhoneCallHandler::GetRegionalEmergencyServices(NGeobase::TId userCountry) {
    const THashMap<NGeobase::TId, NSc::TValue>& phoneBook = GetEmergencyPhoneBook();
    const auto regInfo = phoneBook.find(userCountry);
    if (regInfo != phoneBook.cend()) {
        return regInfo->second;
    } else {
        return phoneBook.at(NGeobase::EARTH_REGION_ID);
    }
}

NSc::TValue TPhoneCallHandler::GetEmergencyServiceInfo(NGeobase::TId userCountry, TStringBuf serviceId) {
    const NSc::TValue& regServices = GetRegionalEmergencyServices(userCountry);

    if (!regServices.Has(serviceId)) {
        serviceId = SOS_SERVICE_ID;
    }

    auto result = regServices[serviceId];
    result["type"] = CALL_TYPE_EMERGENCY;
    return result;
}

void TPhoneCallHandler::AddEmergencyServiceSuggests(NGeobase::TId userCountry, TStringBuf serviceId, TContext& ctx) {
    const NSc::TValue& regServices = GetRegionalEmergencyServices(userCountry);

    for (const auto& name : EmergencyServices) {
        if (name != serviceId && regServices.Has(name)) {
            ctx.AddSuggest("call__emergency_service", regServices[name]);
        }
    }
}

TResultValue TPhoneCallHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::CALL);
    const auto result = DoImpl(r);
    if (!ctx.MetaClientInfo().IsYaAuto()) {
        ctx.AddSearchSuggest();
        ctx.AddOnboardingSuggest();
    }

    return result;
}

void TPhoneCallHandler::Register(THandlersMap* handlers) {
    auto cbPhoneCallForm = []() {
        return MakeHolder<TPhoneCallHandler>();
    };

    handlers->emplace(PHONE_CALL, cbPhoneCallForm);
    handlers->emplace(PHONE_CALL_ELLIPSIS, cbPhoneCallForm);
}

TResultValue TPhoneCallHandler::HandleEmergencyCall(TRequestHandler& r,
                                                    NSc::TValue* recipientInfo) {
    // the recipient is matched with the list of emergency services
    const NGeobase::TLookup& geobase = r.Ctx().GlobalCtx().GeobaseLookup();
    NGeobase::TId userCountry = NGeobase::UNKNOWN_REGION;
    if (NAlice::IsValidId(r.Ctx().UserRegion())) {
        userCountry = geobase.GetCountryId(r.Ctx().UserRegion());
    }
    TStringBuf recipientName = r.Ctx().GetSlot(TStringBuf("recipient"))->Value.GetString();
    if (!EmergencyServices.contains(recipientName)) {
        return TError(TError::EType::INVALIDPARAM, "Unknown recipient name");
    }
    *recipientInfo = GetEmergencyServiceInfo(userCountry, recipientName);
    return TResultValue();
}

TResultValue TPhoneCallHandler::DoImpl(TRequestHandler& r) {
    static const TStringBuf knownPhoneType = "known_phones";
    auto& ctx = r.Ctx();

    if (!ctx.ClientFeatures().SupportsPhoneCalls())
        ctx.AddAttention("calls_not_supported_on_device");

    if (!NContactsFinder::HandlePermission("permission", ctx))
        return TResultValue();

    TContext::TSlot* recipientSlot = ctx.GetSlot("recipient");
    if (IsSlotEmpty(recipientSlot)) {
        recipientSlot->Optional = false;
        return TResultValue();
    }
    ctx.MarkSensitive(*recipientSlot);

    NSc::TValue recipientInfo;
    if (recipientSlot->Type == knownPhoneType)
        if (TResultValue err = HandleEmergencyCall(r, &recipientInfo))
            return err;

    if (ctx.ClientFeatures().SupportsPhoneCalls()) {
        const auto& geobase = r.Ctx().GlobalCtx().GeobaseLookup();
        NGeobase::TId userCountry = NGeobase::UNKNOWN_REGION;
        if (NAlice::IsValidId(r.Ctx().UserRegion())) {
            userCountry = geobase.GetCountryId(r.Ctx().UserRegion());
        }
        AddEmergencyServiceSuggests(userCountry, recipientSlot->Value.GetString(), ctx);

        if (recipientInfo.Has("phone")) {
            recipientInfo["phone_uri"] = GeneratePhoneUri(ctx.MetaClientInfo(), recipientInfo["phone"],
                false /* don't normalize */, false /* prefix free */);
            FillCallData(recipientInfo, ctx, recipientInfo["reason"] == REASON_FLAG_ONE_CONTACT_ONE_NUMBER);
        }
    }

    TContext::TSlot* recipientInfoSlot = ctx.GetOrCreateSlot("recipient_info", "recipient_info");
    recipientInfoSlot->Value = recipientInfo;
    return TResultValue();
}

void TPhoneCallHandler::FillCallData(const NSc::TValue& recipientInfo, TContext& ctx, bool useDivCard) const {
    NSc::TValue data;
    data["uri"] = recipientInfo["phone_uri"];
    ctx.AddCommand<TPhoneCallDirective>(TStringBuf("open_uri"), data);

    if (useDivCard && ctx.ClientFeatures().SupportsDivCards() && ctx.FormName() == PHONE_CALL)
        ctx.AddDivCardBlock("call__known_phone", recipientInfo);
}

}
