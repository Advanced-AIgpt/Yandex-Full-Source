#include "authorizing_handler.h"

#include <alice/bass/forms/crmbot/context.h>
#include <alice/bass/forms/crmbot/order_id.h>
#include <alice/bass/forms/crmbot/personal_data_helper.h>

#include <util/string/join.h>

#include <regex>

namespace NBASS::NCrmbot {

TResultValue TAuthorizingOrderFormHandler::Do(TRequestHandler& r)
{
    TCrmbotContext ctx(r.Ctx());
    ChangeFormToScenarioEntry(ctx);
    TryFillSlots(ctx);
    auto personalData = NCrmbot::TPersonalDataHelper(ctx.Ctx());
    TStringBuf puid = personalData.GetPuid();
    TStringBuf muid = personalData.GetMuid();
    if (puid.Empty() && muid.Empty()) {
        HandleAnonymous(ctx);
    } else {
        HandleAuthorized(ctx, puid, muid);
    }

    return TResultValue();
}

TString TAuthorizingOrderFormHandler::FormatEmail(TStringBuf email) const
{
    TString result = TString(email);
    result.to_lower();
    return result;
}

TString TAuthorizingOrderFormHandler::FormatPhoneNumber(TStringBuf number) const
{
    const std::regex nonDigits("[^\\d]");
    bool hasPlus = (number.front() == '+');
    TString strippedPhone(std::regex_replace(number.data(), nonDigits, ""));
    TStringBuilder result{};
    if (hasPlus) {  // we had a "+" symbol in the begining so no prefix modification is needed
        result << "+" << strippedPhone;
    } else if (strippedPhone.size() == 10) {  // 123 123 12 12
        result << "+7" << strippedPhone;
    } else {
        if (strippedPhone.front() == '8') {  // 8 123 123 12 12
            result << "+7" << strippedPhone.substr(1);
        } else {  // 7 123 123 12 12
            result << "+" << strippedPhone;
        }
    }
    return result;
}

bool TAuthorizingOrderFormHandler::CheckEmail(TStringBuf email) const
{
    // взято отсюда: https://a.yandex-team.ru/arc/trunk/arcadia/crypta/graph/soup/identifiers/lib/email.h?rev=3904333
    const std::regex emailRegex("^[a-zа-я0-9!#$%&'*+/=?^_`{|}~-]+(?:\\.[a-zа-я0-9!#$%&'*+/=?^_`{|}~-]+)*"
                                "@(?:[a-zа-я0-9](?:[a-zа-я0-9-]*[a-zа-я0-9])?\\.)"
                                "+[a-zа-я0-9](?:[a-zа-я0-9-]*[a-zа-я0-9])?$",
                                std::regex::icase);
    return std::regex_match(email.begin(), email.end(), emailRegex);
}

bool TAuthorizingOrderFormHandler::CheckPhoneNumber(TStringBuf number) const
{
    size_t digitCount = std::count_if(number.begin(), number.end(), [](char c) {return std::isdigit(c);});
    return digitCount == 10 || digitCount == 11; // 123 456 78 90 or +1 234 567 89 01
}

void TAuthorizingOrderFormHandler::CheckOrderPhoneAndEmail(
    TMaybe<TCheckouterOrder>& order, TStringBuf email, TStringBuf phone) const
{
    if (order.Defined()) {
        TString o_email = FormatEmail(order->Scheme()->Buyer().Email());
        TString o_phone = FormatPhoneNumber(order->Scheme()->Buyer().Phone());
        if (o_email != email && o_phone != phone) order = Nothing();
    }
}

bool TAuthorizingOrderFormHandler::TryFillOrderId(TStringBuf val, TCrmbotContext& ctx) const
{
    auto orderId = NCrmbot::TOrderId(val);
    if (orderId.HasBeruOrderID()) {
        ctx.SetStringSlot("order_id", ToString(orderId));
        return true;
    }
    return false;
}

bool TAuthorizingOrderFormHandler::TryFillPhoneNumber(TStringBuf val, TCrmbotContext& ctx) const
{
    if (CheckPhoneNumber(val)) {
        ctx.SetStringSlot("phone_number", FormatPhoneNumber(val));
        return true;
    }
    return false;
}

bool TAuthorizingOrderFormHandler::TryFillEmail(TStringBuf val, TCrmbotContext& ctx) const
{
    if (CheckEmail(val)) {
        ctx.SetStringSlot("email", FormatEmail(val));
        return true;
    }
    return false;
}

void TAuthorizingOrderFormHandler::TryFillSlots(TCrmbotContext& ctx) const
{
    // first grab the info provided with dialog initialization
    auto personalData = NCrmbot::TPersonalDataHelper(ctx.Ctx());
    auto email = personalData.GetEmail();
    auto phone = personalData.GetPhone();
    if (!email.Empty()) ctx.SetStringSlot("email", FormatEmail(email));
    if (!phone.Empty()) ctx.SetStringSlot("phone_number", FormatPhoneNumber(phone));

    // next handle tagger results
    TStringBuf orderIdNeuroSlotVal = ctx.GetStringSlot("order_id_neuro");
    if (!orderIdNeuroSlotVal.Empty())
        if (!TryFillOrderId(orderIdNeuroSlotVal, ctx))
            TryFillPhoneNumber(orderIdNeuroSlotVal, ctx);

    TStringBuf phoneNeuroSlotVal = ctx.GetStringSlot("phone_number_neuro");
    if (!phoneNeuroSlotVal.Empty())
        if (!TryFillPhoneNumber(phoneNeuroSlotVal, ctx))
            TryFillOrderId(phoneNeuroSlotVal, ctx);

    TStringBuf emailNeuroSlotVal = ctx.GetStringSlot("email_neuro");
    if (!emailNeuroSlotVal.Empty()) {
        // grab the original version from the Utterance
        TVector<TStringBuf> emailParts{};
        Split(emailNeuroSlotVal, " ", emailParts);
        TString emailReRaw = JoinRange(TStringBuf("[._=@]*"), emailParts.begin(), emailParts.end());
        std::regex emailRe(emailReRaw.c_str());
        std::smatch match{};
        std::regex_search(ctx.Utterance().data(), match, emailRe);
        TryFillEmail(match[0].str(), ctx);
    }
}

void TAuthorizingOrderFormHandler::HandleAnonymous(TCrmbotContext& ctx) const {
    TStringBuf orderIdSlotVal = ctx.GetStringSlot("order_id");
    TStringBuf phoneNumberSlotVal = ctx.GetStringSlot("phone_number");
    TStringBuf emailSlotVal = ctx.GetStringSlot("email");

    TMaybe<TCheckouterOrder> order;

    if (orderIdSlotVal.Empty() && phoneNumberSlotVal.Empty() && phoneNumberSlotVal.Empty()) {
        ctx.AddTextCardBlock("need_order_id_and_phone_or_email");
        return;
    } else if (!orderIdSlotVal.Empty()) {
        if (phoneNumberSlotVal.Empty() && emailSlotVal.Empty()) {
            ctx.AddTextCardBlock("need_phone_or_email");
            return;
        } else {
            TCheckouterClient client(ctx);
            order = client.GetOrderById(ctx.GetStringSlot("order_id"));
            CheckOrderPhoneAndEmail(order, emailSlotVal, phoneNumberSlotVal);
        }
    } else {
        ctx.AddTextCardBlock("need_order_id");
        return;
    }

    if (!order.Defined()) {
        NSc::TValue data{};
        data["orderId"] = orderIdSlotVal;
        data["email"] = emailSlotVal;
        data["phoneNumber"] = phoneNumberSlotVal;
        ctx.AddTextCardBlock("order_not_found", data);
    } else {
        MakeSingleOrderResponse(ctx, order.GetRef());
    }
}

void TAuthorizingOrderFormHandler::HandleAuthorized(TCrmbotContext& ctx, TStringBuf puid, TStringBuf muid) const
{
    TStringBuf orderIdSlotVal = ctx.GetStringSlot("order_id");
    if (!orderIdSlotVal.Empty()) { // The user wants a specific orderId
        TCheckouterClient client(ctx);
        TMaybe<TCheckouterOrder> order = client.GetOrderById(ctx.GetStringSlot("order_id"));
        if (
            !order.Defined() ||
            (
                order->Scheme()->Buyer().HasUid() &&
                order->Scheme()->Buyer().Uid() != static_cast<ui64>(std::stoi(puid.data()))
            ) || (
                order->Scheme()->Buyer().HasMuid() &&
                order->Scheme()->Buyer().Muid() != static_cast<ui64>(std::stoi(muid.data()))
            )
            )
        {
            NSc::TValue data{};
            data["orderId"] = orderIdSlotVal;
            ctx.AddTextCardBlock("order_not_found", data);
        } else {
            MakeSingleOrderResponse(ctx, order.GetRef());
        }
    } else {  // The user did not requested any specific order
        MakeOrderListResponse(ctx, puid.Empty() ? muid : puid);
    }
}

void TListingAuthorizingOrderFormHandler::MakeOrderListResponse(TCrmbotContext& ctx, TStringBuf uid) const
{
    auto checkouterClient = NCrmbot::TCheckouterClient(ctx);
    TVector<EOrderStatus> statuses = {EOrderStatus::UNPAID, EOrderStatus::PENDING, EOrderStatus::PROCESSING,
                                      EOrderStatus::DELIVERY, EOrderStatus::PICKUP};
    const auto & orders = checkouterClient.GetOrdersByUid(uid, statuses).GetResponse();
    NSc::TValue data{};
    data["orders"].SetArray();
    for (const auto& order: orders->Orders()) {
        NSc::TValue o{};
        o["id"] = order.Id();
        o["created_at"] = order->CreationDate();
        data["orders"].Push(o);
        ctx.AddSuggest("select_order", o);
    }
    ctx.AddTextCardBlock("respond_with_order_list", data);
}

}