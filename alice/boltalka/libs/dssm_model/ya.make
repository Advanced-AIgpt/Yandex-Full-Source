LIBRARY()

OWNER(
    alipov
    g:alice_boltalka
)

PEERDIR(
    alice/boltalka/libs/text_utils
    ml/dssm/dssm/lib
    util
    library/cpp/json
)

SRCS(
    dssm_model.cpp
    dssm_pool.cpp
)

END()
