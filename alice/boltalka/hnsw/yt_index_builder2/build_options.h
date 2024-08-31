#pragma once
#include <library/cpp/hnsw/index_builder/build_options.h>

#include <util/generic/typetraits.h>

namespace NYtHnsw {

    struct THnswYtBuildOptions : NHnsw::THnswBuildOptions {
        size_t MaxItemsForLocalBuild;
        size_t NumNearestCandidates;
        size_t AdditionalMemoryLimit;
        size_t NumJoinPartitionsJobs;
        size_t NumTrimNeighborsJobs;
        size_t CpuLimit;
    };

}

Y_DECLARE_PODTYPE(NHnsw::THnswBuildOptions);
Y_DECLARE_PODTYPE(NYtHnsw::THnswYtBuildOptions);

