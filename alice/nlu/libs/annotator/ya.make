LIBRARY(annotator)

OWNER(
    smirnovpavel
    g:alice_quality
)

SRCS(
    annotator.cpp
)

PEERDIR(
    library/cpp/text_processing/tokenizer
    library/cpp/containers/comptrie
    library/cpp/packers
    util
)

END()

RECURSE_FOR_TESTS(
    ut
)
