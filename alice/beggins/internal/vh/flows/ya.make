PY3_LIBRARY()

PEERDIR(
    alice/beggins/internal/vh/operations
    alice/beggins/internal/vh/scripts
    alice/beggins/internal/vh/scripts/python
    contrib/python/pyaml
    nirvana/vh3/src
)

PY_SRCS(
    catboost_evaluation.py
    classification.py
    classifier_creation.py
    common.py
    get_toloka_parameters.py
    evaluation.py
    negatives_sampler.py
    make_commit.py
    report.py
    eval_honeypots.py
    scrapper.py
    toloka_classification.py
    toloka_statistics.py
    repeatable_training.py
    yql.py
    week_logs.py
)

END()
