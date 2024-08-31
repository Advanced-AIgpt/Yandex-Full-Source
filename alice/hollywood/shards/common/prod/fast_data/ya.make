UNION()

OWNER(
    akhruslan
    g:hollywood
)

FILES(.svninfo)

PEERDIR(
    alice/hollywood/shards/common/prod/fast_data/alice_show
    alice/hollywood/shards/common/prod/fast_data/blueprints
    alice/hollywood/shards/common/prod/fast_data/hardcoded_response
    alice/hollywood/shards/common/prod/fast_data/how_to_spell
    alice/hollywood/shards/common/prod/fast_data/market
    alice/hollywood/shards/common/prod/fast_data/music
    alice/hollywood/shards/common/prod/fast_data/news
    alice/hollywood/shards/common/prod/fast_data/notifications
    alice/hollywood/shards/common/prod/fast_data/random_number
    alice/hollywood/shards/common/prod/fast_data/sssss
)

END()

RECURSE(
    alice_show
    hardcoded_response
    how_to_spell
    market
    music
    news
    notifications
    random_number
    sssss
    util
)
