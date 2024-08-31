UNION()

OWNER(
    khr2
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TNewsFastDataProto
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/news/news.pb.txt
    news.pb
    IN alice/hollywood/shards/common/prod/fast_data/news/news.pb.txt
    OUT news.pb
)

END()
