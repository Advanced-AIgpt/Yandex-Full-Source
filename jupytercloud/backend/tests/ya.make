OWNER(g:jupyter-cloud)

RECURSE(
    mock
)

RECURSE_FOR_TESTS(
    auth
    # idm  # doesn't work yet
    # mypy  # doesn't work either
)
