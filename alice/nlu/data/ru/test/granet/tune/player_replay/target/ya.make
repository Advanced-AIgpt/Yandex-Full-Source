OWNER(
    samoylovboris
    g:alice_quality
)

UNION()

FILES(
    replay.ignore.tsv
)

FROM_SANDBOX(2164516244 OUT replay.neg.tsv)
FROM_SANDBOX(2164515918 OUT replay.pos.tsv)

END()
