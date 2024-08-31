import pytest

from alice.json_schema_builder.library import errors, nodes, parser
from collections import OrderedDict


OBJ = {
    'foo': 1,
    'bar': [1, 2, 3],
    'baz': {'a': 2.0, 'b': 3.0},
    'bad_bar': [1, 2, '3'],
    'bad_baz': {'a': 2.0, 'b': '3.0'},
}

LOCATION = nodes.Ref(filename='test.json', path=())


def dummy_use(obj, key):
    pass


@pytest.mark.parametrize('key, obj_type, item_type', [
    ('foo', int, None),
    ('bar', list, int),
    ('baz', dict, float),
    ('bad_bar', list, None),
    ('bad_baz', dict, None),
    ('awol', int, None),
])
def test_get_field_positive(key, obj_type, item_type):
    assert OBJ.get(key) == parser.get_field(OBJ, key, obj_type, dummy_use, LOCATION, item_type=item_type)


@pytest.mark.parametrize('key, obj_type, item_type', [
    ('foo', str, None),
    ('bar', int, None),
    ('bar', list, list),
    ('baz', list, None),
    ('baz', dict, int),
    ('bad_bar', list, int),
    ('bad_bar', list, str),
    ('bad_baz', dict, float),
    ('bad_baz', dict, str),
])
def test_get_field_negative(key, obj_type, item_type):
    with pytest.raises(errors.InvalidFieldTypeError):
        parser.get_field(OBJ, key, obj_type, dummy_use, LOCATION, item_type=item_type)


def test_invalid_type():
    with pytest.raises(errors.UnknownNodeError):
        parser.parse_raw_schema({'type': 'awol'}, {}, dummy_use, LOCATION)


def test_builtin():
    assert nodes.Builtin(fmt='color') == parser.parse_raw_schema(
        {'type': 'string', 'format': 'color'},
        {},
        dummy_use,
        LOCATION
    )


def test_any_of():
    bar_ref = nodes.Ref(filename='foo', path=('bar',))
    baz_ref = nodes.Ref(filename='foo', path=('baz',))
    target = {
        'anyOf': [
            'bar',
            'baz',
        ]
    }
    refs = {
        id(target['anyOf'][0]): bar_ref,
        id(target['anyOf'][1]): baz_ref,
    }
    actual = parser.parse_raw_schema(target, refs, dummy_use, LOCATION)
    expected = nodes.Variant(alternatives=[bar_ref, baz_ref])
    assert expected == actual

    target['anyOf'][0] = 'awol'  # something not in refs
    with pytest.raises(errors.InvalidAnyOfError):
        parser.parse_raw_schema(target, refs, dummy_use, LOCATION)


def test_payload():
    target = {'type': 'object', 'additionalProperties': True}
    actual = parser.parse_raw_schema(target, {}, dummy_use, LOCATION)
    assert nodes.JsonPayload() == actual


@pytest.mark.parametrize('target', [
    {'additionalProperties': True},
    {'type': 'array', 'additionalProperties': True},
    {'type': 'object', 'additionalProperties': True, 'definitions': {}},
    {'type': 'object', 'additionalProperties': True, 'properties': {}},
    {'type': 'object', 'additionalProperties': True, 'required': []},
])
def test_payload_negative(target):
    with pytest.raises(errors.InvalidAdditionalPropertiesError):
        parser.parse_raw_schema(target, {}, dummy_use, LOCATION)


# TODO(a-square): add negative tests
def test_object():
    target = {
        'type': 'object',
        'properties': {
            'foo': {},
        },
        'definitions': {
            'bar': {}
        },
        'required': [
            'foo',
        ],
    }
    ref = nodes.Ref(filename='foo', path=('bar',))
    refs = {id(target['properties']['foo']): ref, id(target['definitions']['bar']): ref}
    expected = nodes.Object(
        properties={'foo': ref},
        required=['foo'],
        constants=OrderedDict(),
    )
    assert expected == parser.parse_raw_schema(target, refs, dummy_use, LOCATION)

    del target['definitions']['bar']
    assert expected == parser.parse_raw_schema(target, refs, dummy_use, LOCATION)

    del target['definitions']
    assert expected == parser.parse_raw_schema(target, refs, dummy_use, LOCATION)

    del target['required']
    expected.required = []
    assert expected == parser.parse_raw_schema(target, refs, dummy_use, LOCATION)


def test_array():
    items = {}
    items_ref = nodes.Ref(filename='foo', path=('bar',))
    refs = {id(items): items_ref}
    assert nodes.Array(items=items_ref, min_items=0) == parser.parse_raw_schema(
        {'type': 'array', 'items': items},
        refs,
        dummy_use,
        LOCATION
    )
    assert nodes.Array(items=items_ref, min_items=5) == parser.parse_raw_schema(
        {'type': 'array', 'items': items, 'minItems': 5},
        refs,
        dummy_use,
        LOCATION
    )


@pytest.mark.parametrize('target', [
    {'type': 'array'},
    {'type': 'array', 'items': {}},
])
def test_array_negative(target):
    with pytest.raises(errors.InvalidArrayError):
        parser.parse_raw_schema(target, {}, dummy_use, LOCATION)


def test_number():
    assert nodes.Number() == parser.parse_raw_schema({'type': 'number'}, {}, dummy_use, LOCATION)


def test_integer():
    assert nodes.Integer() == parser.parse_raw_schema({'type': 'integer'}, {}, dummy_use, LOCATION)


def test_string():
    assert nodes.String() == parser.parse_raw_schema({'type': 'string'}, {}, dummy_use, LOCATION)
    assert nodes.String(min_length=3) == parser.parse_raw_schema({'type': 'string', 'minLength': 3}, {}, dummy_use, LOCATION)
    assert nodes.String(min_length=3, max_length=5) == parser.parse_raw_schema(
        {'type': 'string', 'minLength': 3, 'max_length': 5}, {}, dummy_use, LOCATION
    )

    with pytest.raises(errors.InvalidStringError):
        parser.parse_raw_schema({'type': 'string', 'minLength': -1}, {}, dummy_use, LOCATION)

    with pytest.raises(errors.InvalidStringError):
        parser.parse_raw_schema({'type': 'string', 'max_length': -1}, {}, dummy_use, LOCATION)


@pytest.mark.parametrize('options', [
    ['foo'],
    ['foo', 'bar'],
])
def test_string_enum(options):
    assert nodes.StringEnum(options=options) == parser.parse_raw_schema(
        {'type': 'string', 'enum': options},
        {},
        dummy_use,
        LOCATION
    )


@pytest.mark.parametrize('options, exc', [
    ([], errors.InvalidEnumError),
    (['foo', 1], errors.InvalidFieldTypeError),
])
def test_string_negative(options, exc):
    with pytest.raises(exc):
        parser.parse_raw_schema({'type': 'string', 'enum': options}, {}, dummy_use, LOCATION)


def test_ref():
    ref = nodes.Ref(filename='foo', path=('bar',))
    target = {'$ref': 'foo#/bar'}
    refs = {id(target['$ref']): ref}
    assert ref == parser.parse_raw_schema(target, refs, dummy_use, LOCATION)


def test_patch_refs():
    def _ref(name):
        return nodes.Ref(filename='foo', path=(name,))

    schemas = {
        _ref('obj'): nodes.Object(
            properties={
                'foo': _ref('foo'),
                'bar': _ref('bar'),
                'some_const': _ref('some_const'),
            },
            required=[
                'foo',
                'bar',
                'some_const',
            ],
            constants={},
        ),
        _ref('foo'): nodes.String(),
        _ref('bar'): nodes.StringEnum(options=['foo', 'bar']),
        _ref('some_const'): nodes.StringEnum(options=['only_option']),
    }

    parser.patch_refs(schemas)

    assert schemas[_ref('obj')] == nodes.Object(
        properties={
            'foo': nodes.String(),
            'bar': nodes.StringEnum(options=['foo', 'bar']),
            'some_const': nodes.StringEnum(options=['only_option']),
        },
        required=[
            'foo',
            'bar',
            'some_const',
        ],
        constants={},
    )


def test_patch_constants():
    def _ref(name):
        return nodes.Ref(filename='foo', path=(name,))

    schemas = {
        _ref('obj'): nodes.Object(
            properties={
                'foo': nodes.String(),
                'bar': nodes.StringEnum(options=['foo', 'bar']),
                'some_const': nodes.StringEnum(options=['only_option']),
            },
            required=[
                'foo',
                'bar',
                'some_const',
            ],
            constants={},
        ),
    }

    parser.patch_constants(schemas)

    assert schemas[_ref('obj')] == nodes.Object(
        properties={
            'foo': nodes.String(),
            'bar': nodes.StringEnum(options=['foo', 'bar']),
        },
        required=[
            'foo',
            'bar',
        ],
        constants={
            'some_const': 'only_option',
        },
    )
