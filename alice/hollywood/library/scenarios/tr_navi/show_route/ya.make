LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/show_route/nlg
    alice/nlu/libs/request_normalizer
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL show_route.cpp
)

END()

RECURSE_FOR_TESTS(it)
