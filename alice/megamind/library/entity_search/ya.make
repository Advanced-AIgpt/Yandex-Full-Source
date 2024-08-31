LIBRARY()

OWNER(g:megamind)

PEERDIR(
    alice/bass/libs/fetcher
    alice/megamind/library/requestctx
    alice/megamind/library/sources
    alice/megamind/library/speechkit
    alice/megamind/library/util
    library/cpp/json
)

SRCS(
    entity_finder.cpp
    entity_search.cpp
    response.cpp
)

END()

RECURSE_FOR_TESTS(ut)
