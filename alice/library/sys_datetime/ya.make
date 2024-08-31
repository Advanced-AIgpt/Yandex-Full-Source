LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

SRCS(
    sys_datetime.cpp
)

PEERDIR(
    alice/protos/data/entities
    library/cpp/json
    library/cpp/timezone_conversion
)

END()

RECURSE_FOR_TESTS(ut)
