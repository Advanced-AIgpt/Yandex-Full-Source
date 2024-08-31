from alice.uniproxy.library.utils.deepupdate import deepupdate


def test_deepupdate_copy():
    first = {
        'foo': 1,
        'bar': {
            'baz': 2
        }
    }

    second = {
        'bar': {
            'foo': 3
        }
    }

    x = deepupdate(second, first)

    assert 'foo' in x
    assert 'bar' in x
    assert 'baz' in x['bar']
    assert 'foo' in x['bar']

    second['bar']['foo'] = 42
    assert x['bar']['foo'] == 3

    second['bar']['baz'] = 42
    assert x['bar']['baz'] == 2


def test_deepupdate():
    first = {
        'foo': 1,
        'bar': {
            'baz': 2
        }
    }

    second = {
        'bar': {
            'foo': 3
        }
    }

    x = deepupdate(second, first, False)

    assert 'foo' in x
    assert 'bar' in x
    assert 'baz' in x['bar']
    assert 'foo' in x['bar']

    assert 'foo' in second
    assert 'bar' in second
    assert 'baz' in second['bar']
    assert 'foo' in second['bar']

    second['bar']['foo'] = 42
    assert x['bar']['foo'] == 42

    second['bar']['baz'] = 42
    assert x['bar']['baz'] == 42
