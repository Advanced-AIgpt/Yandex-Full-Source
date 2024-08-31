PY2_PROGRAM(train_tools)

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/apps/personal_assistant
    contrib/python/click
)

PY_SRCS(
    __main__.py
    get_toloka_markup_for_tagger.py
    nlu_to_toloka_markup.py
    toloka_markup.py
    toloka_markup_to_nlu.py
    video_markup_fix.py
)

END()
