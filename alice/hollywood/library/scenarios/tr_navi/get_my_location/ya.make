LIBRARY()

OWNER(
    ardulat
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/get_my_location/nlg
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL get_my_location.cpp
)

END()

RECURSE_FOR_TESTS(it)
