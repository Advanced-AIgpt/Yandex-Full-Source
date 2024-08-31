LIBRARY(default_item_selector)

OWNER(
    volobuev
    g:alice_quality
)

PEERDIR(
    alice/nlu/libs/binary_classifier
    alice/nlu/libs/item_selector/common
    alice/nlu/libs/item_selector/interface
    alice/library/parsed_user_phrase
    library/cpp/dot_product
    library/cpp/langs
    library/cpp/resource
    util
)

SRCS(
    default.cpp
)

RESOURCE(
    alice/nlu/libs/item_selector/default/data/idfs.tsv /idfs.tsv
)

END()

RECURSE_FOR_TESTS(
    ut
)
