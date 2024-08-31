import pytest

from alice.json_schema_builder.library import errors
from alice.json_schema_builder.library.nodes import Ref
from alice.json_schema_builder.library.parser import parse_ref


LOCATION = Ref(filename='default.json', path=('foo', 2))


@pytest.mark.parametrize(
    'ref, ref_str',
    [
        (Ref(filename='foo.json', path=()), 'foo.json'),
        (Ref(filename='foo.json', path=('bar', 3)), 'foo.json#/bar/3'),
        (Ref(filename=None, path=('bar', 'baz')), '#/bar/baz'),
    ],
)
def test_ref_node_representation(ref, ref_str):
    assert ref_str == str(ref)
    assert ref_str == ref.to_json()


@pytest.mark.parametrize(
    'ref_str, location, ref',
    [
        ('#', None, Ref(filename=None, path=())),
        ('#', LOCATION, Ref(filename=LOCATION.filename, path=())),
        ('#/baz/1', None, Ref(filename=None, path=('baz', 1))),
        ('#/baz/1', LOCATION, Ref(filename=LOCATION.filename, path=('baz', 1))),
        ('foo.json', None, Ref(filename='foo.json', path=())),
        ('foo.json', LOCATION, Ref(filename='foo.json', path=())),
        ('foo.json#/baz/1', None, Ref(filename='foo.json', path=('baz', 1))),
        ('foo.json#/baz/1', LOCATION, Ref(filename='foo.json', path=('baz', 1))),
    ],
)
def test_ref_node_parsing_positive(ref_str, location, ref):
    assert ref == parse_ref(ref_str, location)


def test_ref_node_parsing_negative():
    with pytest.raises(errors.InvalidReferenceError):
        parse_ref('#bar', LOCATION)


def test_ref_child():
    assert Ref(path=('foo', 1)) == Ref().child('foo').child(1)
    assert Ref(filename='test.json', path=(1, 2)) == Ref(filename='test.json', path=(1,)).child(2)
