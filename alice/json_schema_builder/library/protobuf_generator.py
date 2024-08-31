from alice.library.python.code_generator.code_generator import ProtobufCodeGenerator, pascalize
from alice.json_schema_builder.library import nodes, visitor
from collections import OrderedDict


JSON_EXT = '.json'


def generate_protobuf(schemas, out_path, package=None, proto3=False):
    if package is None:
        package = 'NAlice.NGenerated'

    with open(out_path, 'wb') as out_file:
        generate_protobuf_obj(schemas, ProtobufCodeGenerator(out_file), package, proto3)


def generate_protobuf_obj(schemas, cg, package, proto3):
    cg.banner(generator='alice/json_schema_builder/tool')
    if proto3:
        cg.syntax3()
    cg.imports('google/protobuf/struct.proto')

    resolver = FieldTypeResolver(schemas)

    # The order is important because we discover new types:
    # 1. objects go first
    # 2. then we write variant wrappers; they can't discover new objects
    #    because we've already written all the objects, but they can discover enums
    # 3. finally, enums, they cannot discover anything new
    for schema in schemas.values():
        if isinstance(schema, nodes.Object):
            write_schema(cg, schema, resolver)

    for variant in resolver.variants.values():
        write_variant(cg, variant, resolver)

    for enum in resolver.enums.values():
        write_enum(cg, enum)


def write_schema(cg, schema, resolver):
    with cg.message(message_name(schema.location)):
        field_num = 1
        for prop_name, prop in schema.properties.items():
            field_type, repeated = resolver.resolve(prop)
            required = prop_name in schema.required
            #  types_to_declare.update(find_types_to_declare(prop))
            cg.field(
                field_type,
                pascalize(prop_name),
                field_num,
                repeated=repeated,
                required=required,
                json_name=prop_name
            )
            field_num += 1


def write_enum(cg, enum):
    option_num = 0
    with cg.message(message_name(enum.location)):
        with cg.enum(enum_name(enum.location)):
            for option in enum.options:
                cg.enum_option(option, option_num)
                option_num += 1


def write_variant(cg, variant, resolver):
    # XXX(a-square): Unfixable (?) bug: wrapper message breaks the format
    with cg.message(message_name(variant.location)):
        with cg.oneof(ref_name(variant.location)):
            field_num = 1
            for alt_name in variant.alternatives:
                alt = resolver.schemas[alt_name]
                field_type, repeated = resolver.resolve(alt)
                cg.field(
                    field_type,
                    ref_name(alt_name),
                    field_num,
                    repeated=repeated,
                )
                field_num += 1


class FieldTypeResolver(visitor.NodeVisitor):
    def __init__(self, schemas):
        super().__init__()
        self.schemas = schemas
        self.enums = OrderedDict()
        self.variants = OrderedDict()

    def resolve(self, prop):
        return self.visit(prop)

    def visit_Ref(self, node):
        return self.visit(self.schemas[node])

    def visit_Builtin(self, node):
        fmt = node.fmt
        if fmt == 'boolean':
            return 'uint32', False  # Perl compatibility
        elif fmt in ('color', 'uri'):
            return 'string', False
        else:
            assert False, 'Unexpected built-in'

    def visit_Variant(self, node):
        # variants can't be repeated so we have to wrap them
        self.variants[node.location] = node
        return message_name(node.location), False

    def visit_JsonPayload(self, node):
        return 'map<string, google.protobuf.Value>', False

    def visit_Object(self, node):
        return message_name(node.location), False

    def visit_Array(self, node):
        return self.visit(node.items)[0], True

    def visit_Number(self, node):
        return 'double', False

    def visit_Integer(self, node):
        return 'int32', False

    def visit_String(self, node):
        return 'string', False

    def visit_StringEnum(self, node):
        self.enums[node.location] = node
        return message_name(node.location) + '.' + enum_name(node.location), False


def message_name(ref):
    return 'T' + ref_name(ref)


def enum_name(ref):
    return 'E' + ref_name(ref)


def ref_name(ref):
    filename = ref.filename or ''
    filename = filename.replace(JSON_EXT, '').replace('.', '_').replace('/', '_').replace('-', '_')
    path = '_'.join(map(str, ref.path))

    if path:
        full_path = filename + '_' + path
    else:
        full_path = filename

    return pascalize(full_path)
