LIBRARY()

OWNER(
    d-dima
)

PEERDIR(
    alice/protos/extensions
    alice/megamind/protos/common
    contrib/libs/protoc
    contrib/libs/jinja2cpp
)

SRCS(
    main.cpp
    jinja2_compiler.cpp
    jinja2_protoc.cpp
    jinja2_proto2json.cpp
    jinja2_render.cpp
)

END()

RECURSE_FOR_TESTS(
    ut
)

