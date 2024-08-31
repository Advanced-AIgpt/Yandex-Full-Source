from alice.json_schema_builder.library import nodes, util


class Foo:
    def to_json(self):
        return 123

    def __eq__(self, other):
        return isinstance(other, Foo)

    def __hash__(self):
        return 0


def test_filter_nested():
    def _even_odd(even):
        def _func(obj, key):
            value = obj[key]
            if isinstance(value, int):
                return (value % 2 == 0) == even
            return False
        return _func

    xs = {'asdf': 4, 'jkl': [5, [6]]}
    assert {'asdf': 4, 'jkl': [[6]]} == util.filter_nested(xs, _even_odd(False))
    assert {'jkl': [5, []]} == util.filter_nested(xs, _even_odd(True))
    assert {'jkl': [5]} == util.filter_nested(
        xs,
        lambda obj, key: _even_odd(True)(obj, key) or isinstance(obj, list) and obj[key] is xs['jkl'][1]
    )
    assert {} == util.filter_nested(xs, lambda obj, key: True)


def test_recursive_merge():
    one = {
        'foo': [0],
        'bar': {
            'bar_prime': None,
        },
    }

    two = {
        'foo': [],
        'bar': {
            'bar_prime': 1,
        },
    }

    three = {
        'bar': {
            'bar_prime_prime': 2,
        },
    }

    expected = {
        'foo': [0],
        'bar': {
            'bar_prime': 1,
            'bar_prime_prime': 2,
        },
    }

    assert expected == util.recursive_merge([one, two, three])


def test_jsonize_objects():
    assert {123: [123, 123]} == util.jsonize_objects({Foo(): [Foo(), Foo()]})


def test_iter_json_objects():
    filename = 'test.json'
    contents = [
        {
            'foo': {
                'bazz': 123,
            },
            'bar': [],
        },
        123,
        {
            'baz': {
                'awol': {},
            }
        },
    ]

    expected = [
        (nodes.Ref(filename=filename, path=(0,)), contents[0]),
        (nodes.Ref(filename=filename, path=(0, 'foo')), contents[0]['foo']),
        (nodes.Ref(filename=filename, path=(2,)), contents[2]),
        (nodes.Ref(filename=filename, path=(2, 'baz')), contents[2]['baz']),
    ]

    actual = [
        (location_factory(), obj)
        for location_factory, obj in util.iter_json_objects(
            contents,
            filename=filename,
            stop=lambda value: 'awol' in value,
        )
    ]

    assert expected == actual
