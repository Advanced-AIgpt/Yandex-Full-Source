LIBRARY()

OWNER(
    ardulat
    g:alice
)

PEERDIR(
    alice/hollywood/library/framework
    alice/hollywood/library/bass_adapter
    alice/hollywood/library/metrics
    alice/hollywood/library/scenarios/tr_navi/general_conversation/nlg
    alice/megamind/protos/common
    alice/megamind/protos/scenarios
)

SRCS(
    common.cpp
    prepare_handle.cpp
    render_handle.cpp
    GLOBAL general_conversation.cpp
)

END()

RECURSE_FOR_TESTS(it)
