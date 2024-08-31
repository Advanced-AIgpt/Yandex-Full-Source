LIBRARY()

OWNER(alexanderplat g:alice)

SRCS(
    create_nlg_renderer_from_nlg_library_path.cpp
    create_nlg_renderer_from_register_function.cpp
    fwd.cpp
    nlg_renderer.cpp
)

PEERDIR(
    alice/library/util
    alice/nlg/library/runtime_api
    alice/nlg/library/voice_prefix
    library/cpp/json
    library/cpp/langs
)

END()

RECURSE_FOR_TESTS(
    ut
)
