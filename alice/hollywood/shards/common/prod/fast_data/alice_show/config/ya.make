UNION()

OWNER(
    lavv17
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/shards/common/prod/fast_data/alice_show/config/builder
    conditions.pb.txt config.pb.txt images.pb.txt phrases.pb.txt
    --out alice_show.pb.txt
    IN conditions.pb.txt config.pb.txt images.pb.txt phrases.pb.txt
    OUT alice_show.pb.txt
)

PEERDIR(
    alice/hollywood/shards/common/prod/fast_data/alice_show/config/builder
)

END()

RECURSE_FOR_TESTS(
    ../../../../../../shards/common/prod/fast_data/util/alice_show/ut
)
