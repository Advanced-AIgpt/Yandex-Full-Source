#include "messaging.h"
#include "directives.h"

#include <alice/bass/forms/common/contacts/contacts.h>
#include <alice/bass/forms/common/contacts/contacts_finder.h>

#include <alice/bass/forms/urls_builder.h>

#include <alice/library/analytics/common/product_scenarios.h>

#include <library/cpp/scheme/scheme_cast.h>

#include <library/cpp/string_utils/quote/quote.h>

namespace {

constexpr TStringBuf MESSAGING = "personal_assistant.scenarios.messaging";
constexpr TStringBuf MESSAGING_RECIPIENT = "personal_assistant.scenarios.messaging__recipient";
constexpr TStringBuf MESSAGING_RECIPIENT_ELLIPSIS = "personal_assistant.scenarios.messaging__recipient_ellipsis";
constexpr TStringBuf MESSAGING_CLIENT = "personal_assistant.scenarios.messaging__client";
constexpr TStringBuf MESSAGING_TEXT = "personal_assistant.scenarios.messaging__text";

constexpr TStringBuf CLIENT_APP_NOT_INSTALLED = "unavailable";

constexpr TStringBuf RECIPIENT_SLOT = "recipient";
constexpr TStringBuf CLIENT_SLOT = "client";
constexpr TStringBuf CONTACT_SEARCH_RESULTS_SLOT = "contact_search_results";

}

namespace NBASS {

bool HasOnlyOneNumber(const NSc::TValue& contact) {
    return contact["phones"].GetArray().size() == 1;
}

NSc::TValue MakeRecipientInfoData(TStringBuf name, TStringBuf phone, TStringBuf avatarUrl) {
    NSc::TValue result;
    result["title"] = name;
    result["phone"] = phone;
    result["avatar_url"] = avatarUrl;
    return result;
}

bool FillRecipientInfoData(const NSc::TValue& contact, NSc::TValue& recipientInfo) {
    TString phone;
    if (HasOnlyOneNumber(contact)) {
        phone = contact["phones"].GetArray()[0]["phone"].GetString();
        recipientInfo = MakeRecipientInfoData(contact["name"].GetString(), phone, contact["avatar_url"].GetString());
        return true;
    }

    return false;
}


void AnalizeContactSearchResults(TContext& ctx, TSlot& recipient, TSlot& searchResults, NSc::TValue* recipientInfo) {
    NSc::TValue result;

    auto contactsManager = TContacts(searchResults.Value, ctx);
    auto searchResultsValue = contactsManager.Preprocess(recipient);
    const auto& contacts = searchResultsValue.GetArray();

    switch (contacts.size()) {
        case 0: {
            ctx.AddAttention("contact_not_found");
            break;
        }
        case 1: {
            if (!FillRecipientInfoData(contacts[0], *recipientInfo)) {
                ctx.AddDivCardBlock("messaging__search_contacts", searchResultsValue);
                ctx.AddTextCardBlock("messaging__specify_which_phone");
            }

            break;
        }
        default: {
            if (contacts[0]["request_full_match"].GetBool()) {
                if (!FillRecipientInfoData(contacts[0], *recipientInfo)) {
                    searchResultsValue.GetArrayMutable().resize(1);
                    ctx.AddDivCardBlock("messaging__search_contacts", searchResultsValue);
                    ctx.AddTextCardBlock("messaging__specify_which_phone");
                }
            } else {
                ctx.AddDivCardBlock("messaging__search_contacts", searchResultsValue);
                ctx.AddTextCardBlock("messaging__specify_which_recipient");
            }
        }
    }

    searchResults.Value = searchResultsValue;
}

void MarkSensitiveIfNeed(const TVector<TStringBuf>& slotNames, TContext& ctx) {
    for (const auto& name : slotNames) {
        TContext::TSlot* slot = ctx.GetSlot(name);
        if (!IsSlotEmpty(slot))
            ctx.MarkSensitive(*slot);
    }
}

bool ActivateSlotAskEvent(TStringBuf name, TContext& ctx) {
    TContext::TSlot* slot = ctx.GetOrCreateSlot(name, "string");
    if (slot->Value.IsNull()) {
        slot->Optional = false; // activate ask event for slot
        return true;
    }
    return false;
}

NSc::TValue GenerateUri(TStringBuf phone, TStringBuf client, TStringBuf text) {
    NSc::TValue data;
    data["uri"] = TString("msg://") + "phone=" + phone + "&client=" + client + "&text=" + text;
    return data;
}

TResultValue TMessagingFormHandler::HandleRequest(TContext& ctx) {
    if (!ctx.ClientFeatures().SupportsMessaging()) {
        ctx.AddAttention("messaging_not_supported");
        return TResultValue();
    }

    if (!NContactsFinder::HandlePermission("permission", ctx))
        return TResultValue();

    TContext::TSlot* clientStatusSlot = ctx.GetSlot("client_status");
    if (!IsSlotEmpty(clientStatusSlot) && clientStatusSlot->Value.GetString() == CLIENT_APP_NOT_INSTALLED) {
        if (!IsSlotEmpty(ctx.GetSlot(CLIENT_SLOT)))
            ctx.AddTextCardBlock("messaging__app_not_installed");
        return TResultValue();
    }

    MarkSensitiveIfNeed({RECIPIENT_SLOT, CONTACT_SEARCH_RESULTS_SLOT}, ctx);
    if (ActivateSlotAskEvent(RECIPIENT_SLOT, ctx))
        return TResultValue();

    NSc::TValue recipientInfo;
    TContext::TSlot* searchResultsSlot = ctx.GetSlot(CONTACT_SEARCH_RESULTS_SLOT);
    if (IsSlotEmpty(searchResultsSlot)) {
        auto payload = GetFindContactsPayload(ctx);
        ctx.AddCommand<TPhoneFindContactsDirective>(TStringBuf("find_contacts"), payload);
        if (ctx.HasExpFlag("find_contacts_view_data"))
            ctx.AddDivCardBlock("call__find_contacts_request_data", payload["request"]);

        ctx.AddUniProxyAction("add_contact_book_asr", NSc::Null());
    } else  {
        AnalizeContactSearchResults(ctx, *ctx.GetSlot("recipient"), *searchResultsSlot, &recipientInfo);

        if (recipientInfo.Has("phone")) {
            if (ActivateSlotAskEvent(CLIENT_SLOT, ctx))
                return TResultValue();

            TContext::TSlot* recipientInfoSlot = ctx.GetOrCreateSlot("recipient_info", "recipient_info");
            recipientInfoSlot->Value = recipientInfo;

            if (ActivateSlotAskEvent("text", ctx))
                return TResultValue();

            NSc::TValue data;

            TContext callback(ctx, "personal_assistant.scenarios.messaging");
            callback.CopySlotsFrom(ctx, {"client"});
            callback.CreateSlot("client_status", "string", true, NSc::TValue(CLIENT_APP_NOT_INSTALLED));

            TString fallback = NJsonConverters::ToJson(callback.ToJson(TContext::EJsonOut::FormUpdate | TContext::EJsonOut::Resubmit));
            Quote(fallback, "");

            data["uri"] = GenerateMessengerUri(
                    recipientInfo["phone"].GetString(),
                    ctx.GetSlot("client")->Value.GetString(),
                    ctx.GetSlot("text")->Value.GetString(),
                    fallback);

            ctx.AddCommand<TPhoneSendMessageDirective>(TStringBuf("open_uri"), data);

            // выше по стеку мы уже можем добавить карточку выбора абонента, еще одну не надо бы добавлять без доп. условий (в звонке такое доп условие есть)
            if (ctx.ClientFeatures().SupportsDivCards())
                ctx.AddDivCardBlock("messaging__send_message", recipientInfo);
        }
    }

    return TResultValue();
}


TResultValue TMessagingFormHandler::Do(TRequestHandler& r) {
    auto& ctx = r.Ctx();
    const auto result = HandleRequest(ctx);

    ctx.GetAnalyticsInfoBuilder().SetProductScenarioName(NAlice::NProductScenarios::PLACEHOLDERS);
    ctx.AddSearchSuggest();
    ctx.AddOnboardingSuggest();

    return result;
}

void TMessagingFormHandler::Register(THandlersMap* handlers) {
    auto cbPhoneCallForm = []() {
        return MakeHolder<TMessagingFormHandler>();
    };

    handlers->emplace(MESSAGING, cbPhoneCallForm);
    handlers->emplace(MESSAGING_RECIPIENT, cbPhoneCallForm);
    handlers->emplace(MESSAGING_RECIPIENT_ELLIPSIS, cbPhoneCallForm);
    handlers->emplace(MESSAGING_CLIENT, cbPhoneCallForm);
    handlers->emplace(MESSAGING_TEXT, cbPhoneCallForm);
}

}
