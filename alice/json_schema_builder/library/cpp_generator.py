import itertools
import os
import shutil

from alice.json_schema_builder.library import errors, nodes, visitor
from alice.library.python.code_generator.code_generator import (
    CppCodeGenerator,
    YaMakeCodeGenerator,
    camelize,
    pascalize,
    stringbuf,
)
from collections import OrderedDict
from contextlib import contextmanager
from yandex_inflector_python import Inflector


ENUMS_HEADER = 'enums.h'
JSON_EXT = '.json'
LOG_ID_FILLER_HEADER = 'log_id_filler.h'
LOG_ID_FILLER_SOURCE = 'log_id_filler.cpp'
PATTERNS_HEADER = 'patterns.h'
RUNTIME_HEADER = 'alice/json_schema_builder/runtime/runtime.h'
RUNTIME_PEERDIR = 'alice/json_schema_builder/runtime'
RUNTIME_NAMESPACE = 'NAlice::NJsonSchemaBuilder::NRuntime'
VALIDATOR_HEADER = 'validate.h'
VALIDATOR_SOURCE = 'validate.cpp'

BASE_BUILDER = RUNTIME_NAMESPACE + '::TBuilder'

INFLECTOR = Inflector('en')


def generate_cpp(schemas, out_dir, namespace, clean_directory=False, generate_log_id_filler=False):
    cpp_files = []

    @contextmanager
    def _code_gen(name, cls=CppCodeGenerator):
        if name.endswith('.cpp'):
            cpp_files.append(name)

        path = os.path.join(out_dir, name)
        os.makedirs(os.path.dirname(path), exist_ok=True)
        with open(path, 'wb') as fobj:
            cg = cls(fobj)
            if name.endswith('.h'):
                cg.pragma('once')
                cg.skip_line()
            cg.banner('alice/json_schema_builder/tool')
            cg.skip_line()
            yield cg

    if clean_directory:
        shutil.rmtree(out_dir, ignore_errors=True)

    pattern_holder = PatternHolder()
    resolver = PropertyResolver(pattern_holder)

    with _code_gen('builders.h') as builders_cg:
        generate_objects(builders_cg, schemas, resolver, namespace)

    with _code_gen(PATTERNS_HEADER) as patterns_cg:
        generate_patterns(patterns_cg, pattern_holder, namespace)

    with _code_gen(ENUMS_HEADER) as enums_cg:
        generate_enums(enums_cg, resolver, namespace)

    with _code_gen(VALIDATOR_HEADER) as validator_h_cg:
        generate_schema_validator_h(validator_h_cg, namespace)

    with _code_gen(VALIDATOR_SOURCE) as validator_cpp_cg:
        generate_schema_validator_cpp(validator_cpp_cg, namespace)

    if generate_log_id_filler:
        with _code_gen(LOG_ID_FILLER_HEADER) as log_id_filler_h_cg:
            generate_log_id_filler_header(log_id_filler_h_cg, namespace)

        with _code_gen(LOG_ID_FILLER_SOURCE) as log_id_filler_cpp_cg:
            generate_log_id_filler_source(log_id_filler_cpp_cg, namespace)

    with _code_gen('ya.make', YaMakeCodeGenerator) as ya_make_cg:
        generate_ya_make(ya_make_cg, cpp_files)


def generate_objects(cg, schemas, resolver, namespace):
    cg.include(RUNTIME_HEADER, system=True)
    cg.skip_line()

    cg.include(ENUMS_HEADER)
    cg.include(PATTERNS_HEADER)
    cg.skip_line()

    with cg.namespace(namespace):
        for schema in schemas.values():
            if isinstance(schema, nodes.Object):
                generate_builder(cg, schemas, schema, resolver)

    resolver.process_variant_alternatives()


def generate_builder(cg, schemas, schema, resolver):
    accessors = OrderedDict()

    with cg.class_def('struct {} : public {}'.format(
        builder_name(schema.location),
        BASE_BUILDER,
    )):
        generate_constructor(cg, schema)

        for prop_name, prop in schema.properties.items():
            accessor_generator = resolver.resolve(prop)
            accessor_generator.write(cg, schema, prop_name)
            accessors[prop_name] = accessor_generator

    generate_builder_makers(cg, schemas, schema, accessors)


def generate_constructor(cg, schema):
    assert isinstance(schema, nodes.Object)
    with cg.func_def('{}()'.format(builder_name(schema.location))):
        # TODO(a-square): consider moving SetType to the base TBuilder
        cg.write_line('Value_.SetType(NJson::JSON_MAP);')
        for constant_name, constant_value in schema.constants.items():
            cg.write_line('Value_[{constant_name}] = {constant_value};'.format(
                constant_name=stringbuf(constant_name),
                constant_value=stringbuf(constant_value),
            ))


def generate_builder_makers(cg, schemas, schema, accessors):
    """Generates free maker functions that set required properties
    of an object, except for array properties, which we don't want in maker functions.
    """
    assert isinstance(schema, nodes.Object)

    class_name = builder_name(schema.location)
    func_name = ref_name(schema.location)

    def _check_constructor_property(prop_name):
        if prop_name not in schema.required:
            return False

        return not isinstance(schema.properties[prop_name], nodes.Array)

    def _iter_concrete_accessors(accessors):
        property_accessors = [
            accessor.calc_property_accessors()
            for prop_name, accessor in accessors.items()
        ]
        return itertools.product(*property_accessors)

    # default maker
    with cg.func_def('[[nodiscard]] inline {class_name} {func_name}()'.format(
        class_name=class_name,
        func_name=func_name,
    )):
        cg.write_line('return {};')

    constructor_properties = OrderedDict(
        (prop_name, accessor)
        for prop_name, accessor in accessors.items()
        if _check_constructor_property(prop_name)
    )

    if not constructor_properties:
        return

    # makers that set required parameters (except for arrays)
    for concrete_accessors in _iter_concrete_accessors(constructor_properties):
        with cg.func_def('[[nodiscard]] inline {class_name} {func_name}({params})'.format(
            class_name=class_name,
            func_name=func_name,
            params=', '.join(
                '{param_type} {param_name}'.format(
                    param_type=accessor.param_type,
                    param_name=camelize(prop_name),
                )
                for prop_name, accessor in zip(constructor_properties.keys(), concrete_accessors)
            )
        )):
            line = 'return {class_name}{{}}'.format(class_name=class_name)
            line += ''.join(
                '.{accessor_name}({value_cast})'.format(
                    accessor_name=pascalize(prop_name),
                    value_cast=camelize(prop_name) if accessor.lvalue else 'std::move({})'.format(camelize(prop_name)),
                )
                for prop_name, accessor in zip(constructor_properties.keys(), concrete_accessors)
            )
            line += ';'
            cg.write_line(line)


def generate_patterns(cg, pattern_holder, namespace):
    cg.include(RUNTIME_HEADER, system=True)
    cg.skip_line()

    with cg.namespace(namespace):
        for pattern, pattern_id in pattern_holder.patterns.items():
            cg.write_line('inline re2::RE2 {pattern_name}(R"regex({pattern})regex"));'.format(
                pattern_name=pattern_name(pattern_id),
                pattern=pattern,
            ))


def generate_enums(cg, resolver, namespace):
    cg.include(RUNTIME_HEADER, system=True)
    cg.skip_line()

    with cg.namespace(namespace):
        for enum in resolver.enums.values():
            generate_enum(cg, enum)


def generate_enum(cg, enum):
    option_num = 0
    with cg.enum_class(enum_name(enum.location)):
        for option in enum.options:
            cg.enum_option(pascalize(option), option_num, serialization=option)
            option_num += 1


def generate_schema_validator_h(cg, namespace):
    cg.include(RUNTIME_HEADER, system=True)
    cg.skip_line()

    with cg.namespace(namespace):
        cg.write_line(
            'NJson::TJsonValue Validate(NAlice::NJsonSchemaBuilder::NRuntime::TBuilder&& builder);'
        )


def generate_schema_validator_cpp(cg, namespace):
    cg.include(VALIDATOR_HEADER)
    cg.skip_line()

    with cg.namespace(namespace):
        # TODO(a-square): generate validation functions
        with cg.func_def(
            'NJson::TJsonValue Validate({}::TBuilder&& builder)'.format(RUNTIME_NAMESPACE)
        ):
            # TODO(a-square): validate the card
            cg.write_line('return std::move(builder).ValueWithoutValidation();')


def generate_log_id_filler_header(cg, namespace):
    cg.include(RUNTIME_HEADER, system=True)
    cg.skip_line()

    with cg.namespace(namespace):
        pass  # TODO(a-square)


def generate_log_id_filler_source(cg, namespace):
    cg.include(LOG_ID_FILLER_HEADER)
    cg.skip_line()

    with cg.namespace(namespace):
        pass  # TODO(a-square)


def generate_ya_make(cg, cpp_files):
    with cg.target('LIBRARY'):
        cg.section('OWNER', ['g:alice'])
        cg.section('PEERDIR', [RUNTIME_PEERDIR])
        cg.section('SRCS', sorted(cpp_files))
        cg.section('GENERATE_ENUM_SERIALIZATION', [ENUMS_HEADER])


class PropertyResolver(visitor.NodeVisitor):
    def __init__(self, pattern_holder):
        super().__init__()
        self.enums = OrderedDict()
        self.variants = OrderedDict()
        self.pattern_holder = pattern_holder

    def resolve(self, prop):
        return self.visit(prop)

    def process_variant_alternatives(self):
        for variant in self.variants.values():
            for alternative in variant.alternatives:
                self.resolve(alternative)  # make sure we visited everything

    def visit_Builtin(self, node):
        fmt = node.fmt
        if fmt == 'boolean':
            # NOTE(a-square): we must write boolean values as 0/1 for Perl compatibility
            return PropertyAccessor('const bool', 'static_cast<i64>(value)')
        if fmt == 'color':
            return PropertyAccessor('const TString&', 'value', validators=[ColorValidator()])
        if fmt == 'uri':
            return PropertyAccessor('const TString&', 'value', validators=[UriValidator()])
        raise errors.UnsupportedBuiltinTypeError('Unsupported built-in: {!r}'.format(node))

    def visit_Variant(self, node):
        self.variants[node.location] = node  # TODO(a-square): handle variant validation
        for alternative in node.alternatives:
            if not isinstance(alternative, nodes.Object):
                raise errors.UnsupportedVariantError(
                    'Currently only variants of objects are supported, found {}'.format(alternative)
                )

        return self.visit_Object(node)

    def visit_JsonPayload(self, node):
        validator = JsonPayloadValidator()
        return CombinedAccessor([
            PropertyAccessor('const NJson::TJsonValue&', 'value', validators=[validator]),
            PropertyAccessor('NJson::TJsonValue&&', 'std::move(value)', validators=[validator], lvalue=False),
        ])

    def visit_Object(self, node):
        return CombinedAccessor([
            PropertyAccessor('const {}::TBuilder&'.format(RUNTIME_NAMESPACE), 'value.ValueWithoutValidation()'),
            PropertyAccessor('{}::TBuilder&&'.format(RUNTIME_NAMESPACE), 'std::move(value).ValueWithoutValidation()', lvalue=False),
        ])

    def visit_Array(self, node):
        return CombinedAccessor([
            ArrayAdder(self.visit(node.items)),
            PropertyAccessor('const {}::TArrayBuilder&'.format(RUNTIME_NAMESPACE), 'value.ValueWithoutValidation()'),
            PropertyAccessor('{}::TArrayBuilder&&'.format(RUNTIME_NAMESPACE), 'std::move(value).ValueWithoutValidation()', lvalue=False),
        ])

    def visit_Number(self, node):
        validators = []
        if node.constraint is not None:
            validators.append(NumberValidator(node.constraint))

        return PropertyAccessor('const double', 'value', validators=validators)

    def visit_Integer(self, node):
        validators = []
        if node.constraint is not None:
            validators.append(NumberValidator(node.constraint, integer=True))

        return PropertyAccessor('const i64', 'value', validators=validators)

    def visit_String(self, node):
        validators = [StringLengthValidator(node.min_length, node.max_length)]
        if node.pattern is not None:
            validators.append(StringPatternValidator(self.pattern_holder.pattern_id(node.pattern)))
        return PropertyAccessor('const TString&', 'value', validators=validators)

    def visit_StringEnum(self, node):
        self.enums[node.location] = node
        return PropertyAccessor('const ' + enum_name(node.location), 'ToString(value)')


class PropertyAccessor:
    def __init__(self, param_type, value_cast, lvalue=True, validators=()):
        self.param_type = param_type
        self.value_cast = value_cast
        self.lvalue = lvalue
        self.validators = validators

    def calc_property_accessors(self):
        return [self]

    def write(self, cg, obj, prop_name):
        for lvalue_flag in fluent_method(
            cg,
            builder_name(obj.location),
            pascalize(prop_name),
            '{} value'.format(self.param_type)
        ):
            for validator in self.validators:
                validator.write(cg)

            cg.write_line('Value_[{key}] = {value_cast};'.format(
                key=stringbuf(prop_name),
                value_cast=self.value_cast,
            ))


class CombinedAccessor:
    def __init__(self, accessors):
        self.accessors = accessors

    def calc_property_accessors(self):
        result = []
        for accessor in self.accessors:
            result.extend(accessor.calc_property_accessors())
        return result

    def write(self, cg, obj, prop_name):
        for accessor in self.accessors:
            accessor.write(cg, obj, prop_name)


class ArrayAdder:
    def __init__(self, item_accessor):
        self.item_accessor = item_accessor

    def calc_property_accessors(self):
        return []

    def write(self, cg, obj, prop_name):
        accessor = self.item_accessor

        if isinstance(accessor, CombinedAccessor):
            for nested_accessor in accessor.accessors:
                if isinstance(nested_accessor, ArrayAdder):
                    raise errors.UnsupportedArrayPropertyError('Nested arrays not supported')

                self._write(cg, obj, prop_name, nested_accessor)

            return

        self._write(cg, obj, prop_name, accessor)

    def _write(self, cg, obj, prop_name, accessor):
        for lvalue_flag in fluent_method(
            cg,
            builder_name(obj.location),
            adder_name(prop_name),
            '{} value'.format(accessor.param_type),
        ):
            for validator in accessor.validators:
                validator.write(cg)

            cg.write_line('Value_[{key}].AppendValue({value_cast});'.format(
                key=stringbuf(prop_name),
                value_cast=accessor.value_cast,
            ))


class ColorValidator:
    def write(self, cg):
        cg.write_line('{ns}::ValidateColor(value);'.format(ns=RUNTIME_NAMESPACE))


class UriValidator:
    def write(self, cg):
        cg.write_line('{ns}::ValidateUri(value);'.format(ns=RUNTIME_NAMESPACE))


class JsonPayloadValidator:
    def write(self, cg):
        cg.write_line('{ns}::ValidateJsonPayload(value);'.format(ns=RUNTIME_NAMESPACE))


class NumberValidator:
    def __init__(self, constraint, integer=False):
        assert constraint is not None
        self.constraint = constraint
        self.integer = integer

    def write(self, cg):
        cg.write_line('{ns}::{func_name}({constraint_str}, [](const {num_type} number) {{ return {constraint}; }}, value);'.format(
            ns=RUNTIME_NAMESPACE,
            func_name='ValidateIntegerConstraint' if self.integer else 'ValidateDoubleConstraint',
            constraint_str=stringbuf(self.constraint),
            num_type='i64' if self.integer else 'double',
            constraint=self.constraint,
        ))


class StringLengthValidator:
    def __init__(self, min_length, max_length):
        self.min_length = min_length
        self.max_length = max_length

    def write(self, cg):
        if self.min_length is None and self.max_length is None:
            return

        cg.write_line('const auto length = {ns}::GetNumUtf16CharsInUtf8String(value);'.format(
            ns=RUNTIME_NAMESPACE,
        ))

        if self.min_length is not None:
            cg.write_line('{ns}::ValidateMinLength(length, {min_length} /* minLength */);'.format(
                ns=RUNTIME_NAMESPACE,
                min_length=self.min_length,
            ))

        if self.max_length is not None:
            cg.write_line('{ns}::ValidateMaxLength(length, {max_length} /* maxLength */);'.format(
                ns=RUNTIME_NAMESPACE,
                max_length=self.max_length,
            ))


class StringPatternValidator:
    def __init__(self, pattern_id):
        self.pattern = pattern_id

    def write(self, cg):
        cg.write_line('{ns}::ValidatePattern({pattern_name}, value);'.format(
            ns=RUNTIME_NAMESPACE,
            pattern_name=pattern_name(self.pattern_id),
        ))


class PatternHolder:
    def __init__(self):
        self.patterns = OrderedDict()

    def pattern_id(self, pattern):
        return self.patterns.setdefault(pattern, len(self.patterns))


def fluent_method(cg, class_name, func_name, params):
    for lvalue_flag in (False, True):
        with cg.func_def('{attrs}{class_name}{qual} {func_name}({params}) {qual}'.format(
            attrs='' if lvalue_flag else '[[nodiscard]] ',
            class_name=class_name,
            qual='&' if lvalue_flag else '&&',
            func_name=func_name,
            params=params,
        )):
            try:
                yield lvalue_flag
            finally:
                this_cast = '*this' if lvalue_flag else 'std::move(*this)'
                cg.write_line('return {};'.format(this_cast))


def builder_name(ref):
    return 'T' + ref_name(ref) + 'Builder'


def enum_name(ref):
    return 'E' + ref_name(ref)


def pattern_name(pattern_id):
    return 'PATTERN_{}'.format(pattern_id)


# TODO(a-square): remove the duplicate in the protobuf_generator
def ref_name(ref):
    filename = ref.filename or ''
    filename = filename.replace(JSON_EXT, '').replace('.', '_').replace(os.sep, '_').replace('-', '_')
    path = '_'.join(map(str, ref.path))

    if path:
        full_path = filename + '_' + path
    else:
        full_path = filename

    return pascalize(shorten(full_path))


def adder_name(prop_name):
    sg_words = INFLECTOR.Inflect(prop_name, 'sg') or prop_name
    return 'Add' + pascalize(shorten(sg_words))


def shorten(name):
    return name.replace('properties_', '', 1).replace('definitions_', '', 1)
