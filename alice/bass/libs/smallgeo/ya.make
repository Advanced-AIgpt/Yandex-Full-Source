LIBRARY()

OWNER(g:bass)

CFLAGS(-DPIRE_NO_CONFIG)

PEERDIR(
    alice/bass/libs/logging_v2
    kernel/geodb
    library/cpp/geobase
    library/cpp/geolocation
    library/cpp/regex/pire
    library/cpp/scheme
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

FROM_SANDBOX(FILE 664611139 OUT_NOAUTO regions.bin)
RESOURCE(regions.bin regions.data)
RESOURCE(data/smallgeo_fixlist.json smallgeo_fixlist.json)
RESOURCE(data/latlon_fixlist.json latlon_fixlist.json)

END()
