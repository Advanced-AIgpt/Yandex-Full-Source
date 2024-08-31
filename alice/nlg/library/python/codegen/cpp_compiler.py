# coding: utf-8

from __future__ import unicode_literals

import itertools
import os

from alice.library.python.code_generator.code_generator import (
    CppCodeGenerator,
    camelize,
    pascalize,
    stringbuf,
)

from alice.library.tool_log import tool_log
from alice.nlg.library.python.codegen import (
    call,
    errors,
    nodes,
    transformer,
    visitor,
)
from collections import defaultdict
from contextlib import contextmanager
from vins_core.utils.data import ensure_dir


GENERATOR_NAME = 'alice/nlg'
RUNTIME_PREFIX = 'alice/nlg/library/runtime'
NLG_LIBRARY_REGISTRY_HEADER_PATH = 'alice/nlg/library/runtime_api/nlg_library_registry.h'
OPERATORS_NAMESPACE = '::NAlice::NNlg::NOperators'
BUILTINS_NAMESPACE = '::NAlice::NNlg::NBuiltins'
MANDATORY_CALLER_PARAMS = [
    '[[maybe_unused]] IOutputStream& out',
]
MANDATORY_MACRO_PARAMS = [
    'const ::NAlice::NNlg::TCallCtx& ctx',
    '[[maybe_unused]] const ::NAlice::NNlg::TCaller* caller',
    'const ::NAlice::NNlg::TGlobalsChain* globalsChain',
    '[[maybe_unused]] IOutputStream& out',
]

BINARY_OPERATORS = {
    # operators (except `and` and `or`)
    '*': OPERATORS_NAMESPACE + '::Mul',
    '/': OPERATORS_NAMESPACE + '::TrueDiv',
    '//': OPERATORS_NAMESPACE + '::FloorDiv',
    '**': OPERATORS_NAMESPACE + '::Pow',
    '%': OPERATORS_NAMESPACE + '::Mod',
    '+': OPERATORS_NAMESPACE + '::Add',
    '-': OPERATORS_NAMESPACE + '::Sub',
    'and': OPERATORS_NAMESPACE + '::And',
    'or': OPERATORS_NAMESPACE + '::Or',
    # comparisons
    'eq': OPERATORS_NAMESPACE + '::Equals',
    'ne': OPERATORS_NAMESPACE + '::NotEquals',
    'gt': OPERATORS_NAMESPACE + '::Greater',
    'gteq': OPERATORS_NAMESPACE + '::GreaterEq',
    'lt': OPERATORS_NAMESPACE + '::Less',
    'lteq': OPERATORS_NAMESPACE + '::LessEq',
    'in': OPERATORS_NAMESPACE + '::ValueIn',
    'notin': OPERATORS_NAMESPACE + '::ValueNotIn',
}

UNARY_OPERATORS = {
    'not': OPERATORS_NAMESPACE + '::UnaryNot',
    '-': OPERATORS_NAMESPACE + '::UnaryMinus',
    '+': OPERATORS_NAMESPACE + '::UnaryPlus',
}


def _const(value):
    if isinstance(value, list):
        cls = nodes.List
    elif isinstance(value, dict):
        cls = nodes.Dict
        value = [nodes.Pair(_const(key), _const(val), lines=[]) for key, val in value.iteritems()]
    else:
        cls = nodes.Const

    return cls(value, lines=[])


# NOTE(a-square): defaults should specify (empty) lines,
# the default lines=None results in a warning intended to catch
# nodes that should've had lines but don't
BUILTIN_PARAMS = {
    # filters
    'abs': (['target'], []),
    'attr': (['target', 'name'], []),
    'capitalize': (['target'], []),
    'capitalize_first': (['target'], []),
    'ceil_seconds': (['time_units', 'aggressive'], [_const(True)]),
    'city_prepcase': (['target'], []),
    'decapitalize': (['target'], []),
    'decapitalize_first': (['target'], []),
    'default': (['target', 'default_value', 'boolean'], [_const(''), _const(False)]),
    'div2_escape': (['target'], []),
    'emojize': (['target'], []),
    'first': (['target'], []),
    'float': (['target', 'default'], [_const(0.0)]),
    'format_weekday': (['target'], []),
    'get_item': (['target', 'key', 'default'], [_const('')]),
    'html_escape': (['target'], []),
    'human_time': (['target', 'timezone'], [_const(None)]),
    'human_time_raw': (['target'], []),
    'human_date': (['target', 'timezone'], [_const(None)]),
    'human_day_rel': (['target', 'timezone', 'mocked_time'], [_const(None), _const(None)]),
    'is_human_day_rel': (['target', 'timezone', 'mocked_time'], [_const(None), _const(None)]),
    'human_month': (['target', 'grams'], [_const(None)]),
    'inflect': (['target', 'cases', 'fio'], [_const(False)]),
    'int': (['target', 'default'], [_const(0)]),
    'join': (['target', 'd', 'attribute'], [_const(' '), _const(None)]),
    'last': (['target'], []),
    'length': (['target'], []),
    'list': (['target'], []),
    'lower': (['target'], []),
    'map': (['target', 'filter', 'attribute'], [_const(None), _const(None)]),
    'max': (['target'], []),
    'min': (['target'], []),
    'music_title_shorten': (['target'], []),
    'normalize_time_units': (['time_units'], []),
    'number_of_readable_tokens': (['target'], []),
    'only_text': (['target'], []),
    'only_voice': (['target'], []),
    'parse_dt': (['target'], []),
    'pluralize': (['target', 'number', 'case'], [_const('nomn')]),
    'pluralize_tag': (['target', 'case'], [_const('nomn')]),
    'random': (['target'], []),
    'render_weekday_type': (['weekday'], []),
    'render_weekday_simple': (['weekday'], []),
    'render_datetime_raw': (['dt'], []),
    'render_date_with_on_preposition': (['dt'], []),
    'render_units_time': (['units', 'cases'], [_const('acc')]),
    'replace': (['target', 'old', 'new'], []),
    'round': (['target', 'precision', 'method'], [_const(0), _const(None)]),
    'singularize': (['target', 'number'], []),
    'split_big_number': (['target'], []),
    'string': (['target'], []),
    'time_format': (['time', 'cases'], [_const('nomn')]),
    'to_json': (['target'], []),
    'trim': (['target'], []),
    'trim_with_ellipsis': (['target', 'width_limit'], [_const(20)]),
    'tts_domain': (['target', 'domain'], []),
    'upper': (['target'], []),
    'urlencode': (['target'], []),
    # functions
    # TODO(a-square): assert that they're the same built-in functions as marked in transformer.py
    'add_hours': (['datetime', 'hours'], [_const(0)]),
    'client_action_directive': (
        ['name', 'payload', 'type', 'sub_name'],
        [_const({}), _const('client_action'), _const(None)],
    ),
    'create_date_safe': (['year', 'month', 'day'], []),
    'datetime': (
        ['year', 'month', 'day', 'hour', 'minute', 'second', 'microsecond'],
        [_const(0), _const(0), _const(0), _const(0)],
    ),
    'datetime_strptime': (['date_string', 'date_format'], []),
    'parse_tz': (['timezone_format'], []),
    'randuniform': (['from', 'to'], []),
    'server_action_directive': (
        ['name', 'payload', 'type', 'ignore_answer'],
        [_const({}), _const('server_action'), _const(False)],
    ),
    'timestamp_to_datetime': (['timestamp', 'timezone'], [_const('UTC')]),
    # methods
    'datetime_strftime': (['self', 'datetime_format'], []),
    'datetime_isoweekday': (['self'], []),
    'dict_get': (['self', 'k', 'd'], [_const(None)]),
    'dict_items': (['self'], []),
    'dict_keys': (['self'], []),
    'dict_update': (['self', 'E'], []),
    'dict_values': (['self'], []),
    'list_append': (['self', 'object'], []),
    'localize': (['self', 'datetime'], []),
    'str_ends_with': (['self', 'suffix'], []),
    'str_join': (['self', 'iterable'], []),
    'str_lower': (['self'], []),
    'str_lstrip': (['self', 'chars'], [_const(None)]),
    'str_replace': (['self', 'old', 'new'], []),
    'str_rstrip': (['self', 'chars'], [_const(None)]),
    'str_split': (['self', 'sep', 'maxsplit'], [_const(None), _const(None)]),
    'str_starts_with': (['self', 'prefix'], []),
    'str_strip': (['self', 'chars'], [_const(None)]),
    'str_upper': (['self'], []),
    # tests
    'test_defined': (['target'], []),
    'test_undefined': (['target'], []),
    'test_divisible_by': (['target', 'num'], []),
    'test_even': (['target'], []),
    'test_odd': (['target'], []),
    'test_eq': (['target', 'other'], []),
    'test_ge': (['target', 'other'], []),
    'test_gt': (['target', 'other'], []),
    'test_in': (['target', 'other'], []),
    'test_le': (['target', 'other'], []),
    'test_lt': (['target', 'other'], []),
    'test_ne': (['target', 'other'], []),
    'test_iterable': (['target'], []),
    'test_lower': (['target'], []),
    'test_upper': (['target'], []),
    'test_mapping': (['target'], []),
    'test_none': (['target'], []),
    'test_number': (['target'], []),
    'test_sequence': (['target'], []),
    'test_string': (['target'], []),
    # macros
    'macro__choose_line': ([], []),
}

assert sorted(transformer.BUILTIN_MACROS) == sorted(
    key for key in BUILTIN_PARAMS.iterkeys() if key.startswith('macro__')
)

LOOP_ATTRS = {
    'index': '::NAlice::NNlg::TValue::Integer(loopIdx + 1)',
    'index0': '::NAlice::NNlg::TValue::Integer(loopIdx)',
    'revindex': '::NAlice::NNlg::TValue::Integer(loopLength - loopIdx)',
    'revindex0': '::NAlice::NNlg::TValue::Integer(loopLength - loopIdx - 1)',
    'first': '::NAlice::NNlg::TValue::Bool(loopIdx == 0)',
    'last': '::NAlice::NNlg::TValue::Bool(loopIdx + 1 == loopLength)',
    'length': '::NAlice::NNlg::TValue::Integer(loopLength)',
    'previtem': 'loopPrevItem',
    'nextitem': 'loopNextItem',
}


def compile_cpp(library, out_dir, include_prefix, localized_mode):
    assert isinstance(library, nodes.Library)
    ensure_dir(out_dir)

    modules = gather_modules(library)
    module_macros = gather_module_macros(library)
    foreign_modules = gather_foreign_modules(library)
    foreign_macros = gather_foreign_macros(library)

    for module in library.modules:
        nlg_filename = module.path
        ensure_dir(os.path.dirname(os.path.join(out_dir, module.path)))
        with open(source_h_path(module.path, out_dir), 'w') as h_file:
            generate_h(module, h_file, nlg_filename)

        with open(source_cpp_path(module.path, out_dir), 'w') as cpp_file:
            generate_cpp(module, cpp_file, modules, module_macros, foreign_modules, foreign_macros, localized_mode)

    reg_dir = os.path.join(out_dir, include_prefix)
    ensure_dir(reg_dir)
    with open(os.path.join(reg_dir, 'register.h'), 'w') as reg_h_file:
        generate_register_header(reg_h_file, include_prefix)

    with open(os.path.join(reg_dir, 'register.cpp'), 'w') as reg_cpp_file:
        generate_register_source(library, reg_cpp_file, include_prefix)


def generate_h(module, fobj, nlg_filename):
    visitor = GenerateHeaderVisitor(fobj, nlg_filename)
    visitor.visit(module)


def generate_cpp(module, fobj, modules, module_macros, foreign_modules, foreign_macros, localized_mode):
    if localized_mode:
        visitor = GenerateLocalizedSourceVisitor(fobj, modules, module_macros, foreign_modules, foreign_macros)
    else:
        visitor = GenerateSourceVisitor(fobj, modules, module_macros, foreign_modules, foreign_macros)

    try:
        visitor.visit(module)
    except Exception as exc:
        errors.TemplateCompilationError.raise_wrapped(exc, visitor.filename, visitor.lineno)


def generate_register_header(fobj, include_prefix):
    cg = CppCodeGenerator(fobj)

    cg.pragma('once')
    cg.skip_line()
    cg.banner(generator=GENERATOR_NAME)
    cg.skip_line()
    cg.include(os.path.join(RUNTIME_PREFIX, 'runtime.h'), system=True)
    cg.skip_line()

    with cg.namespace(gen_namespace(include_prefix)):
        cg.write_line('void RegisterAll(::NAlice::NNlg::TEnvironment& env);')
        cg.skip_line()
        cg.write_line('inline constexpr TStringBuf LIBRARY_PATH = {path};'.format(path=stringbuf(include_prefix)))


def generate_register_source(library, fobj, include_prefix):
    cg = CppCodeGenerator(fobj)

    cg.banner(generator=GENERATOR_NAME)
    cg.skip_line()

    cg.include('register.h', system=False)
    cg.skip_line()

    cg.include(NLG_LIBRARY_REGISTRY_HEADER_PATH, system=True)
    cg.skip_line()

    for module in library.modules:
        path = source_h_path(module.path, dir_name='')
        cg.include(path, system=True)
    cg.skip_line()
    for module in library.foreign_modules:
        path = source_h_path(module.path, dir_name='')
        cg.include(path, system=True)
    cg.skip_line()

    with cg.namespace(gen_namespace(include_prefix)):
        with cg.func_def('void RegisterAll(::NAlice::NNlg::TEnvironment& env)'):
            for module in library.modules:
                cg.write_line('{}::Register(env);'.format(gen_namespace(module.path)))
            for module in library.foreign_modules:
                cg.write_line('{}::Register(env);'.format(gen_namespace(module.path)))
        cg.skip_line()
        cg.write_line('REGISTER_NLG_LIBRARY(LIBRARY_PATH, RegisterAll);')


class GenerateHeaderVisitor(visitor.NodeVisitor):
    permissive = True

    def __init__(self, fobj, nlg_filename):
        self._cg = CppCodeGenerator(fobj)
        self._nlg_filename = nlg_filename

    def visit_Module(self, node):
        cg = self._cg

        cg.pragma('once')
        cg.skip_line()
        cg.banner(generator=GENERATOR_NAME, source_path=node.path)
        cg.skip_line()
        cg.include(os.path.join(RUNTIME_PREFIX, 'runtime.h'), system=True)
        cg.skip_line()

        with cg.namespace(gen_namespace(node.path)):
            for macro in node.macros:
                self.visit(macro)

            cg.skip_line()
            cg.write_line(
                'void InitGlobals(const ::NAlice::NNlg::TMutableCallCtx& ctx, ::NAlice::NNlg::TGlobals& globals);'
            )
            cg.write_line('void Register(::NAlice::NNlg::TEnvironment& env);')
            cg.skip_line()
            cg.write_line('inline constexpr size_t NUM_LINES = {};'.format(node.total_lines))
            cg.write_line('inline constexpr TStringBuf MODULE_PATH = {path};'.format(path=stringbuf(node.path)))
            cg.write_line(
                'inline constexpr TStringBuf NLG_FILENAME = {filename};'.format(filename=stringbuf(self._nlg_filename))
            )

    def visit_Macro(self, node):
        self._cg.write_line(
            'void {name}({params});'.format(
                name=gen_macro(node.name),
                params=callable_params(node, MANDATORY_MACRO_PARAMS),
            )
        )


class GenerateSourceVisitor(visitor.NodeVisitor):
    """Generates C++ code from a transformed AST.

    Statement nodes output code, expression nodes return code
    to be combined by other nodes into more complex expressions.
    """

    def __init__(self, fobj, modules, module_macros, foreign_modules, foreign_macros):
        self._cg = CppCodeGenerator(fobj)
        self._modules = modules
        self._module_macros = module_macros
        self._foreign_modules = foreign_modules
        self._foreign_macros = foreign_macros
        self._tmp_id_number = 0
        self._current_module_path = None
        self.lineno = '??'

    @property
    def filename(self):
        """File name that will be passed to any exception that leaves the visitor."""
        return self._current_module_path or '<unknown>'

    def alter_result(self, node, result):
        if node.has_coverage:
            if not node.lines:
                if node.lines is None:
                    tool_log.warning('Empty lines: file = {}, node = {!r}\n'.format(self.filename, node))
                return result
            return '({}, {})'.format(coverage(node.lines), result)
        return result

    def pre_visit(self, node):
        self.lineno = node.lines[0] if node.lines else '??'

    def visit_Module(self, node):
        self._current_module_path = node.path
        cg = self._cg

        cg.banner(generator=GENERATOR_NAME, source_path=node.path)
        cg.skip_line()
        cg.include(source_h_path(node.path, dir_name=''), system=True)
        cg.include(RUNTIME_PREFIX + '/coverage.h', system=True)
        cg.skip_line()

        nlgimports = self._collect_nlgimports(node)

        imports = set(import_node.path for import_node in node.imports)
        for import_path, macro in nlgimports:
            imports.add(import_path)

        for import_path in sorted(imports):
            cg.include(source_h_path(import_path, dir_name=''), system=True)
        cg.skip_line()

        with cg.namespace(gen_namespace(node.path)):
            for macro in node.macros:
                self.visit(macro)

            with cg.func_def(
                'void InitGlobals([[maybe_unused]] const ::NAlice::NNlg::TMutableCallCtx& ctx, '
                '[[maybe_unused]] ::NAlice::NNlg::TGlobals& globals)'
            ):
                with stack_frame(cg, '<module init>', node.lines):
                    if node.imports:
                        for stmt in node.imports:
                            if isinstance(stmt, nodes.Import):
                                path_str = stringbuf(stmt.path)
                                cg.write_line(
                                    'globals.RegisterImport({}, ctx.Env.InitializeGlobals(ctx, {}));'.format(
                                        path_str,
                                        path_str,
                                    )
                                )
                            elif isinstance(stmt, nodes.FromImport):
                                for name_spec in stmt.names:
                                    if isinstance(name_spec, tuple):
                                        name, target = name_spec
                                    else:
                                        name = target = name_spec
                                    cg.write_line(
                                        'globals.RegisterFromImport({}, {}, ctx.Env.InitializeGlobals(ctx, {}));'.format(
                                            stringbuf(name),
                                            stringbuf(target),
                                            stringbuf(stmt.path),
                                        )
                                    )

                        cg.skip_line()

                    for stmt in node.init:
                        self.visit(stmt)

            with cg.func_def('void Register(::NAlice::NNlg::TEnvironment& env)'):

                def register_macro(module_path, macro):
                    if macro.phrase:
                        func_name = 'RegisterPhrase'
                        name = macro.phrase
                    elif macro.card:
                        func_name = 'RegisterCard'
                        name = macro.card
                        pass
                    else:
                        return

                    if module_path is None:
                        macro_identifier = gen_macro(macro.name)
                    else:
                        # TODO(a-square): refactor so that gen_macro accepts module_path?
                        macro_identifier = '{}::{}'.format(gen_namespace(module_path), gen_macro(macro.name))

                    cg.write_line(
                        'env.{func_name}({intent}, {name}, &{macro});'.format(
                            func_name=func_name,
                            intent=stringbuf(node.intent),
                            name=stringbuf(name),
                            macro=macro_identifier,
                        )
                    )

                for import_path, macro in nlgimports:
                    register_macro(import_path, macro)
                cg.skip_line()

                for macro in node.macros:
                    register_macro(None, macro)
                cg.skip_line()

                cg.write_line('env.RegisterInit(MODULE_PATH, &InitGlobals);')

                coverage_segments = []
                for coverage_segment in node.coverage_segments:
                    coverage_segments.append('{' + ', '.join(map(str, coverage_segment)) + '}')
                coverage_segments_inititializer_list = '{' + ', '.join(coverage_segments) + '}'
                cg.write_line(
                    'TVector<::NAlice::NNlg::TSegment> segments{};'.format(coverage_segments_inititializer_list)
                )
                cg.write_line('NLG_REGISTER(segments);')

    def visit_Macro(self, node):
        cg = self._cg

        with cg.func_def(
            'void {name}({params})'.format(
                name=gen_macro(node.name),
                params=callable_params(node, MANDATORY_MACRO_PARAMS),
            )
        ):
            with stack_frame(cg, node.name, node.lines):
                cg.write_line(coverage(node.lines) + ';')
                cg.write_line(
                    '[[maybe_unused]] ::NAlice::NNlg::TGlobalsChain globals(globalsChain, '
                    '&ctx.Env.GetGlobals(MODULE_PATH));'
                )
                cg.skip_line()

                declare_locals(cg, node.scope)

                for child in node.body:
                    self.visit(child)

    def visit_Output(self, node):
        for child in node.nodes:
            self._visit_output_child(child)

    def _visit_output_child(self, node):
        result = self.visit(node)
        if getattr(node, 'direct_output', False):
            self._cg.write_line(result + ';')
        else:
            self._cg.write_line('out << {};'.format(result))

    def visit_ExprStmt(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')
        for child in node.nodes:
            # TemplateData can only be ExprStmt's direct child at the module's top level,
            # so we skip it to clean up coverage data
            if isinstance(child, nodes.TemplateData):
                continue
            cg.write_line('(void){};'.format(self.visit(child)))

    def visit_CallBlock(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')

        caller_name = self._tmp_identifier('caller')
        cg.write_line(
            '::NAlice::NNlg::TCaller {caller_name}(::NAlice::NNlg::TCaller::TCaller{num_params}([&]({params}) {{'.format(
                caller_name=caller_name,
                num_params=len(node.args),
                params=callable_params(node, MANDATORY_CALLER_PARAMS),
            )
        )
        with cg.indent():
            with stack_frame(cg, '<caller>', node.lines):
                declare_locals(cg, node.scope)
                for stmt in node.body:
                    self.visit(stmt)
        cg.write_line('}));')

        cg.write_line(self.visit(node.call, caller=caller_name) + ';')

    def visit_Call(self, node, caller=None):
        if isinstance(node.target, nodes.Caller):
            func_name = '::NAlice::NNlg::InvokeCaller'
            args = ['caller', 'out'] + map(self.visit, node.args)
        else:
            func_name = self.visit(node.target)
            args = [
                'ctx',
                '/* caller = */ nullptr' if caller is None else '&' + caller,
                '::NAlice::NNlg::WrapGlobals(&globals)' if node.target.with_context else '/* globalsChain = */ nullptr',
                'out',
            ]
            args += map(self.visit, self._resolve_call_args(node))

        call_line = '{func_name}({args})'.format(
            func_name=func_name,
            args=', '.join(args),
        )

        if node.direct_output:
            return (
                '::NAlice::NNlg::RunMacroDirectOutput(out, ' '[&]([[maybe_unused]] IOutputStream& out) {{ {}; }})'
            ).format(call_line)
        elif node.suppress_output:
            return ('::NAlice::NNlg::RunMacroNoResult([&]([[maybe_unused]] IOutputStream& out) {{ {}; }})').format(
                call_line
            )
        else:
            return '::NAlice::NNlg::GetMacroResult([&]([[maybe_unused]] IOutputStream& out) {{ {}; }})'.format(
                call_line
            )

    def visit_Assign(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')
        target = node.target
        if isinstance(target, nodes.List):
            tmp_id = self._tmp_identifier('tuple')
            cg.write_line(
                'auto {tmp_id} = {rvalue};'.format(
                    tmp_id=tmp_id,
                    rvalue=self.visit(node.node),
                )
            )
            for index, item in enumerate(target.items):
                cg.write_line(
                    '{lvalue} = ::NAlice::NNlg::GetItemLoadInt({tmp_id}, {index});'.format(
                        lvalue=self.visit(item),
                        tmp_id=tmp_id,
                        index=index,
                    )
                )
        else:
            # XXX(a-square): assigning a variable to itself is legal,
            # but clang normally gives a warning (as it should, really)
            with cg.disable_warning("-Wself-assign-overloaded"):
                cg.write_line(
                    '{lvalue} = {rvalue};'.format(
                        lvalue=self.visit(target),
                        rvalue=self.visit(node.node),
                    )
                )

    def visit_If(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')

        cg.write_line('if (::NAlice::NNlg::TruthValue({})) {{'.format(self.visit(node.test)))
        with cg.indent():
            for stmt in node.then:
                self.visit(stmt)
        for else_if in node.else_ifs:
            cg.write_line('}} else if (::NAlice::NNlg::TruthValue({})) {{'.format(self.visit(else_if.test)))
            with cg.indent():
                for stmt in else_if.then:
                    self.visit(stmt)
        if node.otherwise:
            cg.write_line('} else {')
            with cg.indent():
                for stmt in node.otherwise:
                    self.visit(stmt)
        cg.write_line('}')

    def visit_For(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')
        cg.write_line('do {')
        with cg.indent():
            cg.write_line('auto loopCollection = {};'.format(self.visit(node.collection)))
            cg.skip_line()

            cg.write_line('auto loopLength = ::NAlice::NNlg::GetLength(loopCollection);')
            cg.write_line('if (loopLength == 0) {')
            with cg.indent():
                declare_locals(cg, node.otherwise_scope)
                for stmt in node.otherwise:
                    self.visit(stmt)

                cg.skip_line()
                cg.write_line('break;')  # breaks out of the do { ... } while loop
            cg.write_line('}')
            cg.skip_line()

            target = node.target

            if isinstance(target, nodes.LocalName):
                item_name = gen_local_name(target.name, target.counter)
                item_names = []
            else:
                assert isinstance(target, nodes.List)
                item_name = self._tmp_identifier('loopCurrItem')
                item_names = []
                for item in target.items:
                    item_names.append(gen_local_name(item.name, item.counter))

            cg.write_line('auto loopPrevItem = ::NAlice::NNlg::TValue::Undefined();')
            cg.write_line('auto loopNextItem = ::NAlice::NNlg::TValue::Undefined();')
            cg.skip_line()

            cg.write_line('auto it = loopCollection.begin();')

            cg.write_line('for (i64 loopIdx = 0; loopIdx < loopLength; ++loopIdx) {')
            with cg.indent():
                cg.write_line('auto nextIt = it;')
                cg.write_line(
                    'loopNextItem = (loopIdx + 1 < loopLength) ? *(++nextIt) : ::NAlice::NNlg::TValue::Undefined();'
                )
                cg.skip_line()

                declare_locals(cg, node.body_scope)

                if item_names:
                    cg.write_line('auto {} = *it;'.format(item_name))
                else:
                    cg.write_line('{} = *it;'.format(item_name))

                for i, name in enumerate(item_names):
                    cg.write_line(
                        '{name} = ::NAlice::NNlg::GetItemLoadInt({item_name}, {i});'.format(
                            name=name,
                            item_name=item_name,
                            i=i,
                        )
                    )
                cg.skip_line()

                for stmt in node.body:
                    self.visit(stmt)

                cg.skip_line()
                cg.write_line('loopPrevItem = *it;')
                cg.write_line('it = nextIt;')
            cg.write_line('}')
        cg.write_line('} while(0);')

    def visit_LoopAttr(self, node):
        return LOOP_ATTRS[node.attr]

    def visit_Scope(self, node):
        cg = self._cg

        # standard do { ... } while(0); idiom to avoid accidentally
        # breaking some nested if-then-else blocks
        # (just a precaution, we put them all in curly braces anyway)
        cg.write_line('do {')
        with cg.indent():
            declare_locals(cg, node.scope)
            for stmt in node.body:
                self.visit(stmt)
        cg.write_line('} while(0);')

    def visit_TextFlag(self, node):
        # our flags are additive: TText::EFlag::Voice means voice is enabled and so on;
        # therefore, the {% voice %} tag should clear the text flag etc
        return '{action}({flag})'.format(
            action='::NAlice::NNlg::ClearFlag' if node.action == node.ACTION_BEGIN else '::NAlice::NNlg::SetFlag',
            flag='::NAlice::NNlg::TText::EFlag::' + ('Voice' if node.flag == node.FLAG_TEXT else 'Text'),
        )

    def visit_TemplateData(self, node):
        return stringbuf(node.data)

    def visit_ChooselineTemplateData(self, node):
        return stringbuf(node.data)

    def visit_BinaryOp(self, node):
        op_func = BINARY_OPERATORS.get(node.op)
        assert op_func is not None

        lhs = self.visit(node.lhs)
        rhs = self.visit(node.rhs)
        if node.op in ('and', 'or'):
            rhs = '[&]() { return %s; }' % rhs

        return '::NAlice::NNlg::InvokeWithStackFrame<{op_func}>(ctx, {name}, {lhs}, {rhs})'.format(
            op_func=op_func,
            name=stringbuf('<operator {}>'.format(node.op)),
            lhs=lhs,
            rhs=rhs,
        )

    def visit_UnaryOp(self, node):
        op_func = UNARY_OPERATORS.get(node.op)
        assert op_func is not None
        return '::NAlice::NNlg::InvokeWithStackFrame<{op_func}>(ctx, {name}, {target})'.format(
            op_func=op_func,
            name=stringbuf('<operator {}>'.format(node.op)),
            target=self.visit(node.target),
        )

    def visit_Concat(self, node):
        return '{func}([&](::NAlice::NNlg::TTextOutput& out) {{ out << {operands}; }})'.format(
            func=OPERATORS_NAMESPACE + '::Concat',
            operands=' << '.join(map(self.visit, node.nodes)),
        )

    def visit_CondExpr(self, node):
        return '::NAlice::NNlg::TruthValue({test}) ? {then} : {otherwise}'.format(
            test=self.visit(node.test),
            then=self.visit(node.then),
            otherwise=self.visit(node.otherwise)
            if node.otherwise is not None
            else '::NAlice::NNlg::TValue::Undefined()',
        )

    def visit_Getattr(self, node):
        func_name = '::NAlice::NNlg::GetAttrLoad' if node.ctx == 'load' else '::NAlice::NNlg::GetAttrStore'
        return '::NAlice::NNlg::InvokeWithStackFrame<{func_name}>(ctx, {name}, {target}, {attr})'.format(
            func_name=func_name,
            name=stringbuf('<{}>'.format(func_name)),
            target=self.visit(node.node),
            attr=stringbuf(node.attr),
        )

    def visit_GetListItem(self, node):
        return '::NAlice::NNlg::InvokeWithStackFrame<::NAlice::NNlg::GetItemLoadInt>(ctx, {name}, {target}, {index}L)'.format(
            name=stringbuf('<GetItemLoadInt>'),
            target=self.visit(node.node),
            index=node.index,
        )

    def visit_Getitem(self, node):
        return '::NAlice::NNlg::InvokeWithStackFrame<::NAlice::NNlg::GetItemLoad>(ctx, {name}, {target}, {arg})'.format(
            name=stringbuf('<GetItemLoad>'),
            target=self.visit(node.node),
            arg=self.visit(node.arg),
        )

    def visit_SliceList(self, node):
        return (
            '::NAlice::NNlg::InvokeWithStackFrame<{func_name}>(ctx, {name}, {target}, {start}, {stop}, {step})'.format(
                func_name=OPERATORS_NAMESPACE + '::SliceList',
                name=stringbuf('<slice>'),
                target=self.visit(node.node),
                start=self.visit(node.start),
                stop=self.visit(node.stop),
                step=self.visit(node.step),
            )
        )

    def visit_GlobalName(self, node):
        func_name = 'ResolveLoad' if node.ctx == 'load' else 'ResolveStore'
        args = [stringbuf(node.name)]
        if node.module_path is not None:
            args.append(stringbuf(node.module_path))

        return 'globals.{func_name}({args})'.format(func_name=func_name, args=', '.join(args))

    def visit_LocalName(self, node):
        return gen_local_name(node.name, node.counter)

    def visit_BuiltinName(self, node):
        return BUILTINS_NAMESPACE + '::' + pascalize(node.name)

    def visit_MacroName(self, node):
        macro_name = gen_macro(node.name)
        if node.module_path:
            return '{}::{}'.format(gen_namespace(node.module_path), macro_name)
        else:
            return macro_name

    def visit_Builtin(self, node):
        if node.name not in BUILTIN_PARAMS:
            raise errors.UnknownBuiltinError('Unknown builtin: {!r}'.format(node.name))

        params, defaults = BUILTIN_PARAMS[node.name]
        args = call.resolve_param_values(node.name, params, defaults, node.args, node.kwargs)
        all_builtin_args = ['ctx', '::NAlice::NNlg::WrapGlobals(&globals)'] + list(map(self.visit, args))
        return ('::NAlice::NNlg::InvokeWithStackFrame<{func_name}>(ctx, {name}, {all_builtin_args})').format(
            func_name=gen_builtin(node.name),
            name=stringbuf('<{}>'.format(node.name)),
            all_builtin_args=', '.join(all_builtin_args),
        )

    def visit_CaptureBlock(self, node):
        cg = self._cg

        # XXX(a-square): expression nodes don't usually write any code directly,
        # this may blow up unless you pay attention to use cases
        # (currently AssignBlock & FilterBlock)
        block_name = self._tmp_identifier('block')
        cg.write_line('auto {} = [&]([[maybe_unused]] IOutputStream& out) {{'.format(block_name))
        with cg.indent():
            with stack_frame(cg, '<capture block>', node.lines):
                for stmt in node.body:
                    self.visit(stmt)
        cg.write_line('};')

        return '::NAlice::NNlg::GetMacroResult({})'.format(block_name)

    def visit_Const(self, node):
        value = node.value
        assert isinstance(value, (bool, int, long, float, basestring, type(None)))
        if isinstance(value, bool):
            return '::NAlice::NNlg::TValue::Bool({})'.format("true" if value else "false")
        elif isinstance(value, (int, long)):
            return '::NAlice::NNlg::TValue::Integer({}L)'.format(value)
        elif isinstance(value, float):
            return '::NAlice::NNlg::TValue::Double({})'.format(value)
        elif isinstance(value, basestring):
            return '::NAlice::NNlg::TValue::String({})'.format(stringbuf(value))
        else:  # value is None
            return '::NAlice::NNlg::TValue::None()'

    def visit_List(self, node):
        return '::NAlice::NNlg::TValue::List(::NAlice::NNlg::TValue::TList({%s}))' % ', '.join(
            itertools.imap(self.visit, node.items)
        )

    def visit_Dict(self, node):
        # generators carry large overhead and dict literals
        # tend to be very small so putting pairs on a list
        # should work out better
        pairs = []
        for item in node.items:
            assert isinstance(item, nodes.Pair)
            if isinstance(item.key, nodes.Const):
                # the most likely case, no need to make a temporary TValue here
                key = 'TString{%s}' % stringbuf(item.key.value)
            else:
                key = '({}).GetString()'.format(self.visit(item.key))
            value = self.visit(item.value)
            pairs.append((key, value))

        return '::NAlice::NNlg::TValue::Dict(::NAlice::NNlg::TValue::TDict({%s}))' % ', '.join(
            '{%s, %s}' % (key, value) for key, value in pairs
        )

    def visit_Pair(self, node):
        # we deal with `nodes.Pair` directly in `visit_Dict`
        assert False, 'Encountered a Pair outside of a Dict literal'

    def visit_Range(self, node):
        return '::NAlice::NNlg::TValue::Range({{{start}, {stop}, {step}}})'.format(
            start='({}).GetInteger()'.format(self.visit(node.start)),
            stop='({}).GetInteger()'.format(self.visit(node.stop)),
            step='({}).GetInteger()'.format(self.visit(node.step)),
        )

    def _resolve_call_args(self, node):
        assert isinstance(node, nodes.Call)
        target = node.target
        assert isinstance(target, (nodes.BuiltinName, nodes.MacroName))

        if isinstance(target, nodes.BuiltinName):
            macro_args, macro_defaults = BUILTIN_PARAMS[target.name]
        elif isinstance(target, nodes.MacroName):
            module_path = target.module_path
            if module_path is None:
                module_path = self._current_module_path

            module = self._module_macros.get(module_path) or self._foreign_macros.get(module_path)
            if module is None:
                raise errors.CallResolutionError('Unknown module path: {!r}'.format(target.module_path))

            if target.name not in module:
                raise errors.CallResolutionError(
                    'Macro {!r} not found in module {!r}'.format(
                        target.name,
                        target.module_path,
                    )
                )

            macro = module[target.name]
            macro_args = [arg.name for arg in macro.args]
            macro_defaults = []
            for default in macro.defaults:
                resolve_call_default_lines(default, node.lines)
                macro_defaults.append(default)

        return call.resolve_param_values(
            target.name,
            macro_args,
            macro_defaults,
            node.args,
            node.kwargs,
        )

    def _tmp_identifier(self, kind):
        result = 'nlg__{number}_{kind}'.format(number=self._tmp_id_number, kind=kind)
        self._tmp_id_number += 1
        return result

    def _collect_nlgimports(self, node):
        assert isinstance(node, nodes.Module)

        result = []

        def process_module(module):
            for import_node in module.imports:
                if isinstance(import_node, nodes.NlgImport):
                    path = import_node.path
                    for macro in self._module_macros[path].itervalues():
                        if macro.phrase or macro.card:
                            result.append((path, macro))
                    for macro in self._foreign_macros[path].itervalues():
                        if macro.phrase or macro.card:
                            result.append((path, macro))
                    imported_module = self._modules.get(path) or self._foreign_modules.get(path)
                    process_module(imported_module)

        process_module(node)
        return result


class GenerateLocalizedSourceVisitor(GenerateSourceVisitor):
    def visit_Output(self, node):
        if node.localized_node:
            self.visit(node.localized_node)
        else:
            super(GenerateLocalizedSourceVisitor, self).visit_Output(node)

    def visit_LocalizedNode(self, node):
        cg = self._cg
        cg.write_line(coverage(node.lines) + ';')
        cg.write_line('out << ' + stringbuf(node.whitespace_prefix) + ';')

        cg.write_line('::NAlice::NNlg::LocalizeTemplateWithPlaceholders(')
        with cg.indent():
            cg.write_line('ctx,')
            cg.write_line(stringbuf(node.key_name) + ', // ' + node.data.replace('\n', '\\n'))
            cg.write_line('THashMap<TStringBuf, ::NAlice::NNlg::TRenderLocalizedNlgPlaceholder> {')
            with cg.indent():
                for placeholder_key, placeholder_node in node.placeholders.items():
                    cg.write_line('{ ' + stringbuf(placeholder_key) + ', ')
                    with cg.indent():
                        cg.write_line('[&]() {')
                        with cg.indent():
                            self._visit_output_child(placeholder_node)
                        cg.write_line('}')
                    cg.write_line('},')
            cg.write_line('},')
            cg.write_line('out')
        cg.write_line(');')

        cg.write_line('out << ' + stringbuf(node.whitespace_suffix) + ';')


def resolve_call_default_lines(default, lines):
    default.lines = lines
    if isinstance(default, nodes.List):
        for item in default.items:
            resolve_call_default_lines(item, lines)
    elif isinstance(default, nodes.Dict):
        for item in default.items:
            resolve_call_default_lines(item.value, lines)
    elif isinstance(default, nodes.Range):
        resolve_call_default_lines(default.start, lines)
        resolve_call_default_lines(default.stop, lines)
        resolve_call_default_lines(default.step, lines)


def gather_module_macros(library):
    result = defaultdict(dict)
    for module in library.modules:
        for macro in module.macros:
            result[module.path][macro.name] = macro
    return result


def gather_modules(library):
    return {module.path: module for module in library.modules}


def gather_foreign_macros(library):
    result = defaultdict(dict)
    for module in library.foreign_modules:
        for macro in module.macros:
            result[module.path][macro.name] = macro
    return result


def gather_foreign_modules(library):
    return {module.path: module for module in library.foreign_modules}


@contextmanager
def stack_frame(cg, name, lines):
    cg.write_line(
        'ctx.CallStack.push_back({{{name}, MODULE_PATH, {lineno}}});'.format(
            name=stringbuf(name),
            lineno=lines[0],
        ),
    )
    cg.write_line('[[maybe_unused]] auto& currentStackFrame = ctx.CallStack.back();')
    cg.skip_line()

    try:
        yield
    finally:
        cg.write_line('ctx.CallStack.pop_back();')


def declare_locals(cg, scope):
    if len(scope.names) > 0:
        for name, counter, param in scope.names.itervalues():
            if not param:
                cg.write_line(
                    'auto {name} = ::NAlice::NNlg::TValue::Undefined();'.format(name=gen_local_name(name, counter))
                )
        cg.skip_line()


def source_h_path(path, dir_name):
    return _source_module_path(path, dir_name) + '.h'


def source_cpp_path(path, dir_name):
    return _source_module_path(path, dir_name) + '.cpp'


def _source_module_path(path, dir_name):
    return os.path.join(dir_name, path)


def gen_namespace(template_path):
    path_noext = os.path.splitext(template_path)[0]
    namespaces = ('N' + pascalize(component.replace('..', '_parent_dir')) for component in path_noext.split('/'))

    return '::'.join(namespaces)


def gen_builtin(name):
    return '{}::{}'.format(BUILTINS_NAMESPACE, pascalize(name))


def gen_macro(name):
    return 'Macro_' + pascalize(name)


def gen_local_name(name, counter):
    return 'nlg__{}__{}'.format(camelize(name), counter)


def coverage(lines):
    return ', '.join('NLG_LINE({})'.format(line) for line in lines)


def callable_params(node, mandatory_params):
    params = mandatory_params + [
        '[[maybe_unused]] ::NAlice::NNlg::TValue ' + gen_local_name(var.name, var.counter)
        for var in node.scope.names.itervalues()
        if var.param
    ]
    return ', '.join(params)
