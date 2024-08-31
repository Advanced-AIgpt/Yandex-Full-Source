import pytest

from alice.json_schema_builder.library import errors
from alice.json_schema_builder.library.nodes import Node


class Foo(Node):
    attrs = ('foo', 'bar')
    extras = ('awol',)  # should not be taken into account


def test_init():
    obj = Foo(foo=1, bar=2, location=4)

    assert 1 == obj.foo
    assert 2 == obj.bar
    assert 4 == obj.location


def test_init_negative():
    with pytest.raises(errors.NodeInitializationError):
        Foo(foo=1, bar=2, awol=3, location=4)

    with pytest.raises(errors.NodeInitializationError):
        Foo(foo=1, bar=2, awol_prime=3, location=4)


def test_repr():
    obj = Foo(foo=1, bar=2, location=4)
    obj.awol = 3

    assert 'Foo(foo=1, bar=2, location=4)' == repr(obj)
    assert 1 == obj.foo
    assert 2 == obj.bar
    assert 4 == obj.location


def test_to_json():
    foo = Foo(foo=1, bar=2, location=4)
    foo.awol = 3

    assert {':type': 'Foo', 'foo': 1, 'bar': 2, 'location': 4} == dict(foo.to_json())


def test_equality():
    def generate_objs():
        for foo in range(5):
            for bar in range(5):
                for awol in range(5):
                    for location in range(5):
                        obj = Foo(foo=foo, bar=bar, location=location)
                        obj.awol = awol
                        yield obj

    for obj1 in generate_objs():
        for obj2 in generate_objs():
            # only attrs matter to the equality class
            assert (obj1 == obj2) == (obj1.foo == obj2.foo and obj1.bar == obj2.bar)

            # check that hash is consistent with equality:
            # obj1 == obj2  =>  hash(obj1) == hash(obj2)
            assert not (obj1 == obj2) or (hash(obj1) == hash(obj2))
