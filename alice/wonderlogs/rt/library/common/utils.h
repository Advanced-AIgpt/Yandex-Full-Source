#pragma once

#include <util/generic/strbuf.h>
#include <util/system/types.h>

namespace NAlice::NWonderlogs {

ui64 GetShardByUuid(TStringBuf uuid, ui64 shards);

} // namespace NAlice::NWonderlogs
