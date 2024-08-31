PY3_LIBRARY()

OWNER(nstbezz)

PY_SRCS(
    pos_aware_levenshtein.pyx
    utils.py
    werp.py
)

FROM_SANDBOX(2051897577 OUT_NOAUTO word_cache.pkl)

RESOURCE(
    alice/analytics/wer/lib/phonemes_mapping.txt /phonemes_mapping.txt
    alice/analytics/wer/lib/stop_words.txt /stop.txt
    alice/analytics/wer/lib/sense_words.txt /sense.txt
    alice/analytics/wer/lib/word_cache.pkl /word_cache.pkl
)

PEERDIR(
    voicetech/asr/tools/g2p/cython
    contrib/python/numpy
    contrib/python/pymorphy2
    contrib/python/transliterate
    contrib/python/weighted-levenshtein
    library/python/resource
)

END()
