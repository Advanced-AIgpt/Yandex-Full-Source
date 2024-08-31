#pragma once

#include "build_options.h"

#include <library/cpp/hnsw/index_builder/internal_build_options.h>

#include <util/generic/typetraits.h>

namespace NYtHnsw {

    struct THnswInternalYtBuildOptions : NHnsw::THnswInternalBuildOptions {
        size_t MaxItemsForLocalBuild;
        size_t NumNearestCandidates;
        size_t AdditionalMemoryLimit;
        size_t NumJoinPartitionsJobs;
        size_t NumTrimNeighborsJobs;
        size_t CpuLimit;

        THnswInternalYtBuildOptions() = default;

        explicit THnswInternalYtBuildOptions(const THnswYtBuildOptions& opts)
            : NHnsw::THnswInternalBuildOptions(opts)
        {
            MaxItemsForLocalBuild = opts.MaxItemsForLocalBuild;
            NumNearestCandidates = opts.NumNearestCandidates;
            AdditionalMemoryLimit = opts.AdditionalMemoryLimit;
            NumJoinPartitionsJobs = opts.NumJoinPartitionsJobs;
            NumTrimNeighborsJobs = opts.NumTrimNeighborsJobs;
            CpuLimit = Min(opts.CpuLimit, NumThreads);
        }
    };

    void UpdateItemLimits(THnswInternalYtBuildOptions& opts, ui64 maxShardSize);

}

Y_DECLARE_PODTYPE(NHnsw::THnswInternalBuildOptions);
Y_DECLARE_PODTYPE(NYtHnsw::THnswInternalYtBuildOptions);

