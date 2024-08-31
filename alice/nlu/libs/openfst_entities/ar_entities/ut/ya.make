UNITTEST()

OWNER(
    moath-alali
    g:alice_quality
)

NO_COMPILER_WARNINGS()

SRCS(
    ar_entities_ut.cpp
)

PEERDIR(
    alice/nlu/libs/openfst_entities/ar_entities
    alice/nlu/libs/normalization
    library/cpp/testing/common
    alice/library/json
)

DEPENDS(
    alice/nlu/data/ar/models/openfst_entities
)

END()
