OWNER(
    dan-anastasev
    igor-darov
    g:alice_quality
)

PY2TEST()

TEST_SRCS(
    test.py
)

DATA(
    sbr://3368514643  # quality_data.tsv
    sbr://3368513416  # medium_data.tsv
)

DEPENDS(
    alice/nlu/libs/rnn_tagger/quality_test/canonize_applier
    alice/nlu/libs/rnn_tagger/quality_test/canonized_data_diff
    search/wizard/data/wizard/AliceTagger
    search/wizard/data/wizard/AliceTokenEmbedder
    search/wizard/data/wizard/CustomEntities
)

SIZE(MEDIUM)

END()

RECURSE(
    canonize_applier
    canonized_data_diff
    mocks_downloader
)
