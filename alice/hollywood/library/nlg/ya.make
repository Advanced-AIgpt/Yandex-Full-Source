LIBRARY()

OWNER(g:hollywood)

PEERDIR(
    alice/hollywood/library/request
    alice/library/json
    alice/library/logger
    alice/library/util
    alice/nlg/library/nlg_renderer
    library/cpp/langs
)

SRCS(
    nlg.cpp
    nlg_data.cpp
    nlg_render_history.cpp
    nlg_wrapper.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)
