PROGRAM()

OWNER(volobuev)

DEPENDS(
    alice/nlu/data/ru/models/alice_item_selector/video_gallery
    alice/nlu/data/ru/models/alice_token_embedder
)

PEERDIR(
    util
    library/cpp/getopt
    library/cpp/json
    library/cpp/dot_product
    alice/nlu/libs/item_selector/catboost_item_selector
    alice/nlu/libs/binary_classifier
)

SRCS(main.cpp)

END()
