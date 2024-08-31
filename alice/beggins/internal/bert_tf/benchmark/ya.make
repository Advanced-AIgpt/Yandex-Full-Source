G_BENCHMARK()

SRCS(
    benchmark_applier.cpp
)

SIZE(
    MEDIUM
)

PEERDIR(
    alice/beggins/internal/bert_tf
    library/cpp/testing/common
)

DEPENDS(
    alice/beggins/internal/bert_tf/ut/data
)

END()
