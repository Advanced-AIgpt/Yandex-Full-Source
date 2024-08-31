LIBRARY()

OWNER(
    vl-trifonov
    g:hollywood
)

SRCS(
    nlu_features.cpp
)

PEERDIR(
    alice/hollywood/library/request
    alice/protos/api/nlu/generated
)

END()
