#pragma once

#include <alice/library/proto/proto.h>

#include <apphost/api/service/cpp/service.h>

namespace NAlice::NHollywood {

inline constexpr TStringBuf HTTP_REQUEST_ITEM = "http_request";
inline constexpr TStringBuf REQUEST_ITEM = "mm_scenario_request";
inline constexpr TStringBuf RESPONSE_ITEM = "mm_scenario_response";
inline constexpr TStringBuf MODIFIER_REQUEST_ITEM = "mm_modifier_request";
inline constexpr TStringBuf MODIFIER_RESPONSE_ITEM = "mm_modifier_response";
inline constexpr TStringBuf RENDER_DATA_ITEM = "render_data";
inline constexpr TStringBuf RENDER_DATA_RESULT = "render_result";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_RESPONSE_NAME = "combinator_response_apphost_type";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_REQUEST_NAME = "combinator_request_apphost_type";
inline constexpr TStringBuf AH_ITEM_COMBINATOR_CONTINUE_REQUEST = "combinator_continue_request_apphost_type";
inline constexpr TStringBuf BLACKBOX_HTTP_RESPONSE_ITEM = "blackbox_http_response";
inline constexpr TStringBuf COMBINATOR_USED_SCENARIOS_ITEM = "combinator_used_scenarios";
inline constexpr TStringBuf AH_ITEM_FULL_MEMENTO_DATA = "full_memento_data";

template <typename TValue>
[[nodiscard]] inline const TValue& GetOnlyItemOrThrow(const NAppHost::TContextItemRefArrayTemplate<TValue>& items, const TStringBuf type) {
    Y_ENSURE(items.size() == 1, type << " expected 1 item, got " << items.size());
    return items.front();
}

template <typename TValue>
[[nodiscard]] inline const TValue* GetMaybeOnlyItem(const NAppHost::TContextItemRefArrayTemplate<TValue>& items) {
    if (items.size() != 1) {
        return nullptr;
    }
    return &items.front();
}

template <typename TProto>
[[nodiscard]] inline TProto GetOnlyProtoOrThrow(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
    auto proto = ParseProto<TProto>(GetOnlyItemOrThrow(ctx.GetProtobufItemRefs(type), type).Raw());
    return proto;
}

template <typename TProto>
[[nodiscard]] inline TMaybe<TProto> GetMaybeOnlyProto(const NAppHost::IServiceContext& ctx, const TStringBuf type) {
    const auto* item = GetMaybeOnlyItem(ctx.GetProtobufItemRefs(type));
    if (!item) {
        return Nothing();
    }
    return ParseProto<TProto>(item->Raw());
}

} // namespace NAlice::NHollywood
