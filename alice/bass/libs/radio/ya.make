LIBRARY()

OWNER(g:bass)

FROM_SANDBOX(FILE 2342535241 OUT_NOAUTO tracks_to_fm_radio.json)
FROM_SANDBOX(FILE 2340937614 OUT_NOAUTO metatags_to_fm_radio.json)

RESOURCE(tracks_to_fm_radio.json tracks_to_fm_radio.json)
RESOURCE(metatags_to_fm_radio.json metatags_to_fm_radio.json)

PEERDIR(
    alice/bass/libs/logging_v2
    alice/bass/libs/smallgeo
    library/cpp/resource
    library/cpp/scheme
    library/cpp/unistat
)

SRCS(
    fmdb.cpp
    recommender.cpp
)

END()
