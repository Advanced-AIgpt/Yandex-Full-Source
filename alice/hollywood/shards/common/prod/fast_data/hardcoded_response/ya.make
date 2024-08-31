UNION()

OWNER(
    akhruslan
    olegator
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto THardcodedResponseFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/hardcoded_response/hardcoded_response.pb.txt
    hardcoded_response.pb
    IN alice/hollywood/shards/common/prod/fast_data/hardcoded_response/hardcoded_response.pb.txt
    OUT hardcoded_response.pb
)

END()
