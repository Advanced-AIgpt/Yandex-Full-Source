PY2_LIBRARY()

OWNER(g:alice)

PEERDIR(
    alice/nlu/py_libs/utils
    bindings/python/inflector_lib
    bindings/python/lemmer_lib
    contrib/python/protobuf
    contrib/python/attrs
    contrib/python/fasteners
    contrib/python/jsonschema
    contrib/python/numpy
    contrib/python/pytz
    contrib/python/PyYAML
    contrib/python/transliterate
    contrib/python/ujson
    library/python/yenv
    library/python/func
    library/python/ylog
)

PY_SRCS(
    NAMESPACE vins_core.utils
    __init__.py
    archives.py
    config.py
    data.py
    datetime.py
    decorators.py
    intent_renamer.py
    iter.py
    json_util.py
    lemmer.py
    logging.py
    metrics.py
    misc.py
    mixins.py
    multiprocess_queue.py
    network.py
    operators.py
    sampling.py
    sequence_alignment.py
    serialize.py
    strings.py
    updater.py
)

END()

RECURSE_FOR_TESTS(ut)
