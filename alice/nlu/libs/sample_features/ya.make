LIBRARY(sample_features)

OWNER(
    smirnovpavel
)

PEERDIR(
    alice/nlu/granet/lib/sample
    alice/nlu/libs/token_aligner
    alice/nlu/libs/tokenization
    library/cpp/langs
)

SRCS(
    sample_features.cpp
)

END()

RECURSE_FOR_TESTS(ut)
