OWNER(
    moath-alali
    g:alice_quality
)

PY3_LIBRARY()

PEERDIR(
    contrib/python/pyaml
    contrib/python/pynini
)

PY_SRCS(
    base_fst.py
    date_fst.py
    datetime_fst.py
    datetime_range_fst.py
    float_fst.py
    fsts.py
    json_keys.py
    number_fst.py
    selection_fst.py
    time_fst.py
    utils.py
    weekdays_fst.py
)

END()
