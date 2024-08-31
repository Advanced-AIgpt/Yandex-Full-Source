LIBRARY()

OWNER(
    smirnovpavel
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/sample_features
    search/begemot/core
    search/begemot/rules/alice/sample_features/proto
)

SRCS(
    sample_features_converter.cpp
)

END()
