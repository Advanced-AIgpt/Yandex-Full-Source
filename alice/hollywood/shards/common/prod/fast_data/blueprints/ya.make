UNION()

OWNER(
    d-dima
    g:hollywood
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TBlueprintsFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/blueprints/blueprints.pb.txt
    blueprints.pb
    IN alice/hollywood/shards/common/prod/fast_data/blueprints/blueprints.pb.txt
    OUT blueprints.pb
)

END()
