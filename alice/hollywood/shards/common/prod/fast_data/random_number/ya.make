UNION()

OWNER(
    d-dima
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TThrowDiceFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/random_number/random_number.pb.txt
    random_number.pb
    IN alice/hollywood/shards/common/prod/fast_data/random_number/random_number.pb.txt
    OUT random_number.pb
)

END()
