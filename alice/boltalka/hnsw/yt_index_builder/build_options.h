#pragma once
#include <library/cpp/hnsw/index_builder/build_options.h>

#include <util/generic/typetraits.h>

namespace NHnsw {

    struct THnswYtBuildOptions : THnswBuildOptions {
        size_t AnnSearchJobCount = AutoSelect;
        size_t AnnSearchMemoryLimit = AutoSelect;
        size_t AnnSearchItemStorageMemoryLimit = AutoSelect;
        size_t ExactSearchJobCount = AutoSelect;
        size_t ExactSearchMemoryLimit = AutoSelect;
        size_t ExactSearchItemStorageMemoryLimit = AutoSelect;

        void CheckOptions() const {
            THnswBuildOptions::CheckOptions();
            Y_VERIFY(AnnSearchMemoryLimit == THnswBuildOptions::AutoSelect ||
                    AnnSearchItemStorageMemoryLimit == THnswBuildOptions::AutoSelect ||
                    AnnSearchMemoryLimit >= AnnSearchItemStorageMemoryLimit);
            Y_VERIFY(ExactSearchMemoryLimit == THnswBuildOptions::AutoSelect ||
                    ExactSearchItemStorageMemoryLimit == THnswBuildOptions::AutoSelect ||
                    ExactSearchMemoryLimit >= ExactSearchItemStorageMemoryLimit);
        }
    };

}

Y_DECLARE_PODTYPE(NHnsw::THnswBuildOptions);
Y_DECLARE_PODTYPE(NHnsw::THnswYtBuildOptions);

