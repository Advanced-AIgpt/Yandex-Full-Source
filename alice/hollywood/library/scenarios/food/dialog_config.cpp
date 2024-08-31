#include "dialog_config.h"

#include <util/generic/is_in.h>

template <>
struct THash<NAlice::NHollywood::NFood::TEventKey> {
    ui64 operator()(const NAlice::NHollywood::NFood::TEventKey& eventKey) const {
        const THash<TStringBuf> hash;
        return (hash(eventKey.GroupName) << 2) ^ hash(eventKey.FrameName);
    }
};

template <>
struct TEqualTo<NAlice::NHollywood::NFood::TEventKey> {
    bool operator()(const NAlice::NHollywood::NFood::TEventKey& lhs, const NAlice::NHollywood::NFood::TEventKey& rhs) const {
        return lhs == rhs;
    }
};

namespace NAlice::NHollywood::NFood {

using namespace std::literals;

static const THashSet<TEventKey> CLEAN_FRAMES = {
    {"main", "alice.food.main.reset_scenario"},
    {"exit", "alice.food.exit.exit_scenario"},
    {"form_order", "alice.food.form_order.decline"},
    {"main", "alice.food.main.start_mc"},
    {"main", "alice.food.main.can_order"},
    {"main", "alice.food.main.can_order_mc"},
};

bool IsFrameClean(const TEventKey& eventKey) {
    return CLEAN_FRAMES.contains(eventKey);
}

static const TVector<TString> NLG_HARDCODED_ITEMS = {
    "hardcoded_item_1", 
    "hardcoded_item_2", 
    "hardcoded_item_3",
};

const TVector<TString>& GetNlgHardcodedItems() {
    return NLG_HARDCODED_ITEMS;
}

static const THashSet<TString> EST_DISH_SUGGESTS = {
    "nlg_dish",
    "nlg_add_dish",
};

bool IsDishSuggest(const TString& suggest) {
    return EST_DISH_SUGGESTS.contains(suggest);
}

static const TVector<TString> TRIPLE_RANDOM_DISH_SUGGEST = {
    "nlg_dish",
    "nlg_dish",
    "nlg_dish",
};

static const TVector<TString> ORDER_SUGGEST = {
    "nlg_yes",
    "nlg_add_dish",
};

static const TVector<TString> ORDER_END_SUGGEST = {
    "nlg_enough",
    "nlg_add_dish",
};

static const TVector<TString> WHAT_CAN_YOU_DO_SUGGEST = {
    "nlg_what_can_you_do",
};

static const TDialogConfig DIALOG_CONFIG = {
    .FallbackTtl = TDuration::Minutes(5),
    .ShortMemoryTtl = TDuration::Days(1),
    .PlaceSlugTtl = TDuration::Minutes(10),
    .RequestTtl = TDuration::Minutes(10),
    .WeakFrames = {
        "alice.food.cart.add_item",
        "alice.food.main.can_order_mc",
    },
    .FrameGroups = {
        {"main", {
            .Frames = {
                // П: Сбрось сценарий еды.
                "alice.food.main.reset_scenario",       // -> ProcessResetScenario
                // П: Закажи в Макдональдсе.
                "alice.food.main.start_mc",             // -> ProcessStartMc
                // П: Закажи в Макдональдсе два
                //    гамбургера и большую картошку.
                "alice.food.main.start_mc_add_item",    // -> ProcessAddItem
                // П: Умеешь заказывать еду?
                // П: Закажи еду
                "alice.food.main.can_order",            // -> nlg_how_to_order_outside
                // П: Умеешь заказывать в Макдональдсе?
                "alice.food.main.can_order_mc",         // -> nlg_how_to_order_outside
                // П: Повтори заказ.
                "alice.food.main.repeat_last_order",    // -> ProcessRepeatLastOrder
            },
            .FallbackNlg = ""
        }},
        {"exit", {
            .Frames = {
                // П: Хватит.
                "alice.food.exit.exit_scenario"         // -> silent_exit_scenario
            },
            .FallbackNlg = ""
        }},
        {"cart", {
            .Frames = {
                // П: Закажи в Макдональдсе два
                //    гамбургера и большую картошку.
                "alice.food.main.start_mc_add_item",    // -> ProcessAddItem
                // П: Что ты умеешь?
                "alice.food.cart.what_you_can",         // -> nlg_how_to_order_inside
                // П: Очисть корзину.
                // П: Удали всё.
                "alice.food.cart.clear_cart",           // -> ProcessClearCart
                // П: Что у меня в корзине?
                "alice.food.cart.show_cart",            // -> ProcessShowCart
                // П: Добавь пирожок.
                // П: Гамбургер.
                "alice.food.cart.add_item",             // -> ProcessAddItem
                // П: Убери пирожок
                "alice.food.cart.remove_item",          // -> ProcessRemoveItem
                // П: Умеешь заказывать еду?
                // П: Закажи еду.
                "alice.food.main.can_order",            // -> nlg_how_to_order_inside
                // П: Умеешь заказывать в Макдональдсе?
                "alice.food.main.can_order_mc",         // -> nlg_how_to_order_inside
                // П: Откуда еда?
                "alice.food.cart.where_from_order",     // -> nlg_order_from_mcdonalds
            },
            .FallbackNlg = ""
        }},
        {"what_you_wish", {
            .Frames = {
                // П: Ничего.
                "alice.food.common.nothing",            // -> ProcessNothingElse
            },
            .FallbackNlg = "fallback_what_you_wish"     // А: Не очень поняла. Что будем заказывать?
        }},
        {"what_you_wish_no_fallback", {
            .Frames = {
                // П: Ничего.
                "alice.food.common.nothing",            // -> ProcessNothingElse
            },
            .FallbackNlg = ""
        }},
        {"something_else", {
            .Frames = {
                // П: Да.
                "alice.food.common.agree",              // -> nlg_what_you_wish
                // П: Нет.
                "alice.food.common.decline",            // -> ProcessNothingElse
                // П: Ничего.
                "alice.food.common.nothing",            // -> ProcessNothingElse
            },
            .FallbackNlg = "fallback_something_else"    // А: Не очень поняла. Добавить что-нибудь в корзину?
        }},
        {"keep_old_cart", {
            .Frames = {
                // П: Да, оставь.
                "alice.food.keep_old_cart.agree",       // -> nlg_cart_resume_order
                // П: Нет, очисть.
                "alice.food.keep_old_cart.decline",     // -> ProcessDeclineKeepOldCart
                // П: Что там?
                "alice.food.keep_old_cart.show_cart",   // -> ProcessShowCart
            },
            .FallbackNlg = "fallback_keep_old_cart"     // А: Не очень поняла вас. Продолжим оформлять заказ?
        }},
        {"begin_new_cart", {
            .Frames = {
                // П: Да.
                "alice.food.common.agree",              // -> ProcessBeginNewCart -> nlg_what_you_wish
                // П: Нет.
                "alice.food.common.decline",            // -> silent_exit_scenario
            },
            .FallbackNlg = "fallback_begin_new_cart"    // А: Хм, чтобы сделать заказ, скажи мне: «Алиса, закажи картошку фри в Макдоналдсе».
        }},
        {"form_order", {
            .Frames = {
                // П: Всё, заказывай.
                "alice.food.form_order.agree",          // -> ProcessFormOrderAgree
                // П: Нет.
                "alice.food.form_order.decline",        // -> nlg_form_order_decline
            },
            .FallbackNlg = "fallback_form_order"        // А: Не очень поняла вас. Что-то ещё?
        }},
        {"confirm_order", {
            .Frames = {
                // П: Да, оформляй.
                "alice.food.confirm_order.agree",       // -> ProcessConfirmOrderAgree
                // П: Нет.
                "alice.food.confirm_order.decline",     // -> nlg_confirm_order_decline
            },
            .FallbackNlg = "fallback_confirm_order"     // А: Не расслышала что вы сказали, оформляем заказ?
        }},
        {"go_to_app", {
            .Frames = {
                // П: Да.
                "alice.food.common.agree",              // -> ProcessGoToApp
                // П: Нет.
                "alice.food.common.decline",            // -> nlg_ok_what_you_wish
            },
            .FallbackNlg = "fallback_go_to_app"         // А: Извините, я вас не поняла. Продолжить заказ в телефоне?
        }},
    },
    .Responses = {
        {"silent_exit_scenario",            {RCF_SILENT, {}, {}}},
        {"silent_irrelevant",               {RCF_SILENT, {}, {}}},
        {"nlg_internal_error",              {0, {}, {}}},           // А: Произошла ошибка в сценарии еды... Прислала вам ссылку в телефон.

        {"nlg_no_response_from_eda",        {                       // А: Мне не хватает данных, чтобы оформить заказ. Зайдите в приложение.
            0,
            {},
            WHAT_CAN_YOU_DO_SUGGEST,
        }},
        {"nlg_mcdonalds_not_found",         {                       // А: Все Макдональдсы слишком далеко от вас. Не могу сделать заказ. Может вам рецепт шарлотки рассказать? :)
            RCF_LISTEN,
            {},
            {"nlg_charlotte_recipie", "nlg_closest_McD"},
        }},
        {"nlg_how_to_order_outside",        {                       // А: Чтобы сделать заказ... Что будем заказывать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish_no_fallback"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_how_to_order_inside",         {                       // А: Чтобы сделать заказ... Что будем заказывать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_start_onboarding",            {                       // А: Чтобы сделать заказ... Что будем заказывать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            {},
        }},
        {"nlg_what_you_wish",               {                       // А: Что закажем?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_ok_what_you_wish",            {                       // А: Хорошо. Что будем заказывать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_order_from_mcdonalds",        {
            RCF_LISTEN,
            {},
            {},
            true,
        }},
        {"nlg_keep_old_cart",               {                       // А: В вашей корзине уже есть заказ. Продолжим?
            RCF_LISTEN,
            {"exit", "cart", "keep_old_cart"},
            {"nlg_yes", "nlg_new_order"},
        }},
        {"nlg_begin_new_cart",              {                       // А: Хорошо. Сделаете новый заказ?
            RCF_LISTEN,
            {"exit", "cart", "begin_new_cart"},
            {"nlg_yes", "nlg_no"},
        }},

        {"nlg_last_order_not_found",        {                       // А: Последних заказов нет. Хотите сделать новый заказ?
            RCF_LISTEN,
            {"exit", "cart", "begin_new_cart"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_last_order_too_far",          {                       // А: Последних заказов нет. Хотите сделать новый заказ?
            RCF_LISTEN,
            {"exit", "cart", "begin_new_cart"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_last_order_content",          {                       // А: Последний раз вы заказывали... Заказываем?
            RCF_LISTEN,
            {"exit", "cart", "confirm_order"},
            ORDER_SUGGEST,
        }},

        {"nlg_cart_add_first_items",        {                       // А: Всё или что-то ещё?
            RCF_LISTEN,
            {"exit", "cart", "form_order"},
            ORDER_END_SUGGEST,
        }},
        {"nlg_cart_add_new_items",          {                       // А: Что ещё вы хотите заказать?
            RCF_LISTEN,
            {"exit", "cart", "form_order"},
            ORDER_END_SUGGEST,
        }},
        {"nlg_cart_resume_order",          {                        // А: Что-нибудь ещё?
            RCF_LISTEN,
            {"exit", "cart", "form_order"},
            ORDER_END_SUGGEST,
        }},
        {"nlg_cart_add_unavailable",        {                       // А: Сейчас в Макдональдсе нет <блюда>. Заказать что-нибудь другое?
            RCF_LISTEN,
            {"exit", "cart", "something_else"},
            {},
        }},
        {"nlg_cart_add_unknown",            {                       // А: Не могу найти в меню блюдо «пицца». Может продолжить заказ в телефоне?
            RCF_LISTEN,
            {"exit", "cart", "go_to_app"},
            WHAT_CAN_YOU_DO_SUGGEST,
        }},
        {"nlg_cart_add_unknown_go_to_app",  {
            0,
            {},
            WHAT_CAN_YOU_DO_SUGGEST,
        }},

        {"nlg_remove_item_from_empty_cart", {                       // А: Корзина уже пустая. Что хотите заказать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_remove_item_not_found",       {                       // А: Не могу найти «пирожок» в вашей корзине.
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_remove_item_ok_last_item",    {                      // А: Удалила «пирожок вишнёвый». Корзина пуста. Что хотите заказать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_remove_item_ok",              {                       // А: Удалила «пирожок вишнёвый». Сумма вашего заказа теперь 123 р. Заказать что-нибудь ещё?
            RCF_LISTEN,
            {"exit", "cart", "something_else"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},

        {"nlg_cart_show_empty",             {                       // А: В вашей корзине ничего нет. Что туда положить?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_cart_show_outdated",             {                    // А: Все блюда из вашей корзины устарели. Что хотите заказать?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_cart_show_ok",                {                       // А: В корзине <список>... Заказываем?
            RCF_LISTEN,
            {"exit", "cart", "confirm_order"},
            ORDER_SUGGEST,
        }},

        {"nlg_cart_clear_empty",            {                       // А: Корзина уже пустая. Что туда положить?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},
        {"nlg_cart_clear_ok",               {                       // А: Ок, корзина пуста. Что туда положить?
            RCF_LISTEN,
            {"exit", "cart", "what_you_wish"},
            TRIPLE_RANDOM_DISH_SUGGEST,
        }},

        {"nlg_order_details",               {                       // А: В корзине <список>... Заказываем?
            RCF_LISTEN,
            {"exit", "cart", "confirm_order"},
            {},
        }},
        {"nlg_cart_was_pushed_to_app",      {                       // А: Отправила вам пуш в приложении Яндекса. Завершите заказ там.
            0,
            {},
            WHAT_CAN_YOU_DO_SUGGEST,
        }},
        {"nlg_order_checkout_push",         {                       // А: Откройте приложение для оплаты. Прислала вам ссылку в телефон.
            0,
            {},
            WHAT_CAN_YOU_DO_SUGGEST,
        }},
        {"nlg_form_order_decline",          {0, {}, {}}},           // А: Ок. Если захотите, сможем завершить заказ в другой раз.
        {"nlg_confirm_order_decline",       {0, {}, {}}},           // А: Ок. Если захотите, сможем завершить заказ в другой раз.
        {"nlg_reset_scenario_ok",           {0, {}, {}}},           // А: Хорошо.
        {"nlg_cancel_order",                {0, {}, {}}},           // А: Отменила заказ.
    }
};

const TDialogConfig& GetDialogConfig() {
    return DIALOG_CONFIG;
}

// ~~~~ TDialogConfig ~~~~

bool TDialogConfig::IsFrameMain(TStringBuf frameName) const {
    return IsFrameInGroup(frameName, "main");
}

bool TDialogConfig::IsFrameCart(TStringBuf frameName) const {
    return IsFrameInGroup(frameName, "cart");
}

bool TDialogConfig::IsFrameAction(TStringBuf frameName) const {
    return !IsFrameMain(frameName) && !IsFrameCart(frameName);
}

bool TDialogConfig::IsFrameWeak(TStringBuf frameName) const {
    return WeakFrames.contains(frameName);
}

bool TDialogConfig::IsFrameInGroup(TStringBuf frameName, TStringBuf groupName) const {
    const TFrameGroupConfig* group = FrameGroups.FindPtr(groupName);
    return group != nullptr && IsIn(group->Frames, frameName);
}

const TFrameGroupConfig& TDialogConfig::GetFrameGroup(TStringBuf groupName) const {
    const TFrameGroupConfig* group = FrameGroups.FindPtr(groupName);
    Y_ENSURE(group != nullptr, "Unknown frame group name: " << groupName);
    return *group;
}

const TResponseConfig& TDialogConfig::GetResponse(TStringBuf name) const {
    const TResponseConfig* response = Responses.FindPtr(name);
    Y_ENSURE(response != nullptr, "Unknown Alice response name: " << name);
    return *response;
}

} // namespace NAlice::NHollywood::NFood
