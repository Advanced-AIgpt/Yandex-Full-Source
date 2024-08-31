from alice.uniproxy.library.backends_bio.yabio_storage import get_key


def test_get_key():
    key = get_key(group_id='group-id', dev_manuf='dev-manuf', dev_model='dev-model')
    assert key == b'group-id_dev-model_dev-manuf'


def test_get_key_2():
    key = get_key(group_id=None, dev_manuf='dev-manuf', dev_model='dev-model')
    assert key == b''
