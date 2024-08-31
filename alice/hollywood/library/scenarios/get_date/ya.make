LIBRARY()

OWNER(
    d-dima
    g:hollywood
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/scenarios/get_date/nlg
    alice/hollywood/library/scenarios/get_date/proto
    alice/hollywood/library/scenarios/get_date/slot_utils
    alice/library/analytics/common
    alice/library/json
    alice/library/logger
    alice/library/sys_datetime
    alice/protos/data/entities
    library/cpp/geobase
    library/cpp/timezone_conversion
)

SRCS(
    GLOBAL get_date.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
    it2
)
