LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/library/geo/protos
    alice/library/json
    alice/protos/data
    kernel/geodb
    library/cpp/geobase
    library/cpp/resource
    library/cpp/scheme
)

SRCS(
    geodb.cpp
    user_location.cpp
)

GENERATE_ENUM_SERIALIZATION(geodb.h)

END()

RECURSE_FOR_TESTS(
    ut
)
