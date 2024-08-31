PY2_PROGRAM(nlu_tools)

OWNER(g:alice)

PEERDIR(
    alice/vins/core
    alice/vins/tools/vins_tools
    alice/vins/apps/personal_assistant
    contrib/python/click
    alice/vins/apps/crm_bot
)

PY_SRCS(
    __main__.py
    compare_reports.py
    compile_app_model.py
    download_toloka_sets_from_yt.py
    dump_nlu.py
    ner/compile_fst.py
    process_nlu_on_dataset.py
    where_is_this_phrase.py
)

END()
