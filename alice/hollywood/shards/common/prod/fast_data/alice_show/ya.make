UNION()

OWNER(
    lavv17
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TAliceShowFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/alice_show/config/alice_show.pb.txt
    alice_show.pb
    IN alice/hollywood/shards/common/prod/fast_data/alice_show/config/alice_show.pb.txt
    OUT alice_show.pb
)

PEERDIR(
    alice/hollywood/shards/common/prod/fast_data/alice_show/config
)

END()

RECURSE_FOR_TESTS(
    ../../../../common/prod/fast_data/util/alice_show/ut
)
