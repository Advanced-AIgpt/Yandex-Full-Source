import json

import jsonschema
import pytest  # noqa
import yatest.common

from jupytercloud.backend.lib.idm import ROLE_TREE


def test_schema():
    schema_path = yatest.common.source_path('jupytercloud/backend/lib/idm/tests/role_tree.schema.json')

    with open(schema_path) as s:
        schema = json.load(s)

    jsonschema.validate(schema=schema, instance=ROLE_TREE)
