OWNER(
    samoylovboris
    g:alice_quality
    amullanurov
)

UNION()

FILES(
    rewind.ignore.tsv
)

FROM_SANDBOX(2195186903 OUT rewind.neg.tsv)
FROM_SANDBOX(2195190433 OUT rewind.pos.tsv)

END()
