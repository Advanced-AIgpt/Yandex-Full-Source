LIBRARY()

OWNER(alexanderplat g:alice)

SRCS(
    builtins.cpp
    coverage.cpp
    emoji.cpp
    helpers.cpp
    inflector.cpp
    operators.cpp
    runtime.cpp
)

PEERDIR(
    alice/nlg/library/runtime_api

    kernel/inflectorlib/fio/simple
    kernel/inflectorlib/phrase/simple
    kernel/inflectorlib/pluralize

    library/cpp/containers/stack_vector
    library/cpp/json
    library/cpp/langs
    library/cpp/string_utils/quote
    library/cpp/timezone_conversion

    contrib/libs/cctz
    contrib/libs/double-conversion
    contrib/libs/re2
)

END()

RECURSE_FOR_TESTS(
    ut
)
