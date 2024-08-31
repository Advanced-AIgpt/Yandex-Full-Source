LIBRARY()

OWNER(
    nzinov
    g:alice_boltalka
)

SRCS(
    main.h
)

IF (USE_GPU)
    CFLAGS(-DUSE_GPU)
ENDIF()

PEERDIR(
    dict/mt/libs/nn/ynmt/config_helper
    dict/mt/libs/nn/ynmt/extra
    dict/mt/libs/nn/ynmt_backend/cpu
    dict/mt/libs/nn/ynmt_backend/gpu
    kernel/bert
    library/cpp/float16
    library/cpp/getopt/small
)

END()
