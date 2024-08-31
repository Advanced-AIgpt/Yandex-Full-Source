#pragma once

#include <util/generic/strbuf.h>

namespace NAlice::NMegamind {

inline constexpr TStringBuf MM_RPC_REQUEST_ITEM_NAME = "mm_rpc_request";
inline constexpr TStringBuf MM_RPC_RESPONSE_ITEM_NAME = "mm_rpc_response";
inline constexpr TStringBuf SCENARIO_RPC_REQUEST_ITEM_NAME_PREFIX = "rpc_request_";
inline constexpr TStringBuf SCENARIO_RPC_RESPONSE_ITEM_NAME = "rpc_response";

} // namespace NAlice::NMegamind
