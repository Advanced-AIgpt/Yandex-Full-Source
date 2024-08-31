import glob
import json
import os

from alice.json_schema_builder.library import errors, nodes, util
from alice.library.tool_log import tool_log
from collections import OrderedDict


KNOWN_FORMATS = (
    'boolean',
    'color',
    'formatted_string',
    'uri',
)

IGNORED_FIELDS = (
    'alias',
    'clientMinItems',
    'clientMinLength',
    'client_optimized',
    'default_value',  # TODO(a-square): useful for JSON producer? (e.g. compression)
    'description',
    'generate_serialization_swift',
    'include_in_documentation_toc',
    'java_extends',
    'java_extends_local',
    'kotlin_interfaces',
    'protocol_name',
    'strictParsing',
    'swift_super_protocol',
    'version',
)


class JSONSchemaParser:
    """Top-level JSON Schema parser.
    Collects parse trees for schemas found in given files and directories.
    """
    def __init__(self):
        self.schemas = OrderedDict()
        self._raw_schemas = OrderedDict()  # maps formalized references to node refs
        self._refs = {}  # maps node ids to formalized references
        self._storage = OrderedDict()  # stores contents of files parsed
        self._used = set()

    def calc_unknowns(self):
        """Returns JSON Schema data as it was originally parsed as JSON,
        removes values that we haven't marked as used, that is, returns
        values that we don't know how to process.

        If the result is not empty, it is unsafe to proceed with the code
        generation in production environment.
        """
        return util.filter_nested(self._storage, remove_pred=self._is_used)

    def calc_knowns(self):
        """Returns JSON Schema data as it was originally parsed as JSON,
        removes values that we have marked as used, that is, returns
        values that we know how to process.
        """
        return util.filter_nested(self._storage, remove_pred=self._is_unused)

    def calc_dependencies(self):
        refs = self._refs

        result = OrderedDict()
        for raw_schema_ref, raw_schema in self._raw_schemas.items():
            deps = []
            for _, obj in iter_raw_schema_objects(raw_schema, refs):
                if obj is raw_schema:
                    continue

                ref = refs.get(id(obj))
                if ref is not None:
                    deps.append(ref)

                ref_obj = obj.get('$ref')
                if ref_obj is not None:
                    deps.append(refs[id(ref_obj)])

            result[raw_schema_ref] = deps

        return result

    def calc_raw_schemas(self):
        refs = self._refs

        def _truncate(obj):
            ref = refs.get(id(obj))
            if ref is not None:
                return ref

            if isinstance(obj, dict):
                return OrderedDict(
                    (key, _truncate(value))
                    for key, value in obj.items()
                )

            if isinstance(obj, list):
                return list(map(_truncate, obj))

            return obj

        result = OrderedDict()
        for raw_schema_ref, raw_schema in self._raw_schemas.items():
            # NOTE(a-square): raw_schema is in refs, so we can't call _truncate on it directly
            assert isinstance(raw_schema, dict)
            result[raw_schema_ref] = OrderedDict(
                (key, _truncate(value))
                for key, value in raw_schema.items()
            )

        return result

    def scan_directory(self, dir_path):
        """Scans the given directory for JSON files, ignores subdirectories.
        Files found are then parsed.
        """
        for json_path in glob.iglob(os.path.join(dir_path, '*.json')):
            with open(json_path) as json_file:
                self.scan_file(json_file)

    def scan_file(self, fobj, name=None):
        """Scans the contents of the given file as a JSON schema.
        """
        if name is None:
            name = os.path.basename(fobj.name)
        if name in self._storage:
            raise errors.ParserUsageError('File already parsed: {}'.format(name))
        self._storage[name] = patch_all_ofs(json.load(fobj, object_pairs_hook=OrderedDict))
        self._use(self._storage, name)
        self._parse_node(name)

    def parse_schemas(self):
        """Parses raw schemas into AST objects.
        """
        raw_schemas = self._raw_schemas
        refs = self._refs

        dangling_refs = set(refs.values()) - raw_schemas.keys()
        if dangling_refs:
            readable_refs = list(map(str, dangling_refs))
            raise errors.ParserUsageError('Dangling references found: {!r}'.format(readable_refs))

        for ref, raw_schema in self._raw_schemas.items():
            mark_known_irrelevant_fields(raw_schema, refs, self._use)
            self.schemas[ref] = parse_raw_schema(raw_schema, refs, self._use, ref)

        patch_refs(self.schemas)
        patch_constants(self.schemas)

    def _parse_node(self, json_name):
        """Parses the given node in _storage as a file containing JSON schemas.
        """
        raw_schemas, refs = collect_schemas_refs(json_name, self._storage[json_name], self._use)

        duplicate_schemas = self._raw_schemas.keys() & raw_schemas.keys()
        if duplicate_schemas:
            raise errors.ParserUsageError('Duplicate schema IDs: {!r}'.format(duplicate_schemas))

        self._raw_schemas.update(raw_schemas)
        self._refs.update(refs)

    def _use(self, obj, key):
        """Marks the stored object as used, meaning we know what it is and how it affects schema.
        It only makes sense to mark leaf objects, i.e. not lists or dicts.
        - `obj`: parent object
        - `key`: the key to the object that should be marked as used, `str` or `int`
        """
        self._used.add((id(obj), key))

    def _is_used(self, obj, key):
        """Checks if the given object was previously marked as used.
        - `obj`: parent object
        - `key`; the key to the object that needs checking, `str` or `int`
        """
        return (id(obj), key) in self._used

    def _is_unused(self, obj, key):
        return not self._is_used(obj, key)


def patch_all_ofs(contents):
    """Patches away all "allOf" keys by validating that their values are arrays
    and merging them into the parent object.
    Because of this, the storage no longer reflects the real JSON file read,
    but it's better than dealing with issues like pointer equivalence, property lookup
    and so on.
    """
    if isinstance(contents, list):
        return list(map(patch_all_ofs, contents))

    if isinstance(contents, dict):
        all_of = contents.get('allOf')
        if all_of is not None:
            contents.pop('allOf')
            if not isinstance(all_of, list):
                raise errors.InvalidAllOfError('allOf must be an array, got {!r}'.format(type(all_of).__name__))
            new_contents = util.recursive_merge([contents] + all_of)
            return patch_all_ofs(new_contents)  # in case there are nested allOfs which now lie at the top level

        return OrderedDict(
            (key, patch_all_ofs(value))
            for key, value in contents.items()
        )

    return contents


def collect_schemas_refs(filename, contents, use):
    """Collects (raw) schemas and references to schemas from a nested JSON schema
    with the intention of:
    - using raw schemas as starting points for AST recovery
    - using refs as stop gaps to stop further descent during AST recovery
      - collecting both schemas and refs into the same refs dict allows for uniform stop test
    """
    schemas = OrderedDict()
    refs = {}

    # NOTE(a-square): common.json has a peculiar structure,
    # and our assumptions about parsing are such that we never have
    # a chance to mark all of its immediate children as used after this
    #
    # TODO(a-square): consider reworking iter_json_objects to remove this crutch
    if filename == 'common.json':
        for key in contents.keys():
            use(contents, key)

    for location_factory, obj in util.iter_json_objects(contents, filename=filename):
        # what counts as an object schema:
        # - object schemas with an explicit "type" (must be a string)
        # - object schemas with a "$ref", which we assume point to object schemas
        # - variant types with "anyOf"
        #
        # There are also mixin objects ("allOf"), but we expand them before this function is ever called.
        obj_type = obj.get('type', None)
        obj_any_of = obj.get('anyOf', None)
        obj_ref = obj.get('$ref', None)
        if isinstance(obj_type, str) or obj_any_of is not None or obj_ref is not None:
            location = location_factory()
            schemas[location] = obj
            refs[id(obj)] = location

            if obj_ref is not None:
                use(obj, '$ref')
                ref = parse_ref(obj_ref, location)
                refs[id(obj_ref)] = ref

    return schemas, refs


def mark_known_irrelevant_fields(raw_schema, refs, use):
    for _, obj in iter_raw_schema_objects(raw_schema, refs):
        for key in IGNORED_FIELDS:
            value = obj.get(key)
            if value is not None:
                use(obj, key)


def parse_raw_schema(raw_schema, refs, use, location):
    parsers = [
        _try_parse_builtin,  # must go first so that it can fall back on others
        _try_parse_payload,
        _try_parse_any_of,
        _try_parse_object,
        _try_parse_array,
        _try_parse_number,
        _try_parse_integer,
        _try_parse_string,
        _try_parse_ref,
    ]

    for parser in parsers:
        result = parser(raw_schema, refs, use, location)
        if result is not None:
            return result

    raise errors.UnknownNodeError('Could not find a correct parse for schema: {}'.format(location))


def parse_ref(ref_str, location):
    """Parses a textual representation of a schema pointer into a `node.Ref` object.
    Note: `location` is also a node.Ref object, presumably constructed by hand.
    """
    parts = ref_str.split('#', 1)
    if len(parts) == 2:
        filename, raw_path = parts
    else:
        assert len(parts) == 1
        filename = parts[0]
        raw_path = ''

    if not filename:
        filename = location.filename if location is not None else None

    if raw_path and not raw_path.startswith('/'):
        raise errors.InvalidReferenceError('Relative pointers are currently not supported.')

    def _int_or_string(value):
        try:
            return int(value)
        except ValueError:
            return str(value)

    path = tuple(
        _int_or_string(path_entry)
        for path_entry in raw_path.split('/')
        if path_entry
    )

    return nodes.Ref(filename=filename, path=path, location=location)


def patch_refs(schemas):
    """Patches schemas by replacing reference chains with their tails.
    This saves us from having to deal with them during code generation.

    This cannot be done in the first pass over raw schemas
    because at that point we don't know the targets of all refs yet.
    """
    def _resolve_refs(node):
        while isinstance(node, nodes.Ref):
            node = schemas[node]
        return node

    for schema in schemas.values():
        # we only go one layer deep, no need to do recursion
        for attr in schema.attrs:
            attr_value = getattr(schema, attr)
            if isinstance(attr_value, nodes.Ref):
                setattr(schema, attr, _resolve_refs(attr_value))
            elif isinstance(attr_value, dict):
                for key, value in attr_value.items():
                    if isinstance(value, nodes.Ref):
                        attr_value[key] = _resolve_refs(value)
            elif isinstance(attr_value, list):
                for key, value in enumerate(attr_value):
                    if isinstance(value, nodes.Ref):
                        attr_value[key] = _resolve_refs(value)


def patch_constants(schemas):
    """Patches schemas by moving known string constants from normal properties
    into their own attribute.
    """
    for schema in schemas.values():
        if not isinstance(schema, nodes.Object):
            continue

        constants = OrderedDict()
        for required_prop_name in schema.required:
            prop = schema.properties[required_prop_name]
            if not isinstance(prop, nodes.StringEnum):
                continue

            if len(prop.options) != 1:
                continue

            constants[required_prop_name] = prop

        for prop_name, prop in constants.items():
            schema.required.remove(prop_name)
            del schema.properties[prop_name]
            schema.constants[prop_name] = prop.options[0]


def iter_raw_schema_objects(raw_schema, refs):
    """Iterates over JSON objects (i.e. dicts) on of the given raw schema.
    Stops at references and nested schemas, that is they are NOT yielded.
    """
    def _stop(value):
        return value is not raw_schema and id(value) in refs

    for location_factory, obj in util.iter_json_objects(raw_schema, stop=_stop):
        yield location_factory, obj


def _try_parse_builtin(raw_schema, refs, use, location):
    if 'format' not in raw_schema:
        return None

    use(raw_schema, 'format')

    fmt = raw_schema['format']
    if fmt not in KNOWN_FORMATS:
        tool_log.warning('Unknown format: {!r}'.format(fmt))
        return None

    if fmt == 'formatted_string':
        return None  # fall back to treating it as a normal string

    schema_type = raw_schema.get('type')
    if schema_type is not None:
        use(raw_schema, 'type')

    if fmt in ('uri', 'color'):
        if schema_type != 'string':
            raise errors.UnexpectedBuiltinTypeError('Unexpected type {!r} for format {!r}'.format(
                schema_type,
                fmt,
            ))

    # some extra nodes are used for some nodes
    if fmt == 'boolean':
        if schema_type != 'integer':
            raise errors.UnexpectedBuiltinTypeError('Unexpected type {!r} for format {!r}'.format(
                schema_type,
                fmt,
            ))
        get_field(raw_schema, 'enum', list, use, location, item_type=int)  # mark as used

    return nodes.Builtin(fmt=fmt, location=location)


def _try_parse_any_of(raw_schema, refs, use, location):
    if 'anyOf' not in raw_schema:
        return None

    use(raw_schema, 'anyOf')
    anyOf = raw_schema['anyOf']

    alternatives = []
    for index, item in enumerate(anyOf):
        use(anyOf, index)
        item_ref = refs.get(id(item))
        if item_ref is None:
            raise errors.InvalidAnyOfError('Invalid anyOf item: {}'.format(location.child(index)))
        alternatives.append(item_ref)

    return nodes.Variant(alternatives=alternatives, location=location)


def _try_parse_payload(raw_schema, refs, use, location):
    if raw_schema.get('additionalProperties') is not True:
        return None

    schema_type = raw_schema.get('type')
    if schema_type != 'object':
        raise errors.InvalidAdditionalPropertiesError(
            'Invalid additional properties object: {}'.format(location.child('object'))
        )

    for forbidden_key in ('definitions', 'properties', 'required'):
        if forbidden_key in raw_schema:
            raise errors.InvalidAdditionalPropertiesError(
                'Invalid additional properties object: {}'.format(location.child(forbidden_key))
            )

    use(raw_schema, 'type')
    use(raw_schema, 'additionalProperties')

    return nodes.JsonPayload(location=location)


def _try_parse_object(raw_schema, refs, use, location):
    if 'properties' not in raw_schema:
        return None

    schema_type = raw_schema.get('type')
    ref = raw_schema.get('$ref')
    if schema_type != 'object' and id(ref) not in refs:
        raise errors.InvalidObjectError('Expected type = object: {}'.format(location.child('object')))

    if schema_type is not None:
        use(raw_schema, 'type')

    # definitions must reference other schemas
    definitions = get_field(raw_schema, 'definitions', dict, use, location)
    if definitions is not None:
        for key, value in definitions.items():
            if id(value) not in refs:
                raise errors.InvalidObjectError(
                    'Invalid definition: {}'.format(location.child('definitions').child(key))
                )
            use(definitions, key)

    properties = get_field(raw_schema, 'properties', dict, use, location)
    assert properties is not None

    # just ignore these
    get_field(raw_schema, 'ignore_properties_java', list, use, location, item_type=str)

    prop_refs = OrderedDict()
    for key, value in properties.items():
        prop_ref = refs.get(id(value))
        if prop_ref is None:
            raise errors.InvalidObjectError(
                'Invalid property: {}'.format(location.child('properties').child(key))
            )

        use(properties, key)
        prop_refs[key] = prop_ref

    required = get_field(raw_schema, 'required', list, use, location, item_type=str) or []

    if not set(required).issubset(prop_refs.keys()):
        raise errors.InvalidObjectError('Required must be a subset of properties: {}'.format(location))

    return nodes.Object(
        properties=prop_refs,
        required=required,
        location=location,
        constants=OrderedDict(),
    )


def _try_parse_array(raw_schema, refs, use, location):
    if raw_schema.get('type') != 'array':
        return None

    use(raw_schema, 'type')

    items = get_field(raw_schema, 'items', dict, use, location)
    if items is None:
        raise errors.InvalidArrayError('Absent items: {}'.format(location))

    if id(items) not in refs:
        raise errors.InvalidArrayError('Invalid items: {}'.format(location.child('items')))

    min_items = get_field(raw_schema, 'minItems', int, use, location) or 0

    return nodes.Array(
        items=refs[id(items)],
        min_items=min_items,
        location=location,
    )


def _try_parse_number(raw_schema, refs, use, location):
    if raw_schema.get('type') != 'number':
        return None

    use(raw_schema, 'type')

    constraint = get_field(raw_schema, 'constraint', str, use, location)
    return nodes.Number(location=location, constraint=constraint)


def _try_parse_integer(raw_schema, refs, use, location):
    if raw_schema.get('type') != 'integer':
        return None

    use(raw_schema, 'type')

    constraint = get_field(raw_schema, 'constraint', str, use, location)
    return nodes.Integer(location=location, constraint=constraint)


def _try_parse_string(raw_schema, refs, use, location):
    if raw_schema.get('type') != 'string':
        return None

    use(raw_schema, 'type')

    enum = get_field(raw_schema, 'enum', list, use, location, item_type=str)
    if enum is not None:
        if not enum:
            raise errors.InvalidEnumError('Enum must have options: {}'.format(location.child('enum')))
        return nodes.StringEnum(
            options=enum,
            location=location,
        )

    min_length = get_field(raw_schema, 'minLength', int, use, location)
    if min_length is not None and min_length < 0:
        raise errors.InvalidStringError('minLength must be positive, got {}'.format(min_length))
    max_length = get_field(raw_schema, 'max_length', int, use, location)
    if max_length is not None and max_length < 0:
        raise errors.InvalidStringError('max_length must be positive, got {}'.format(max_length))

    pattern = get_field(raw_schema, 'pattern', str, use, location)

    return nodes.String(
        min_length=min_length,
        max_length=max_length,
        location=location,
        pattern=pattern,
    )


def _try_parse_ref(raw_schema, refs, use, location):
    ref = raw_schema.get('$ref')
    if ref is None:
        return None

    ref_obj = refs.get(id(ref))
    if ref_obj is None:
        return None

    # TODO(a-square): ref objects can have default_value and friends, deal with them
    return ref_obj


def get_field(obj, key, obj_type, use, location, item_type=None):
    """A helper that gets the field, type-checks it and marks it as used.
    Optionally, it also type-checks and marks as used its children.
    """
    if key not in obj:
        return None

    value = obj[key]
    value_location = location.child(key)
    if not isinstance(value, obj_type):
        raise errors.InvalidFieldTypeError('Expected type {}: {}'.format(obj_type, value_location))

    use(obj, key)
    if item_type is not None:
        assert isinstance(value, (list, dict))
        if isinstance(value, list):
            iterator = enumerate(value)
        else:
            iterator = value.items()

        for key, item in iterator:
            if not isinstance(item, item_type):
                raise errors.InvalidFieldTypeError('Expected type {}: {}'.format(item_type, value_location.child(key)))
            use(value, key)

    return value
