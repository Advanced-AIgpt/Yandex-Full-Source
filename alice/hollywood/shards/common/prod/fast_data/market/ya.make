UNION()

OWNER(
    artemkoff
    g:marketinalice
)

RUN_PROGRAM(
    alice/hollywood/convert_proto
    --proto TMarketFastData
    --to-binary
    alice/hollywood/shards/common/prod/fast_data/market/market.pb.txt
    market.pb
    IN alice/hollywood/shards/common/prod/fast_data/market/market.pb.txt
    OUT market.pb
)

END()
