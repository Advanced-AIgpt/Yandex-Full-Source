# coding: utf-8

from __future__ import print_function
import argparse
import codecs
import json
import sys

import alice.nlg.library.python.codegen_tool as codegen_tool


def parse_args():
    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers()

    render_parser = subparsers.add_parser('render', help='Renders one phrase or card directly from the file system')
    render_parser.add_argument('--phrase', help='Phrase to render')
    render_parser.add_argument('--card', help='Card to render')
    render_parser.add_argument('--import-dir', help='Directory that import statements should be relative from')
    render_parser.add_argument('--context', help='Rendering context JSON')
    render_parser.add_argument('--intent', help='Intent name to use, file name without extension by default')
    render_parser.add_argument('script', help='NLG script to render')
    render_parser.set_defaults(func=render_main)

    dump_ast_parser = subparsers.add_parser('dump-ast', help='Dumps NLG\'s internal representation.')
    dump_ast_parser.add_argument('--import-dir', help='Directory that import statements should be relative from')
    dump_ast_parser.add_argument('--out-file', help='File to write ast as json')
    dump_ast_parser.add_argument('script', help='NLG script to dump')
    dump_ast_parser.set_defaults(func=dump_ast_main)

    compile_py_parser = subparsers.add_parser('compile-py', help='Uses Jinja2\'s original compiler on the codegen_tool')
    compile_py_parser.add_argument('--import-dir', help='Directory that import statements should be relative from')
    compile_py_parser.add_argument('-o', '--output', help='File to write the compilation output to')
    compile_py_parser.add_argument('script', help='NLG script to compile')
    compile_py_parser.set_defaults(func=compile_py_main)

    dump_trans_ast_parser = subparsers.add_parser(
        'dump-trans-ast', help='Transforms the AST for code generation and dumps the result'
    )
    dump_trans_ast_parser.add_argument('--import-dir', help='Directory that import statements should be relative from')
    dump_trans_ast_parser.add_argument('--out-file', help='File to write ast as json')
    dump_trans_ast_parser.add_argument('script', help='NLG script to dump')
    dump_trans_ast_parser.set_defaults(func=dump_trans_ast_main)

    dump_keyset_parser = subparsers.add_parser('dump-keyset', help='Dumps keys from NLG\'s Templates')
    dump_keyset_parser.add_argument(
        '-I', '--import-dir', help='Directory that import statements should be relative from'
    )
    dump_keyset_parser.add_argument(
        '-o', '--out-file', required=True, help='File that the output NLG\'s parts should be written to'
    )
    dump_keyset_parser.add_argument(
        '-l', '--language', default=None, help='File that the output NLG\'s parts should be written to'
    )
    dump_keyset_parser.add_argument('script', help='Directiry or file from where including NLG as a source')
    dump_keyset_parser.set_defaults(func=dump_keyset)

    compile_cpp_parser = subparsers.add_parser(
        'compile-cpp', help='Transforms the AST for code generation and dumps the result'
    )
    compile_cpp_parser.add_argument(
        '--out-dir', required=True, help='Directory that the output C++ library should be written to'
    )
    compile_cpp_parser.add_argument(
        '--import-dir', required=True, help='Directory that import statements should be relative from'
    )
    compile_cpp_parser.add_argument(
        '--include-prefix', required=True, help='Prefix for include direcitves in the generated source'
    )
    compile_cpp_parser.add_argument(
        'script',
        nargs='+',
        help='NLG script to compile; you must specify all scripts that import each other for compilation to succeed',
    )
    compile_cpp_parser.add_argument(
        '--localized-mode', action='store_true', help='Generate cpp code that uses translations from external resource instead of direct phrase inlining'
    )
    compile_cpp_parser.set_defaults(func=compile_cpp_main)

    return parser.parse_args()


def render_main(cmd):
    if bool(cmd.phrase) == bool(cmd.card):
        raise ValueError('render accepts one of either --phrase or --card')

    context_value = json.loads(cmd.context) if cmd.context else dict()
    result = codegen_tool.render_template(
        cmd.script,
        import_dir=cmd.import_dir,
        phrase=cmd.phrase,
        card=cmd.card,
        context=context_value,
        intent=cmd.intent,
    )
    json.dump(result, sys.stdout, ensure_ascii=False, indent=4)
    print()


def dump_ast_main(cmd):
    with codecs.open(cmd.out_file, "w", encoding="utf-8") as f:
        json.dump(codegen_tool.dump_file_ast(cmd.script, cmd.import_dir), f, ensure_ascii=False, indent=4)


def compile_py_main(cmd):
    out_file = sys.stdout
    if cmd.output:
        out_file = open(cmd.output, 'w')

    codegen_tool.compile_py(cmd.script, out_file, cmd.import_dir)


def dump_trans_ast_main(cmd):
    with codecs.open(cmd.out_file, "w", encoding="utf-8") as f:
        json.dump(codegen_tool.dump_trans_ast(cmd.script, cmd.import_dir), f, ensure_ascii=False, indent=4)


# TODO: вынести в отдельный бинарь
def dump_keyset(cmd):
    codegen_tool.dump_keyset(cmd.import_dir, cmd.script, cmd.out_file, cmd.language)


def compile_cpp_main(cmd):
    codegen_tool.compile_cpp(cmd.script, cmd.out_dir, cmd.import_dir, cmd.include_prefix, cmd.localized_mode)


if __name__ == '__main__':
    args = parse_args()
    args.func(args)
