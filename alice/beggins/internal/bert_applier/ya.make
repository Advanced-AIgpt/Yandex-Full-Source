LIBRARY()

OWNER(
    alkapov
)

IF (LINUX AND NOT SANITIZER_TYPE)
    CFLAGS("-DUSE_GPU=1 -DUSE_CUDA=1 -DCUDA_VERSION=11.4")
ENDIF()

PEERDIR(
    kernel/search_query
    library/cpp/float16
    library/cpp/threading/cancellation
    library/cpp/threading/future
    library/cpp/time_provider
    quality/relev_tools/bert_models/lib/model_descr_metadata
    quality/relev_tools/bert_models/lib/without_tf
)

SRCS(
    applier.cpp
)

END()
