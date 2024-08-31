LIBRARY()

OWNER(
    alkapov
)

PEERDIR(
    kernel/search_query
    quality/relev_tools/bert_models/input_parsing_lib
    quality/relev_tools/bert_models/lib/without_tf
)

SRCS(
    embedder.cpp
)

END()

RECURSE_FOR_TESTS(
    benchmark
    ut
)
