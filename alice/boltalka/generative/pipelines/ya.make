PY3_LIBRARY()

OWNER(
    artemkorenev
    g:alice_boltalka
)

PEERDIR(
    dict/mt/make/db
    dict/mt/make/libs/common
    dict/mt/make/libs/dumper
    dict/mt/make/modules/tfnn
    dict/mt/make/pipeline/nmt
    nirvana/valhalla/src

    alice/boltalka/generative/tfnn/bucket_maker/vh

    alice/boltalka/generative/training/data/nn/util

    bindings/python/dssm_nn_applier_lib/lib
)

PY_SRCS(
    __init__.py
    calc_informativeness.py
    make_buckets.py
    measure_on_toloka.py
    util.py

    analysis/__init__.py
    analysis/analyze_buckets.py

    factor_dssm/__init__.py
    factor_dssm/calc_factor_dssm.py

    staging/converting/__init__.py
    staging/converting/convert.py
)

END()
