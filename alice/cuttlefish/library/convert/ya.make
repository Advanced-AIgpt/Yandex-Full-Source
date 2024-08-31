LIBRARY()
OWNER(g:voicetech-infra)

SRCS(
    private/json_converts.h
    private/process_tree.h
    converter.h
    builder.h
    methods.h
    rapid_node.h
    json_value_writer.h
)

PEERDIR(
    library/cpp/json
)

END()

RECURSE_FOR_TESTS(ut)
