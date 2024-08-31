UNION()

OWNER(
    akhruslan
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TSSSSFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/sssss/sssss.pb.txt
    sssss.pb
    IN alice/hollywood/shards/common/prod/fast_data/sssss/sssss.pb.txt
    OUT sssss.pb
)

END()
