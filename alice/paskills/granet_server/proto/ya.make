PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:paskills)

SRCS(
    compile_grammar_response.proto
    compile_grammar_request.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()