from alice.uniproxy.library.utils.dict_object import DictObj


def test_existing_value():
    a = DictObj({'foo': 1, 'bar': 'bar'})

    assert a.foo == 1
    assert a.bar == 'bar'


def test_missing_value():
    a = DictObj({'foo': 1, 'bar': 'bar'})

    assert a.baz is None


def test_dict_value():
    a = DictObj({'foo': 1, 'bar': 'bar', 'baz': {'foo': 42}})

    assert isinstance(a.baz, DictObj)
    assert a.baz.foo == 42
