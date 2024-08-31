LIBRARY()

OWNER(g:megamind)

CFLAGS(-DPIRE_NO_CONFIG)

PEERDIR(
    kernel/geodb
    library/cpp/geobase
    library/cpp/geolocation
    library/cpp/regex/pire
)

SRCS(
    engine.cpp
    kdtree.cpp
    latlon.cpp
    region.cpp
    result.cpp
    utils.cpp
    vocabulary.cpp
)

END()

RECURSE_FOR_TESTS(ut)
