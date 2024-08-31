OWNER(
    samoylovboris
    g:alice_quality
    amullanurov
)

UNION()

FILES(
    previous_track.ignore.tsv
)

FROM_SANDBOX(2190310477 OUT previous_track.neg.tsv)
FROM_SANDBOX(2190309854 OUT previous_track.pos.tsv)

END()
