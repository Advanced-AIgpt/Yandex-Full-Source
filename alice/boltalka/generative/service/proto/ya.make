PROTO_LIBRARY()
SET(PROTOC_TRANSITIVE_HEADERS "no")

OWNER(g:alice_boltalka)

SRCS(
    bert_request.proto
    bert_response.proto
    common.proto
    embedding_request.proto
    embedding_response.proto
    generative_request.proto
    generative_response.proto
    phead.proto
    scoring_request.proto
    scoring_response.proto
)

EXCLUDE_TAGS(GO_PROTO)

END()
