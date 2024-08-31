OWNER(
    samoylovboris
    g:alice_quality
    vitvlkv
)

UNION()

FILES(
    next_track.ignore.tsv
)

FROM_SANDBOX(2180976893 OUT next_track.neg.tsv)
FROM_SANDBOX(2180977819 OUT next_track.pos.tsv)

END()
