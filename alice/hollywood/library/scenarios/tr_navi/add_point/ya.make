LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/add_point/nlg
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL add_point.cpp
)

END()

RECURSE_FOR_TESTS(it)
