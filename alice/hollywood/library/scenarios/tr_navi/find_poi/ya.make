LIBRARY()

OWNER(
    ardulat
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/find_poi/nlg
    alice/nlu/libs/request_normalizer
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL find_poi.cpp
)

END()

RECURSE_FOR_TESTS(it)
