import pytest
from uuid import uuid1 as gen_uuid
from alice.uniproxy.library.vins_context_storage import get_instance
from tests.basic import async_test
from alice.uniproxy.library.global_counter import GlobalCounter
from alice.uniproxy.library.global_counter import GlobalTimings
from alice.uniproxy.library.logging import Logger


def get_counter(name):
    for k, v in GlobalCounter.get_metrics():
        if k == name:
            return v
    raise RuntimeError('unknown counter [{0}]'.format(name))


@pytest.fixture(scope="session", autouse=True)
def session_setup():
    Logger.init('unit_test_vins_context_storage', True)


@pytest.fixture(scope="function", autouse=True)
def method_setup():
    GlobalCounter.init()
    GlobalTimings.init()


async def create_accessor(uuid, dialog_id, request_id, prev_req_id):
    instance = await get_instance()
    return instance.create_accessor(gen_uuid(),
                                    {
                                        'application': {
                                            'uuid': uuid
                                        },
                                        'header': {
                                            'dialog_id': dialog_id,
                                            'request_id': request_id,
                                            'prev_req_id': prev_req_id
                                        }
                                    })


last_request_id = None


async def create_next_accessor(uuid, dialog_id):
    global last_request_id

    request_id = str(gen_uuid())
    prev_req_id = last_request_id
    last_request_id = request_id
    return await create_accessor(uuid, dialog_id, request_id, prev_req_id)


@async_test
async def test_simple():
    user_id = str(gen_uuid()) + '__uuid'
    dialog_id = str(gen_uuid()) + '__dialog_id'

    accessor = await create_next_accessor(user_id, dialog_id)
    session_data = await accessor.load()
    assert session_data is None

    new_data = str(gen_uuid()).encode('ascii') + b'__new_data'
    new_dialog_id = str(gen_uuid()) + '__new_dialog_id'
    new_dialog_data = str(gen_uuid()).encode('ascii') + b'__new_dialog_data'
    await accessor.save(dict([(dialog_id, new_data), (new_dialog_id, new_dialog_data)]))

    accessor = await create_accessor(user_id, dialog_id, str(gen_uuid()), last_request_id)
    assert(await accessor.load() == new_data)

    accessor = await create_accessor(user_id, None, str(gen_uuid()), last_request_id)
    assert (await accessor.load() is None)

    accessor = await create_accessor(user_id, new_dialog_id, str(gen_uuid()), last_request_id)
    assert (await accessor.load() == new_dialog_data)


@async_test
async def test_can_save_empty_session():
    user_id = str(gen_uuid()) + '__uuid'

    accessor = await create_next_accessor(user_id, None)
    await accessor.load()
    session_data = str(gen_uuid()).encode('ascii') + b'__session_data'
    await accessor.save(dict([('', session_data)]))

    accessor = await create_next_accessor(user_id, None)
    assert await accessor.load() == session_data
    await accessor.save_base64(dict([('', '')]))

    accessor = await create_next_accessor(user_id, None)
    assert not await accessor.load()


@async_test
async def test_counters():
    user_id = str(gen_uuid()) + '__uuid'
    dialog_id = str(gen_uuid()) + '__dialog_id'

    accessor = await create_next_accessor(user_id, dialog_id)
    session_data = await accessor.load()
    assert session_data is None

    new_data = str(gen_uuid()).encode('ascii') + b'__new_data'
    new_dialog_id = str(gen_uuid()) + '__new_dialog_id'
    new_dialog_data = str(gen_uuid()).encode('ascii') + b'__new_dialog_data'
    await accessor.save(dict([(dialog_id, new_data), (new_dialog_id, new_dialog_data)]))

    assert get_counter('vins_context_load_requests_summ') >= 1
    assert get_counter('vins_context_save_requests_summ') >= 1
    assert get_counter('vins_context_load_failed_requests_summ') == 0


@async_test
async def test_concurrent_create():
    user_id = str(gen_uuid()) + '__uuid'

    request_id1 = str(gen_uuid())
    accessor1 = await create_accessor(user_id, None, request_id1, None)
    assert await accessor1.load() is None
    request_id2 = str(gen_uuid())
    accessor2 = await create_accessor(user_id, None, request_id2, None)
    assert await accessor2.load() is None

    await accessor2.save(dict([('', b'accessor2-new-data'), ('test-dialog2', b'accessor2-dialog-data')]))
    await accessor1.save(dict([('', b'accessor1-new-data'), ('test-dialog1', b'accessor1-dialog-data')]))

    accessor = await create_accessor(user_id, None, str(gen_uuid()), request_id1)
    assert await accessor.load() == b'accessor1-new-data'
    accessor = await create_accessor(user_id, 'test-dialog1', str(gen_uuid()), request_id1)
    assert await accessor.load() == b'accessor1-dialog-data'
    accessor = await create_accessor(user_id, 'test-dialog2', str(gen_uuid()), request_id2)
    assert await accessor.load() == b'accessor2-dialog-data'
    accessor = await create_accessor(user_id, 'test-dialog2', str(gen_uuid()), request_id1)
    assert await accessor.load() is None


@async_test
async def test_concurrent_update():
    user_id = str(gen_uuid()) + '__uuid'

    request_id = str(gen_uuid())
    accessor = await create_accessor(user_id, None, request_id, None)
    assert await accessor.load() is None
    await accessor.save(dict([('', b'test-data')]))

    request_id2 = str(gen_uuid())
    accessor1 = await create_accessor(user_id, None, request_id2, request_id)
    assert await accessor1.load() is not None
    request_id3 = str(gen_uuid())
    accessor2 = await create_accessor(user_id, None, request_id3, request_id)
    assert await accessor2.load() is not None

    await accessor2.save(dict([('', b'new-test-data1')]))
    await accessor1.save(dict([('', b'new-test-data2')]))

    request_id4 = str(gen_uuid())
    accessor = await create_accessor(user_id, None, request_id4, request_id3)
    assert await accessor.load() == b'new-test-data1'

    request_id5 = str(gen_uuid())
    accessor = await create_accessor(user_id, None, request_id5, request_id2)
    assert await accessor.load() == b'new-test-data2'
