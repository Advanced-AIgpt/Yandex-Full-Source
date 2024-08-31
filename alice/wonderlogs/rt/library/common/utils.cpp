#include "utils.h"

#include <alice/wonderlogs/library/common/utils.h>

namespace NAlice::NWonderlogs {

ui64 GetShardByUuid(const TStringBuf uuid, ui64 shards) {
    return HashStringToUi64(uuid) % shards;
}

} // namespace NAlice::NWonderlogs
