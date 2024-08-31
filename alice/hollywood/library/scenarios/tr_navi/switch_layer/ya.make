LIBRARY()

OWNER(
    flimsywhimsy
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/switch_layer/nlg
)

SRCS(
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL switch_layer.cpp
)

END()

RECURSE_FOR_TESTS(it)
