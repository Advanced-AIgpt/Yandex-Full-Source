import importlib.resources
import json

import jsonschema
import pytest

from jupytercloud.backend.lib.qyp.instance import DEFAULT_INSTANCES
from jupytercloud.backend.lib.qyp.quota import Accounts
from jupytercloud.backend.services.idm.handlers import (
    ADMIN_ROLE_PATH, ADMIN_ROLE_SPEC, QUOTA_ROLE_PREFIX,
)


@pytest.fixture
def idm_schema():
    with importlib.resources.path('jupytercloud.idm', 'role_tree.schema.json') as path:
        with path.open() as file_:
            return json.load(file_)


def check_no_jupyterhub_users(jupyterhub_app):
    assert len(jupyterhub_app.users) == 1 and jupyterhub_app.users['admin']


def check_jupyterhub_users(jupyterhub_app, users, admin=False):
    assert len(jupyterhub_app.users) == len(users) + 1
    for username in users:
        assert jupyterhub_app.users[username]

    if admin is not None:
        for username in users:
            assert jupyterhub_app.users[username].admin == admin


def check_empty_idm_state(db, username):
    assert (
        db.get_user_idm_state(username) ==
        db.get_patched_user_idm_state(username) ==
        {}
    )


def check_idm_instance_state(idm_state, types):
    expected_state = {}
    excpected_instances = []

    for type_ in types:
        instance_name = f'cpu1_ram4_{type_}24'
        instance = DEFAULT_INSTANCES[instance_name]
        role_path = f'{QUOTA_ROLE_PREFIX}vm/{instance_name}/'
        role_spec = {
            'fields': None,
            'role': {'role': 'quota', 'vm': instance_name},
        }

        expected_state[role_path] = role_spec
        excpected_instances.append(instance)

    assert idm_state == expected_state
    assert Accounts._get_default_available_instances(idm_state) == excpected_instances


@pytest.mark.gen_test
async def test_idm_app_ok(idm_app):
    assert await idm_app.request() == b'OK'


@pytest.mark.gen_test
async def test_idm_role_tree(idm_app, idm_schema):
    raw_data = await idm_app.request('info')
    data = json.loads(raw_data)
    assert data.pop('code') == 0
    role_tree = data.pop('roles')
    assert not data

    jsonschema.validate(role_tree, idm_schema)

    top_roles = role_tree['values']

    assert set(top_roles) == {'admin', 'quota'}

    quota_roles = top_roles['quota']['roles']['values']

    assert set(quota_roles) == {
        # NOTE: role with "hdd" in name actually gives access to
        # similar "ssd" instance
        name for name in DEFAULT_INSTANCES if 'ssd' not in name
    }


@pytest.mark.gen_test
async def test_idm_add_admin(idm_app, jupyterhub_app, jupytercloud_db):
    new_user = idm_app.generate_username()

    check_no_jupyterhub_users(jupyterhub_app)
    check_empty_idm_state(jupytercloud_db, new_user)

    request_kwargs = dict(
        method='post',
        data={
            'login': new_user,
            'path': ADMIN_ROLE_PATH,
            'role': json.dumps(ADMIN_ROLE_SPEC),
        },
    )

    result = await idm_app.request('add-role', **request_kwargs)
    assert json.loads(result) == {'code': 0}

    # user added to jupyterhub DB
    check_jupyterhub_users(jupyterhub_app, [new_user], admin=True)

    # user added to jupytercloud DB
    assert (
        jupytercloud_db.get_user_idm_state(new_user) ==
        jupytercloud_db.get_patched_user_idm_state(new_user) ==
        {ADMIN_ROLE_PATH: {'role': ADMIN_ROLE_SPEC, 'fields': None}}
    )

    # second request does nothing
    result = await idm_app.request('add-role', **request_kwargs)
    assert json.loads(result) == {
        'code': 0,
        'warning': f'User {new_user} already have role {ADMIN_ROLE_PATH}',
    }
    check_jupyterhub_users(jupyterhub_app, [new_user], admin=True)

    # removing of admin role
    result = await idm_app.request('remove-role', **request_kwargs)
    assert json.loads(result) == {'code': 0}
    check_jupyterhub_users(jupyterhub_app, [new_user], admin=False)
    check_empty_idm_state(jupytercloud_db, new_user)


@pytest.mark.gen_test
async def test_idm_add_instance(idm_app, jupyterhub_app, jupytercloud_db):
    new_user = idm_app.generate_username()
    instance_name = 'cpu1_ram4_hdd24'
    role_path = f'{QUOTA_ROLE_PREFIX}vm/{instance_name}/'
    role_spec = {'role': 'quota', 'vm': instance_name}

    check_no_jupyterhub_users(jupyterhub_app)
    check_empty_idm_state(jupytercloud_db, new_user)
    check_idm_instance_state(jupytercloud_db.get_user_idm_state(new_user), [])

    request_kwargs = dict(
        method='post',
        data={
            'login': new_user,
            'path': role_path,
            'role': json.dumps(role_spec),
        },
    )

    result = await idm_app.request('add-role', **request_kwargs)
    assert json.loads(result) == {'code': 0}

    check_jupyterhub_users(jupyterhub_app, [new_user], admin=False)
    check_idm_instance_state(jupytercloud_db.get_user_idm_state(new_user), ['hdd'])
    check_idm_instance_state(jupytercloud_db.get_patched_user_idm_state(new_user), ['hdd', 'ssd'])

    # second request does nothing
    result = await idm_app.request('add-role', **request_kwargs)
    assert json.loads(result) == {
        'code': 0,
        'warning': f'User {new_user} already have role {role_path}',
    }
    check_jupyterhub_users(jupyterhub_app, [new_user], admin=False)

    # removing of VM role
    result = await idm_app.request('remove-role', **request_kwargs)
    assert json.loads(result) == {'code': 0}
    check_jupyterhub_users(jupyterhub_app, [new_user], admin=False)
    check_empty_idm_state(jupytercloud_db, new_user)
    check_idm_instance_state(jupytercloud_db.get_user_idm_state(new_user), [])
