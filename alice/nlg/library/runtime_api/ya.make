LIBRARY()

OWNER(alexanderplat g:alice)

SRCS(
    caller.cpp
    call_stack.cpp
    exceptions.cpp
    env.cpp
    globals.cpp
    nlg_library_registry.cpp
    postprocess.cpp
    range.cpp
    text.cpp
    text_stream.cpp
    translations.cpp
    value.cpp
)

PEERDIR(
    alice/library/util
    library/cpp/json
    library/cpp/langs
    contrib/libs/re2
)

END()

RECURSE_FOR_TESTS(
    ut
)
