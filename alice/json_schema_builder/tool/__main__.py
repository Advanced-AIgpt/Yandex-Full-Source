import argparse
import sys

from alice.json_schema_builder.library.parser import JSONSchemaParser
from alice.json_schema_builder.library.cpp_generator import generate_cpp
from alice.json_schema_builder.library.protobuf_generator import generate_protobuf
from alice.json_schema_builder.library.util import dump_objects, dump_graphvis
from alice.library.tool_log import tool_log


def parse_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()  # XXX(a-square): required=True doesn't work so we work around it below

    inspect_parser = subparsers.add_parser(
        'inspect',
        help='Look at the JSON Schema in a given directory, see if we understand it',
    )
    inspect_parser.set_defaults(func=inspect_main)
    inspect_parser.add_argument('in_dir', help='JSON Schema directory')
    inspect_parser.add_argument('--dump-deps', action='store_true')
    inspect_parser.add_argument('--dump-knowns', action='store_true')
    inspect_parser.add_argument('--dump-raw-schemas', action='store_true')
    inspect_parser.add_argument('--dump-schemas', action='store_true')
    inspect_parser.add_argument('--dump-unknowns', action='store_true')
    inspect_parser.add_argument('--graphvis', action='store_true')

    generate_protobuf_parser = subparsers.add_parser(
        'generate-protobuf',
        help='Convert the JSON Schema in a given directory to a protobuf schema',
    )
    generate_protobuf_parser.set_defaults(func=generate_protobuf_main)
    generate_protobuf_parser.add_argument('in_dir', help='JSON Schema directory')
    generate_protobuf_parser.add_argument('out_path', help='Output path')
    generate_protobuf_parser.add_argument('--package', help='Protobuf package')
    generate_protobuf_parser.add_argument('--proto3', action='store_true', help='Use proto3 syntax')

    generate_cpp_parser = subparsers.add_parser(
        'generate-cpp',
        help='Convert the JSON Schema in a given directory to C++ classes',
    )
    generate_cpp_parser.set_defaults(func=generate_cpp_main)
    generate_cpp_parser.add_argument('in_dir', help='JSON Schema directory')
    generate_cpp_parser.add_argument('out_dir', help='Output directory')
    generate_cpp_parser.add_argument('--namespace', required=True, help='Namespace to put the generated code into')
    generate_cpp_parser.add_argument(
        '--clean-directory',
        action='store_true',
        help='Delete and recreate the output directory',
    )
    generate_cpp_parser.add_argument(
        '--generate-log-id-filler',
        action='store_true',
        help='Generate functions for filling log_id in the output JSON',
    )

    args = parser.parse_args()
    if not hasattr(args, 'func'):
        parser.parse_args(['--help'])  # XXX(a-square): work around required=True not working for subparsers
    return args


def inspect_main(cmd):
    parser = JSONSchemaParser()
    parser.scan_directory(cmd.in_dir)
    parser.parse_schemas()
    if cmd.dump_deps:
        deps = parser.calc_dependencies()
        if cmd.graphvis:
            dump_graphvis(deps)
            return

        print('Schema dependencies:')
        dump_objects(deps)
        print()
    if cmd.dump_knowns:
        print('Knowns:')
        dump_objects(parser.calc_knowns())
        print()
    if cmd.dump_raw_schemas:
        print('Raw schemas:')
        dump_objects(parser.calc_raw_schemas())
        print()
    if cmd.dump_schemas:
        print('Schemas:')
        dump_objects(parser.schemas)
        print()
    if cmd.dump_unknowns:
        print('Unknowns:')
        dump_objects(parser.calc_unknowns())
        print()


def generate_protobuf_main(cmd):
    parser = JSONSchemaParser()
    parser.scan_directory(cmd.in_dir)
    parser.parse_schemas()

    unknowns = parser.calc_unknowns()
    if unknowns:
        tool_log.error('Unknowns found')
        dump_objects(unknowns, out_file=sys.stderr)
        sys.stderr.write('\n')
        return 1

    generate_protobuf(parser.schemas, cmd.out_path, package=cmd.package, proto3=cmd.proto3)


def generate_cpp_main(cmd):
    parser = JSONSchemaParser()
    parser.scan_directory(cmd.in_dir)
    parser.parse_schemas()

    unknowns = parser.calc_unknowns()
    if unknowns:
        tool_log.error('Unknowns found')
        dump_objects(unknowns, out_file=sys.stderr)
        sys.stderr.write('\n')
        return 1

    generate_cpp(
        parser.schemas,
        cmd.out_dir,
        namespace=cmd.namespace,
        clean_directory=cmd.clean_directory,
        generate_log_id_filler=cmd.generate_log_id_filler,
    )


def main():
    args = parse_args()
    return args.func(args)


if __name__ == '__main__':
    sys.exit(main() or 0)
