UNITTEST()

OWNER(
    vl-trifonov
    g:alice_quality
)

PEERDIR(
    alice/library/json
    alice/nlu/proto/dataset_info
    alice/protos/data/language
    library/cpp/json
)

DEPENDS(
    alice/nlu/data/ar/datasets
)

SRCS(
    validation_test.cpp
)

END()
