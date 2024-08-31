UNION()

OWNER(
    akastornov
)

FROM_SANDBOX(2424417418 OUT base.tsv)

FILES(
    config.json
)

PEERDIR(
    alice/nlu/data/ru/test/granet/quality/messenger_call/target
)

END()

RECURSE(
    target
)
