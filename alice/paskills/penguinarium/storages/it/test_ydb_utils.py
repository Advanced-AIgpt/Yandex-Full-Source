import pytest
from unittest.mock import patch
import os

from kikimr.public.sdk.python import tvm
import ydb

from alice.paskills.penguinarium.storages.ydb_utils import retriable_perform


def test_ydb_manager(ydb_manager, endpoint, database):
    assert ydb_manager.timeout is None  # as defaults

    with patch('tvmauth.tvmauth.TvmClient'):
        tvm_creds = ydb_manager._create_tvm_credentials('tvm_secret', 1, 2)
        assert isinstance(tvm_creds, tvm.TvmCredentialsProvider)

        tvm_secret_env = 'TVM_SECRET'
        os.environ[tvm_secret_env] = 'tvm_secret'
        tvm_creds = ydb_manager._create_credentials({
            'use_tvm': True,
            'tvm_secret_env': tvm_secret_env,
            'self_tvm_id': 1
        })
        assert isinstance(tvm_creds, tvm.TvmCredentialsProvider)

    with pytest.raises(RuntimeError):
        ydb_manager._setup_ydb_driver(
            endpoint=endpoint,
            database='not_a_database',
            credentials=ydb.AuthTokenCredentials('')
        )


@pytest.mark.asyncio
async def test_retriable_call(ydb_manager):
    async def callee(session, transaction):
        callee.counter += 1
        raise ydb.issues.BadSession('')

    callee.counter = 0
    with pytest.raises(ydb.issues.BadSession):
        await retriable_perform(ydb_manager, callee)

    assert callee.counter == ydb_manager.max_retries + 1
