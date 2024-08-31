#pragma once

#include "context.h"

#include <alice/bass/util/error.h>

namespace NBASS {

namespace NMarket {

/**
 * @brief Предлагает пользователю авторизоваться
 *
 * Важно: вызывать нужно на незалогиненном пользователе.
 * Выводит suggest, предлагающий авторизоваться, а также
 * suggest, отправляющий фразу "Я залогинен" для
 * продолжения сценария.
 *
 * Для использования надо сделать nlgimport "market/market_login.nlg"
 *
 * @param ctx Маркетовый контекст
 * @param isLoginForm true, если пользователь опять утверждает, что залогинился
 *
 * @return TResultValue
 */
TResultValue HandleGuest(TMarketContext& ctx, bool isLoginForm);

} // namespace NMarket

} // namespace NBASS
