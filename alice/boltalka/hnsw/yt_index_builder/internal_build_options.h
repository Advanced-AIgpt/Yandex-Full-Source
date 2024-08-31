#pragma once

#include "build_options.h"

#include <library/cpp/hnsw/index_builder/internal_build_options.h>

#include <util/generic/typetraits.h>

namespace NHnsw {

    struct THnswInternalYtBuildOptions : THnswInternalBuildOptions {
        size_t AnnSearchJobCount;
        size_t AnnSearchMemoryLimit;
        size_t AnnSearchItemStorageMemoryLimit;
        size_t ExactSearchJobCount;
        size_t ExactSearchMemoryLimit;
        size_t ExactSearchItemStorageMemoryLimit;

        THnswInternalYtBuildOptions() = default;

        explicit THnswInternalYtBuildOptions(const THnswYtBuildOptions& opts)
            : THnswInternalBuildOptions(opts)
        {
            AnnSearchJobCount = opts.AnnSearchJobCount;
            AnnSearchMemoryLimit = opts.AnnSearchMemoryLimit;
            AnnSearchItemStorageMemoryLimit = opts.AnnSearchItemStorageMemoryLimit;
            ExactSearchJobCount = opts.ExactSearchJobCount;
            ExactSearchMemoryLimit = opts.ExactSearchMemoryLimit;
            ExactSearchItemStorageMemoryLimit = opts.ExactSearchItemStorageMemoryLimit;
            if (opts.AnnSearchItemStorageMemoryLimit == THnswBuildOptions::AutoSelect) {
                AnnSearchItemStorageMemoryLimit =
                    opts.AnnSearchMemoryLimit == THnswBuildOptions::AutoSelect ? 0 : opts.AnnSearchMemoryLimit / 2;
            }
            if (opts.ExactSearchItemStorageMemoryLimit == THnswBuildOptions::AutoSelect) {
                ExactSearchItemStorageMemoryLimit =
                    opts.ExactSearchMemoryLimit == THnswBuildOptions::AutoSelect ? 0 : opts.ExactSearchMemoryLimit / 2;
            }
        }
    };
}

Y_DECLARE_PODTYPE(NHnsw::THnswInternalBuildOptions);
Y_DECLARE_PODTYPE(NHnsw::THnswInternalYtBuildOptions);

