PY3_LIBRARY()

OWNER(
    andreyshspb
)

PEERDIR(
    yt/python/client
    contrib/python/pandas
    contrib/python/plotly
    contrib/python/beautifulsoup4
    contrib/python/scikit-learn
    contrib/python/requests
)

PY_SRCS(
    collect_match_stats.py
    commit_draft.py
    json_conversion.py
    toloka_request_script.py
    find_thresholds_script.py
    eval_metrics_script.py
    extract_threshold_script.py
    make_report_script.py
    modification_for_commit.py
    validate_honeypots_evaluation.py
    wait_fast_data.py
)

END()

RECURSE_FOR_TESTS(
    tests
)
