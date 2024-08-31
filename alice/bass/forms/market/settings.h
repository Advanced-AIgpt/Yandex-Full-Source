#pragma once

#include <util/datetime/base.h>

namespace NBASS {

namespace NMarket {

inline constexpr TDuration MAX_CHECKOUT_WAIT_DURATION = TDuration::Seconds(15);
inline constexpr TDuration MAX_REQUEST_HANDLE_DURATION = TDuration::Seconds(2);

inline constexpr TDuration CHECKOUT_ORDERS_HISTORY_DEPTH = TDuration::Days(180);

inline constexpr ui32 MAX_CHECKOUT_ATTEMPTS = 3;
inline constexpr ui32 MAX_CHECKOUT_STEP_ATTEMPTS_COUNT = 4;

inline constexpr ui32 MAX_DELIVERY_OPTIONS_COUNT = 6;

inline constexpr ui32 ADVERTISEMENT_GALLERY_PLACE = 1;

inline constexpr size_t MAX_MARKET_RESULTS = 10;

inline constexpr size_t MAX_GALLERY_MODEL_ADVANTAGES = 2;

inline constexpr ui32 MAX_ITEMS_NUMBER_SUGGESTS = 5;

inline constexpr size_t MAX_OFFERS_IN_OFFERS_CARD = 3;

inline constexpr ui32 MARKET_ORDERS_PAGE_SIZE = 10;
} // namespace NMarket

} // namespace NBASS
