#include "internal_build_options.h"

namespace NYtHnsw {

void UpdateItemLimits(THnswInternalYtBuildOptions& opts, ui64 maxShardSize) {
    opts.MaxItemsForLocalBuild = Min(opts.MaxItemsForLocalBuild, maxShardSize + 1);
}

}
