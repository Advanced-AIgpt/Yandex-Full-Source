LIBRARY()

OWNER(
    movb
)

PEERDIR(
    kernel/inflectorlib/phrase/simple
    library/cpp/containers/comptrie
    quality/trailer/suggest/data_structs
    alice/nlu/query_wizard_features/proto
)

SRCS(
    reader.cpp
)

END()
