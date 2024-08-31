OWNER(
    samoylovboris
    g:alice_quality
)

UNION()

FILES(
    config.json
)

FROM_SANDBOX(2041038894 OUT base.tsv)

PEERDIR(
    alice/nlu/data/ru/test/granet/quality/player_commands/target
)

END()

RECURSE(
    target
)
