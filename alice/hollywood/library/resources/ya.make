LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/config
    alice/library/logger
    library/cpp/geobase
)

SRCS(
    common_resources.cpp
    geobase.cpp
    nlg_translations.cpp
    resources.cpp
)

END()

RECURSE_FOR_TESTS(ut)
