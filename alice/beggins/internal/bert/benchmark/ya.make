G_BENCHMARK()

OWNER(alkapov)

PEERDIR(
    alice/beggins/internal/bert

    quality/relev_tools/bert_models/lib/without_tf
)

DATA(sbr://2380054539=test_models)

SRCS(
    embedder_benchmark.cpp
)

END()
