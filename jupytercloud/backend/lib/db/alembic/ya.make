PY3_LIBRARY()

OWNER(g:jupyter-cloud)

PEERDIR(
    contrib/python/sqlalchemy/sqlalchemy-1.3
    contrib/python/alembic
)

PY_SRCS(
    main.py
    env.py
    versions/eab2ab022148_added_users_table.py
    versions/f96a31a74bff_add_pillars_table.py
    versions/e93183fcc25e_add_user_settings_field.py
    versions/06c0ba0d6ded_add_jupyticket_and_related_tables.py
)

NO_CHECK_IMPORTS(
    jupytercloud.backend.lib.db.alembic.env  # env imports dynamically, so import test goes mad
)

END()
