OWNER(g:alice_analytics)

PY23_LIBRARY()

PEERDIR(
    library/python/resource
)

RESOURCE(
    data/basket_configs.json basket_configs.json
)

PY_SRCS(
    NAMESPACE alice.analytics.tasks.va_571
    intents_to_human_readable.py
    standardize_answer.py
    basket_configs.py
    slices_mapping.py
    handcrafted_responses.py
)

NO_CHECK_IMPORTS()

END()
