import time
import uuid as uuid_
import logging

from alice.uniproxy.library.subway.common import UnisystemRegistry, SubwayRegistry


# --------------------------------------------------------------------------------------------------------------------
def test_subway_registry_add_remove_one():
    registry = SubwayRegistry()

    process = [
        '123def',
        '456abc'
    ]

    guids_str = [
        '58cb52e6-41b9-4018-a393-8ddd4f4e6f39',
        'a50164ee-bca0-4534-9834-7010fa67fbbe',
        '557fb649-238e-44cc-b4ca-3e3a55dc61c5',
        'eda45605-6587-4c7e-b76d-0f721d84f09e',
    ]

    guids = list([
        uuid_.UUID(x).bytes for x in guids_str
    ])

    registry.add(process[0], guids[0])
    registry.add(process[0], guids[1])
    registry.add(process[1], guids[2])
    registry.add(process[1], guids[3])

    assert registry.count(guids[0]) == 1
    assert registry.count(guids[1]) == 1
    assert registry.count(guids[2]) == 1
    assert registry.count(guids[3]) == 1

    assert len(list(registry.enumerate(guids[0]))) == 1
    assert list(registry.enumerate(guids[0]))[0] == (True, guids[0], process[0])

    assert len(list(registry.enumerate(guids[1]))) == 1
    assert list(registry.enumerate(guids[1]))[0] == (True, guids[1], process[0])

    assert len(list(registry.enumerate(guids[2]))) == 1
    assert list(registry.enumerate(guids[2]))[0] == (True, guids[2], process[1])

    assert len(list(registry.enumerate(guids[3]))) == 1
    assert list(registry.enumerate(guids[3]))[0] == (True, guids[3], process[1])

    registry.remove(process[0], guids[0])
    assert list(registry.enumerate(guids[0]))[0][0] is False
    assert list(registry.enumerate(guids[1]))[0][0] is True
    assert list(registry.enumerate(guids[2]))[0][0] is True
    assert list(registry.enumerate(guids[3]))[0][0] is True

    registry.remove(process[0], guids[1])
    assert list(registry.enumerate(guids[0]))[0][0] is False
    assert list(registry.enumerate(guids[1]))[0][0] is False
    assert list(registry.enumerate(guids[2]))[0][0] is True
    assert list(registry.enumerate(guids[3]))[0][0] is True

    registry.remove(process[0], guids[2])
    assert list(registry.enumerate(guids[0]))[0][0] is False
    assert list(registry.enumerate(guids[1]))[0][0] is False
    assert list(registry.enumerate(guids[2]))[0][0] is True
    assert list(registry.enumerate(guids[3]))[0][0] is True

    registry.remove(process[1], guids[2])
    assert list(registry.enumerate(guids[0]))[0][0] is False
    assert list(registry.enumerate(guids[1]))[0][0] is False
    assert list(registry.enumerate(guids[2]))[0][0] is False
    assert list(registry.enumerate(guids[3]))[0][0] is True

    registry.remove(process[1], guids[3])
    assert list(registry.enumerate(guids[0]))[0][0] is False
    assert list(registry.enumerate(guids[1]))[0][0] is False
    assert list(registry.enumerate(guids[2]))[0][0] is False
    assert list(registry.enumerate(guids[3]))[0][0] is False


# --------------------------------------------------------------------------------------------------------------------
def test_subway_registry_add_remove_many():
    registry = SubwayRegistry()

    process = '123def'

    guids = list([
        uuid_.UUID(x).bytes for x in [
            '58cb52e6-41b9-4018-a393-8ddd4f4e6f39',
            'a50164ee-bca0-4534-9834-7010fa67fbbe',
        ]
    ])

    registry.add(process, guids[0])
    registry.add(process, guids[0])
    registry.add(process, guids[0])
    registry.add(process, guids[1])
    registry.add(process, guids[1])

    assert registry.count(guids[0]) == 3
    assert registry.count(guids[1]) == 2

    assert len(list(registry.enumerate(guids[0]))) == 1
    assert list(registry.enumerate(guids[0]))[0] == (True, guids[0], process)

    assert len(list(registry.enumerate(guids[1]))) == 1
    assert list(registry.enumerate(guids[1]))[0] == (True, guids[1], process)

    registry.remove(process, guids[0])
    assert registry.count(guids[0]) == 2
    assert list(registry.enumerate(guids[0]))[0] == (True, guids[0], process)

    registry.remove(process, guids[0])
    assert registry.count(guids[0]) == 1
    assert list(registry.enumerate(guids[0]))[0] == (True, guids[0], process)

    registry.remove(process, guids[0])
    assert registry.count(guids[0]) == 0
    assert list(registry.enumerate(guids[0]))[0] == (False, guids[0], None)

    registry.remove(process, guids[1])
    assert registry.count(guids[1]) == 1
    assert list(registry.enumerate(guids[1]))[0] == (True, guids[1], process)

    registry.remove(process, guids[1])
    assert registry.count(guids[1]) == 0
    assert list(registry.enumerate(guids[1]))[0] == (False, guids[1], None)


# --------------------------------------------------------------------------------------------------------------------
def test_subway_registry_enumerate_many():
    registry = SubwayRegistry()

    process = [
        '123def',
        '456abc'
    ]

    guids = list([
        uuid_.UUID(x).bytes for x in [
            '58cb52e6-41b9-4018-a393-8ddd4f4e6f39',
            'a50164ee-bca0-4534-9834-7010fa67fbbe',
            '557fb649-238e-44cc-b4ca-3e3a55dc61c5',
            'eda45605-6587-4c7e-b76d-0f721d84f09e',
        ]
    ])

    registry.add(process[0], guids[0])
    registry.add(process[0], guids[0])
    registry.add(process[0], guids[1])
    registry.add(process[1], guids[0])
    registry.add(process[1], guids[1])
    registry.add(process[1], guids[2])
    registry.add(process[1], guids[3])

    assert registry.count(guids[0]) == 3
    assert registry.count(guids[1]) == 2
    assert registry.count(guids[2]) == 1
    assert registry.count(guids[3]) == 1

    result = list(registry.enumerate_many([guids[0]]))
    assert len(result) == 2
    assert (True, guids[0], process[0]) in result
    assert (True, guids[0], process[1]) in result

    result = list(registry.enumerate_many([guids[0], guids[1]]))
    assert len(result) == 4
    assert (True, guids[0], process[0]) in result
    assert (True, guids[0], process[1]) in result
    assert (True, guids[1], process[0]) in result
    assert (True, guids[1], process[1]) in result

    result = list(registry.enumerate_many([guids[0], guids[1], guids[2]]))
    assert len(result) == 5
    assert (True, guids[0], process[0]) in result
    assert (True, guids[0], process[1]) in result
    assert (True, guids[1], process[0]) in result
    assert (True, guids[1], process[1]) in result
    assert (True, guids[2], process[1]) in result

    result = list(registry.enumerate_many([guids[0], guids[1], guids[2], guids[3]]))
    assert len(result) == 6
    assert (True, guids[0], process[0]) in result
    assert (True, guids[0], process[1]) in result
    assert (True, guids[1], process[0]) in result
    assert (True, guids[1], process[1]) in result
    assert (True, guids[2], process[1]) in result
    assert (True, guids[3], process[1]) in result


# --------------------------------------------------------------------------------------------------------------------
class UnisystemMock(object):
    COUNTER = 0

    def next_index(self):
        index = UnisystemMock.COUNTER
        UnisystemMock.COUNTER += 1
        return index

    def __init__(self, guid=None):
        self.guid = guid if guid else str(uuid_.uuid4())
        self.guid_bytes = uuid_.UUID(self.guid).bytes
        self.session_id = str(uuid_.uuid4())
        self.session_id_bytes = uuid_.UUID(self.session_id).bytes
        self.index = self.next_index()


# --------------------------------------------------------------------------------------------------------------------
def test_unisystem_registry_add_remove_one():
    registry = UnisystemRegistry()

    unisystems = [
        UnisystemMock() for x in range(0, 4)
    ]

    for us in unisystems:
        registry.add(us, us.guid, us.session_id)

    assert registry.count(unisystems[0].guid) == 1
    assert registry.count(unisystems[1].guid) == 1
    assert registry.count(unisystems[2].guid) == 1
    assert registry.count(unisystems[3].guid) == 1

    result = list(registry.enumerate(unisystems[0].guid_bytes))
    assert len(result) == 1
    assert result[0][:-1] == (unisystems[0].guid, unisystems[0].session_id)

    result = list(registry.enumerate(unisystems[1].guid_bytes))
    assert len(result) == 1
    assert result[0][:-1] == (unisystems[1].guid, unisystems[1].session_id)

    result = list(registry.enumerate(unisystems[2].guid_bytes))
    assert len(result) == 1
    assert result[0][:-1] == (unisystems[2].guid, unisystems[2].session_id)

    result = list(registry.enumerate(unisystems[3].guid_bytes))
    assert len(result) == 1
    assert result[0][:-1] == (unisystems[3].guid, unisystems[3].session_id)

    registry.remove(unisystems[0].guid, unisystems[0].session_id)
    assert registry.count(unisystems[0].guid) == 0
    assert registry.count(unisystems[1].guid) == 1
    assert registry.count(unisystems[2].guid) == 1
    assert registry.count(unisystems[3].guid) == 1

    registry.remove(unisystems[1].guid, unisystems[1].session_id)
    assert registry.count(unisystems[0].guid) == 0
    assert registry.count(unisystems[1].guid) == 0
    assert registry.count(unisystems[2].guid) == 1
    assert registry.count(unisystems[3].guid) == 1

    registry.remove(unisystems[2].guid, unisystems[1].session_id)
    assert registry.count(unisystems[0].guid) == 0
    assert registry.count(unisystems[1].guid) == 0
    assert registry.count(unisystems[2].guid) == 1
    assert registry.count(unisystems[3].guid) == 1

    registry.remove(unisystems[2].guid, unisystems[2].session_id)
    assert registry.count(unisystems[0].guid) == 0
    assert registry.count(unisystems[1].guid) == 0
    assert registry.count(unisystems[2].guid) == 0
    assert registry.count(unisystems[3].guid) == 1

    registry.remove(unisystems[3].guid, unisystems[3].session_id)
    assert registry.count(unisystems[0].guid) == 0
    assert registry.count(unisystems[1].guid) == 0
    assert registry.count(unisystems[2].guid) == 0
    assert registry.count(unisystems[3].guid) == 0


# --------------------------------------------------------------------------------------------------------------------
def test_unisystem_registry_add_remove_many():
    registry = UnisystemRegistry()

    guid = '0e51d5fb-57c1-46e0-be95-fd3f47822ec9'
    guid2 = '8786c660-f4f5-4c05-9094-a0132c1b4667'

    unisystems = [
        UnisystemMock(guid) for x in range(0, 4)
    ]
    unisystems.extend([
        UnisystemMock(guid2) for x in range(0, 4)
    ])

    for us in unisystems:
        registry.add(us, us.guid, us.session_id)

    assert registry.count(unisystems[0].guid) == 4
    assert registry.count(unisystems[4].guid) == 4

    result = list(registry.enumerate(unisystems[0].guid_bytes))
    assert len(result) == 4
    result_session_ids = list([x[1] for x in result])

    assert unisystems[0].session_id in result_session_ids
    assert unisystems[1].session_id in result_session_ids
    assert unisystems[2].session_id in result_session_ids
    assert unisystems[3].session_id in result_session_ids

    result = list(registry.enumerate(unisystems[4].guid_bytes))
    assert len(result) == 4
    result_session_ids = list([x[1] for x in result])

    assert unisystems[4].session_id in result_session_ids
    assert unisystems[5].session_id in result_session_ids
    assert unisystems[6].session_id in result_session_ids
    assert unisystems[7].session_id in result_session_ids

    registry.remove(unisystems[2].guid, unisystems[2].session_id)
    registry.remove(unisystems[3].guid, unisystems[3].session_id)
    result = list(registry.enumerate(unisystems[0].guid_bytes))
    assert len(result) == 2
    result_session_ids = list([x[1] for x in result])

    assert unisystems[0].session_id in result_session_ids
    assert unisystems[1].session_id in result_session_ids

    registry.remove(unisystems[0].guid, unisystems[2].session_id)
    registry.remove(unisystems[1].guid, unisystems[3].session_id)
    result = list(registry.enumerate(unisystems[0].guid_bytes))
    assert len(result) == 2
    result_session_ids = list([x[1] for x in result])
    assert unisystems[0].session_id in result_session_ids
    assert unisystems[1].session_id in result_session_ids

    registry.remove(unisystems[0].guid, unisystems[0].session_id)
    registry.remove(unisystems[1].guid, unisystems[1].session_id)
    assert registry.count(unisystems[0].guid) == 0
    result = list(registry.enumerate(unisystems[0].guid_bytes))
    assert len(result) == 0


# --------------------------------------------------------------------------------------------------------------------
def test_unisystem_registry_add_remove_20k():
    registry = UnisystemRegistry()
    unisystems = [
        UnisystemMock() for x in range(0, 20000)
    ]

    add_started_at = time.time()
    for us in unisystems:
        registry.add(us, us.guid, us.session_id)
    add_duration = time.time() - add_started_at
    logging.info('add 20k systems in %.3fms, %.3fus per record', add_duration * 1000, add_duration * 10000)

    guids = list([us.guid_bytes for us in unisystems])
    enum_started_at = time.time()
    result = list(registry.enumerate_many(guids))
    enum_duration = time.time() - enum_started_at
    logging.info('itr 20k records in %.3fms, %.3fus per record', enum_duration * 1000, enum_duration * 10000)
    assert len(result) == 20000

    del_started_at = time.time()
    for us in unisystems:
        registry.remove(us.guid, us.session_id)
    del_duration = time.time() - del_started_at
    logging.info('del 20k systems in %.3fms, %.3fus per record', del_duration * 1000, del_duration * 10000)


# --------------------------------------------------------------------------------------------------------------------
def test_subway_registry_add_remove_20k():
    registry = SubwayRegistry()

    process = '123'
    guids = [
        uuid_.uuid4().bytes for x in range(0, 20000)
    ]

    add_started_at = time.time()
    for guid in guids:
        registry.add(process, guid)
    add_duration = time.time() - add_started_at
    logging.info('add 20k records in %.3fms, %.3fus per record', add_duration * 1000, add_duration * 10000)

    enum_started_at = time.time()
    result = list(registry.enumerate_many(guids))
    enum_duration = time.time() - enum_started_at
    logging.info('itr 20k records in %.3fms, %.3fus per record', enum_duration * 1000, enum_duration * 10000)
    assert len(result) == 20000

    del_started_at = time.time()
    for guid in guids:
        registry.remove(process, guid)
    del_duration = time.time() - del_started_at
    logging.info('del 20k records in %.3fms, %.3fus per record', del_duration * 1000, del_duration * 10000)


def test_subway_registry_enumerate():
    registry = SubwayRegistry()

    process = '123'

    guids = [
        uuid_.uuid4().bytes for x in range(0, 10)
    ]

    for guid in guids:
        registry.add(process, guid)

    ready = []
    for guid, processes in registry.enumerate_clients(False).items():
        assert processes == 1
        assert guid in guids
        assert guid not in ready

        ready.append(guid)
