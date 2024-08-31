#pragma once

#include <util/generic/string.h>

namespace NMatrix::NNotificator::NAnalytics {

static inline constexpr TStringBuf PUSH_ID_COLUMN_NAME = "push_id";
static inline constexpr TStringBuf PUSH_EVENT_TYPE_ID_COLUMN_NAME = "event_type_id";
static inline constexpr TStringBuf PUSH_EVENT_COLUMN_NAME = "event";
static inline constexpr TStringBuf AGGREGATED_EVENTS_COLUMN_NAME = "events";
static inline constexpr TStringBuf TIMESTAMP_COLUMN_NAME = "timestamp";
static inline constexpr TStringBuf MIN_TIMESTAMP_COLUMN_NAME = "min_timestamp";
static inline constexpr TStringBuf MAX_TIMESTAMP_COLUMN_NAME = "max_timestamp";
static inline constexpr TStringBuf VALIDATION_RESULT_COLUNM_NAME = "validation_result";
static inline constexpr TStringBuf TAG_COLUMN_NAME = "user_tag";
static inline constexpr TStringBuf DELIVERED_COLUMN_NAME = "delivered";

} // namespace NMatrix::NNotificator::NAnalytics
