LIBRARY()

OWNER(
    nstbezz
    g:voicetech-infra
)


PEERDIR(
    alice/cuttlefish/library/surface_mapper
    library/cpp/json
    library/cpp/string_utils/base64
)

USE_PYTHON3()

SRCS(
    uaas_mapper.cpp
)

PY_SRCS(
    uaas_mapper.pyx
)

END()


RECURSE_FOR_TESTS(
    ut
    pytest
)
