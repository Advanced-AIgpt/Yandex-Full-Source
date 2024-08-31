LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/get_weather/nlg
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL get_weather.cpp
)

END()

RECURSE_FOR_TESTS(it)
