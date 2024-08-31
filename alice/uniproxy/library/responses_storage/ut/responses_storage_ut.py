from alice.uniproxy.library.testing.config_patch import ConfigPatch
from alice.uniproxy.library.responses_storage import ResponsesStorage


def test_responses_storage():
    storage = ResponsesStorage()

    # store
    for reqid in [f'reqid-{num}' for num in range(100)]:
        for hash in [f'hash-{num}' for num in range(100)]:
            storage.store(reqid, hash, f'{reqid}: {hash}')

    # load
    for reqid in [f'reqid-{num}' for num in range(100)]:
        s = storage.load(reqid)
        for hash in [f'hash-{num}' for num in range(100)]:
            assert s.get(hash) == f'{reqid}: {hash}'


def test_responses_storage_ttl_cleaning():
    zero_ttl_config = {
        'responses_storage': {
            'ttl': 0.0,
        },
    }

    with ConfigPatch(zero_ttl_config):
        storage = ResponsesStorage()

        for num in range(100):
            # store new reqid
            for hash in [f'hash-{num}' for num in range(100)]:
                storage.store(f'reqid-{num}', hash, f'{hash}')

            # previous reqid disappeared
            prev_num = num - 1
            assert not storage.load(f'reqid-{prev_num}')
