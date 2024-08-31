LIBRARY(embedder)

OWNER(
    dan-anastasev
    g:alice_quality
)

PEERDIR(
    library/cpp/containers/comptrie
    library/cpp/json
    util
)

SRCS(
    embedder.cpp
    embedding_loading.cpp
)

END()

RECURSE_FOR_TESTS(ut)
