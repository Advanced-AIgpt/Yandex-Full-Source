UNION()

LARGE_FILES(model.tar)

FROM_ARCHIVE(
    model.tar
    OUT_NOAUTO model.pb
    OUT_NOAUTO start.trie
    OUT_NOAUTO cont.trie
    OUT_NOAUTO vocab.txt
)

END()
