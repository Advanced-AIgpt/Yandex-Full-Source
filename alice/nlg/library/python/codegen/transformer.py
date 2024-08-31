# coding: utf-8

from __future__ import unicode_literals

import itertools
import os
import jinja2.nodes as jinja_nodes
import six

from google.protobuf import text_format

from alice.nlg.library.python.codegen import errors
from alice.nlg.library.python.codegen import nodes
from alice.nlg.library.python.codegen import visitor
from alice.nlg.library.python.codegen.proto import module_serializer_pb2 as module_serializer
from alice.nlg.library.python.codegen.scope import DynamicScope, LexicalScope
from alice.nlg.library.python.codegen.localized_builder import LocalizedOutputBuilder
from contextlib import contextmanager
from copy import copy
from vins_core.utils.data import ensure_dir


CALLER_PARAMS_MAX = 4

TEXT_FLAGS_DATA = {
    '<vins_only_text>': (nodes.TextFlag.FLAG_TEXT, nodes.TextFlag.ACTION_BEGIN),
    '</vins_only_text>': (nodes.TextFlag.FLAG_TEXT, nodes.TextFlag.ACTION_END),
    '<vins_only_voice>': (nodes.TextFlag.FLAG_VOICE, nodes.TextFlag.ACTION_BEGIN),
    '</vins_only_voice>': (nodes.TextFlag.FLAG_VOICE, nodes.TextFlag.ACTION_END),
}

NLG_EXT = 'vins_core.nlg.nlg_extension.NLGExtension'
EXT_ATTRS = {
    (NLG_EXT, 'choose_line'): 'macro__choose_line',
}
BUILTIN_MACROS = EXT_ATTRS.values()

BUILTIN_FUNCTIONS = [
    'add_hours',
    'ceil_seconds',
    'client_action_directive',
    'create_date_safe',
    'datetime',
    'datetime_strptime',
    'normalize_time_units',
    'parse_dt',
    'parse_tz',
    'randuniform',
    'render_weekday_type',
    'render_weekday_simple',
    'render_datetime_raw',
    'render_units_time',
    'server_action_directive',
    'time_format',
    'timestamp_to_datetime',
]

BUILTIN_METHODS = {
    'append': 'list_append',
    'endswith': 'str_ends_with',
    'get': 'dict_get',
    'isoweekday': 'datetime_isoweekday',
    'items': 'dict_items',
    'join': 'str_join',
    'keys': 'dict_keys',
    'localize': 'localize',
    'lower': 'str_lower',
    'lstrip': 'str_lstrip',
    'replace': 'str_replace',
    'rstrip': 'str_rstrip',
    'split': 'str_split',
    'startswith': 'str_starts_with',
    'strftime': 'datetime_strftime',
    'strip': 'str_strip',
    'update': 'dict_update',
    'upper': 'str_upper',
    'values': 'dict_values',
}

TEST_BUILTINS = {
    'defined': 'test_defined',
    'undefined': 'test_undefined',
    'divisibleby': 'test_divisible_by',
    'even': 'test_even',
    'odd': 'test_odd',
    'eq': 'test_eq',
    '==': 'test_eq',
    'equalto': 'test_eq',
    'ge': 'test_ge',
    '>=': 'test_ge',
    'gt': 'test_gt',
    '>': 'test_gt',
    'greaterthan': 'test_gt',
    'in': 'test_in',
    'le': 'test_le',
    '<=': 'test_le',
    'lt': 'test_lt',
    '<': 'test_lt',
    'lessthan': 'test_lt',
    'ne': 'test_ne',
    '!=': 'test_ne',
    'iterable': 'test_iterable',
    'lower': 'test_lower',
    'upper': 'test_upper',
    'mapping': 'test_mapping',
    'none': 'test_none',
    'number': 'test_number',
    'sequence': 'test_sequence',
    'string': 'test_string',
}

NODES_WITH_OWN_SCOPE = frozenset(
    [
        # Template - not included because global scope is handled differently
        # Block - not included because we don't support blocks and extensions
        jinja_nodes.For,
        jinja_nodes.Macro,
        jinja_nodes.CallBlock,  # TODO(a-square): verify
        jinja_nodes.With,
        jinja_nodes.Scope,
    ]
)
SUPPORTED_LOOP_ATTRS = {'index', 'index0', 'revindex', 'revindex0', 'first', 'last', 'length', 'previtem', 'nextitem'}


def transform_ast(node, template_path, intent):
    assert isinstance(node, jinja_nodes.Template)

    transformer = AstTransformer(template_path, intent)
    try:
        return transformer.visit(node)
    except Exception as exc:
        errors.TemplateCompilationError.raise_wrapped(exc, template_path, transformer.lineno)


class AstTransformer(visitor.NodeVisitor):
    base_node_class = jinja_nodes.Node

    def __init__(self, template_path, intent):
        self.template_path = template_path
        self.intent = intent
        self.context = AstTransformerContext()
        self._import_manager = None
        self._scope = DynamicScope()
        self.lineno = '??'

    def pre_visit(self, node):
        if has_own_scope(node):
            # TODO(a-square): learn to pass lineno and scope to transformed nodes implicitly
            self._scope = LexicalScope(parent=self._scope)
            self.lineno = node.lineno

    def post_visit(self, node):
        if has_own_scope(node):
            if self._scope is not None:
                self._scope = self._scope.parent

    ######################################################################
    # statements
    ######################################################################

    def visit_Template(self, node):
        # collate all macros defined or imported from other templates
        # so that we can statically resolve them in the transformer stage
        self._import_manager = ImportManager()
        import_macros_visitor = ImportMacrosVisitor(self._import_manager)
        import_macros_visitor.visit(node)

        with self.context(want_macros=True):
            macros = flatmap(self.visit, node.body)
        with self.context(want_code=True):
            inits = flatmap(self.visit, node.body)
        with self.context(want_imports=True):
            imports = flatmap(self.visit, node.body)

        module = nodes.Module(
            self.template_path,
            self.intent,
            macros,
            inits,
            imports,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

        line_counter = LineCounterVisitor()
        line_counter.visit(module)
        module.total_lines = line_counter.num_lines

        coverage_segments_collector = CoverageSegmentsCollectorVisitor()
        coverage_segments_collector.visit(module)
        module.coverage_segments = coverage_segments_collector.segments()

        return module

    def visit_Import(self, node):
        if self.context.frame.reject_imports:
            raise errors.ImportError('Imports can only be defined at the top level')

        nlg_import = getattr(node, 'import_type', None) == 'nlgimport'
        external = getattr(node, 'external', False)

        if not isinstance(node.template, jinja_nodes.Const):
            raise errors.ImportError("Can only import statically defined template paths")

        if not isinstance(node.template.value, basestring):
            raise errors.ImportError("Import path must be a string")

        if not self.context.frame.want_imports:
            return []

        if nlg_import:
            return nodes.NlgImport(
                node.template.value,
                external,
                lines=one_line(node.lineno),
                scope=self._scope,
            )
        else:
            return nodes.Import(
                node.template.value,
                node.target,
                node.with_context,
                external,
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_FromImport(self, node):
        if self.context.frame.reject_imports:
            raise errors.ImportError('Imports can only be defined at the top level')

        if not isinstance(node.template, jinja_nodes.Const):
            raise errors.ImportError("Can only import statically defined template paths")

        if not isinstance(node.template.value, basestring):
            raise errors.ImportError("Import path must be a string")

        if not self.context.frame.want_imports:
            return []

        return nodes.FromImport(
            node.template.value,
            node.names,
            node.with_context,
            getattr(node, 'external', False),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Macro(self, node):
        if self.context.frame.reject_macros:
            raise errors.MacroDefinitionError('Macros can only be defined at the top level')

        if not self.context.frame.want_macros:
            return []

        with self.context(want_macros=False, want_code=True, allow_output=True, reject_imports=True):
            return nodes.Macro(
                node.name,
                self.intent,
                extract_substring(node.name, prefix='nlg_phrase__'),
                extract_substring(node.name, prefix='nlg_card__'),
                map(lambda arg: self.visit(arg, declaration=True), node.args),
                map(self.visit, node.defaults),
                flatmap(self.visit, node.body),
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_Output(self, node):
        if not self.context.frame.want_code:
            return []

        with self.context(reject_macros=True, reject_imports=True):
            items = flatmap(self.visit, node.nodes)

            if self.context.frame.allow_output:
                for item in items:
                    if 'direct_output' in item.attributes:
                        item.direct_output = True
                node = nodes.Output(items, lines=one_line(node.lineno), scope=self._scope)

                node.localized_node = LocalizedOutputBuilder().build_node(node, self.template_path)

                return node
            else:
                for item in items:
                    if 'suppress_output' in item.attributes:
                        item.suppress_output = True
                return nodes.ExprStmt(items, lines=one_line(node.lineno), scope=self._scope)

    def visit_ExprStmt(self, node):
        if not self.context.frame.want_code:
            return []

        expr = self.visit(node.node)
        if 'suppress_output' in expr.attributes:
            expr.suppress_output = True

        return nodes.ExprStmt(
            [expr],
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_If(self, node):
        if not self.context.frame.want_code:
            return []

        with self.context(reject_macros=True, reject_imports=True):
            test = self.visit(node.test)
            then = flatmap(self.visit, node.body)
            else_ifs = flatmap(self.visit, node.elif_)
            otherwise = flatmap(self.visit, node.else_)
            return nodes.If(
                test,
                then,
                else_ifs,
                otherwise,
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_Assign(self, node):
        if not self.context.frame.want_code:
            return []

        with self.context(reject_macros=True, reject_imports=True):
            # expression must be transformed before the target,
            # otherwise target might shadow another variable prematurely,
            #
            # e.g. {% set foo %}{{ foo }}{% endset %}
            expr = self.visit(node.node)
            return nodes.Assign(
                self.visit(node.target, declaration=True),
                expr,
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_With(self, node):
        if not self.context.frame.want_code:
            return []

        with self.context(reject_macros=True, reject_imports=True):
            # we translate targets and nodes to simple assignments in the body
            body = []
            for target, value in itertools.izip(node.targets, node.values):
                assert isinstance(target, jinja_nodes.Name)
                new_target = jinja_nodes.Name(target.name, 'store', lineno=node.lineno)
                body.append(jinja_nodes.Assign(new_target, value, lineno=node.lineno))
            body.extend(node.body)
            return nodes.Scope(
                flatmap(self.visit, body),
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_Scope(self, node):
        if not self.context.frame.want_code:
            return []

        with self.context(reject_macros=True, reject_imports=True):
            return nodes.Scope(
                flatmap(self.visit, node.body),
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_For(self, node):
        if not self.context.frame.want_code:
            return []

        if node.test:
            raise errors.UnsupportedFeatureError('For-if not supported')

        if node.recursive:
            raise errors.UnsupportedFeatureError('Recursive for not supported')

        with self.context(reject_macros=True, reject_imports=True):
            body_scope = LexicalScope(parent=self._scope)
            with self._tmp_scope(body_scope):
                target = self.visit(node.target, declaration=True)
                assert isinstance(target, (nodes.LocalName, nodes.List))

                body = flatmap(self.visit, node.body)

            otherwise_scope = LexicalScope(parent=self._scope)
            with self._tmp_scope(otherwise_scope):
                otherwise = flatmap(self.visit, node.else_)

            collection = self.visit(node.iter)

            return nodes.For(
                target,
                collection,
                body,
                otherwise,
                lines=one_line(node.lineno),
                scope=self._scope,
                body_scope=body_scope,
                otherwise_scope=otherwise_scope,
            )

    def visit_Continue(self, node):
        raise errors.UnsupportedFeatureError('Continue statement not supported')

    def visit_Break(self, node):
        raise errors.UnsupportedFeatureError('Break statement not supported')

    def _is_chooseline(self, call_node):
        return isinstance(call_node, nodes.Call) \
            and isinstance(call_node.target, nodes.BuiltinName) \
            and call_node.target.name == 'macro__choose_line'

    def visit_CallBlock(self, node):
        if not self.context.frame.want_code:
            return []

        if len(node.args) > CALLER_PARAMS_MAX:
            raise errors.UnsupportedFeatureError('Maximum number of caller parameters: {}'.format(CALLER_PARAMS_MAX))

        if len(node.defaults) > 0:
            raise errors.UnsupportedFeatureError('Default caller parameter values not supported')

        # TODO(a-square): learn to set proper direct_output value in other contexts
        call = self.visit(node.call, direct_output=True, call_block=True)
        if isinstance(call, nodes.Dict):
            # If this happened, someone used the built-in function dict or namespace
            # in the call block context.
            # We don't support kwargs so they are implemented by rewriting the AST
            # as if was a dictionary literal, which makes it impossible to use in a call block
            raise errors.CallTransformationError(
                'dict and namespace are special functions that cannot be used in a call block'
            )

        args = map(lambda arg: self.visit(arg, declaration=True), node.args)

        if self._is_chooseline(call):
            with self.context(allow_output=True, in_chooseline=True):
                body = flatmap(self.visit, node.body)
        else:
            with self.context(allow_output=True):
                body = flatmap(self.visit, node.body)

        return nodes.CallBlock(
            call,
            args,
            body,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_AssignBlock(self, node):
        if not self.context.frame.want_code:
            return []

        body = jinja_nodes.Scope(node.body, lineno=node.lineno)  # force a new scope
        expr = CaptureBlock([body], lineno=node.lineno)

        if node.filter is not None:
            filter_node = copy(node.filter)
            filter_node.node = expr
            expr = filter_node

        stmt = jinja_nodes.Assign(node.target, expr, lineno=node.lineno)
        return self.visit(stmt)

    def visit_FilterBlock(self, node):
        if not self.context.frame.want_code:
            return []

        body = jinja_nodes.Scope(node.body, lineno=node.lineno)  # force a new scope
        expr = CaptureBlock([body], lineno=node.lineno)
        filter_node = copy(node.filter)
        filter_node.node = expr
        expr = filter_node

        stmt = jinja_nodes.Output([expr], lineno=node.lineno)
        return self.visit(stmt)

    def visit_Block(self, node):
        raise errors.UnsupportedFeatureError('Blocks and extensions not supported')

    def visit_Extends(self, node):
        raise errors.UnsupportedFeatureError('Blocks and extensions not supported')

    def visit_Include(self, node):
        raise errors.UnsupportedFeatureError('Includes not supported')

    def visit_OverlayScope(self, node):
        raise errors.UnexpectedNodeError('Unexpected OverlayScope')

    def visit_EvalContextModifier(self, node):
        raise errors.UnexpectedNodeError('Unexpected EvalContextModifier')

    def visit_ScopedEvalContextModifier(self, node):
        raise errors.UnsupportedFeatureError('autoescape tag not supported')

    ######################################################################
    # expressions and helpers
    ######################################################################

    def visit_Call(self, node, direct_output=False, call_block=False):
        # we don't support *args and **kwargs
        if node.dyn_args is not None:
            raise errors.CallTransformationError('Dynamic args not supported')
        if node.dyn_kwargs is not None:
            raise errors.CallTransformationError('Dynamic kwargs not supported')

        call_to_type_node = self._try_call_to_type(node, call_block=call_block)
        if call_to_type_node is not None:
            return call_to_type_node

        builtin_method_call_node = self._try_call_builtin_method(node)
        if builtin_method_call_node is not None:
            return builtin_method_call_node

        builtin_function_node = self._try_call_builtin_function(node)
        if builtin_function_node is not None:
            return builtin_function_node

        # may throw CallTransformationError
        target_visitor = CallTargetVisitor(self._import_manager, self._scope)
        target = target_visitor.visit(node.node)

        if isinstance(target, nodes.Caller):
            if call_block:
                # caller is not a macro so it would take special care
                # to ensure it handles another caller well
                raise errors.UnsupportedFeatureError('Caller cannot be called with another caller')

            if len(node.args) > CALLER_PARAMS_MAX:
                raise errors.UnsupportedFeatureError('Maximum number of caller arguments: {}'.format(CALLER_PARAMS_MAX))
            if len(node.kwargs) > 0:
                raise errors.UnsupportedFeatureError('Callers don\'t support keyword arguments')

        args = []
        for arg in node.args:
            if isinstance(arg, jinja_nodes.ContextReference):
                target.with_context = True
                continue
            args.append(self.visit(arg))

        # built-ins can decide what arguments they do and do not want
        if isinstance(target, nodes.BuiltinName):
            args = transform_builtin_args(target.name, args)

        # we cannot resolve kwargs at this stage because templates are
        # transformed independently, so argument resolution id deferred
        # until the final compilation phase

        return nodes.Call(
            target,
            args,
            map(self.visit, node.kwargs),
            direct_output=direct_output,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_BinExpr(self, node):
        return nodes.BinaryOp(
            self.visit(node.left),
            node.operator,
            self.visit(node.right),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_UnaryExpr(self, node):
        return nodes.UnaryOp(
            node.operator,
            self.visit(node.node),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_CondExpr(self, node):
        return nodes.CondExpr(
            self.visit(node.test),
            self.visit(node.expr1),
            self.visit(node.expr2) if node.expr2 is not None else None,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Concat(self, node):
        return nodes.Concat(
            flatmap(self.visit, node.nodes),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Compare(self, node):
        if len(node.ops) != 1:
            raise errors.UnsupportedFeatureError('Only binary comparisons supported')
        op_node = node.ops[0]
        assert isinstance(op_node, jinja_nodes.Operand)

        lhs = self.visit(node.expr)
        rhs = self.visit(op_node.expr)
        op = op_node.op
        return nodes.BinaryOp(lhs, op, rhs, lines=one_line(node.lineno), scope=self._scope)

    def visit_Operand(self, node):
        assert False, 'Unexpected Operand outside of a Compare node'

    def visit_TemplateData(self, node):
        if node.data in TEXT_FLAGS_DATA:
            if not self.context.frame.allow_output:
                raise errors.UnsupportedFeatureError("text/voice tags aren't supported outside of macros")
            flag, action = TEXT_FLAGS_DATA[node.data]

            return nodes.TextFlag(
                flag,
                action,
                lines=one_line(node.lineno),
                scope=self._scope,
            )

        if self.context.frame.in_chooseline:
            node_builder = nodes.ChooselineTemplateData
        else:
            node_builder = nodes.TemplateData

        return node_builder(
            node.data,
            lines=string_lines(node.lineno, node.data),
            scope=self._scope,
        )

    def visit_Keyword(self, node):
        return nodes.Keyword(node.key, self.visit(node.value), lines=one_line(node.lineno), scope=self._scope)

    def visit_Getattr(self, node):
        module_global = self._try_module_global(node)
        if module_global is not None:
            return module_global

        if isinstance(node.node, jinja_nodes.Name) and node.node.name == 'loop':
            if node.attr not in SUPPORTED_LOOP_ATTRS:
                raise errors.UnsupportedFeatureError('Supported loop attributes: {!r}'.format(SUPPORTED_LOOP_ATTRS))

            if node.ctx != 'load':
                raise errors.UnsupportedFeatureError('You can only read loop attributes')

            return nodes.LoopAttr(node.attr, lines=one_line(node.lineno), scope=self._scope)

        return nodes.Getattr(
            self.visit(node.node),
            node.attr,
            node.ctx,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Getitem(self, node):
        if node.ctx != 'load':
            raise errors.UnsupportedFeatureError('Expressions like xs[i] = foo aren\'t supported')

        arg = node.arg
        if isinstance(arg, jinja_nodes.Const):
            value = arg.value
            if isinstance(value, basestring):
                return nodes.Getattr(
                    self.visit(node.node),
                    value,
                    node.ctx,
                    lines=one_line(node.lineno),
                    scope=self._scope,
                )
            elif isinstance(value, int):
                return nodes.GetListItem(
                    self.visit(node.node),
                    value,
                    lines=one_line(node.lineno),
                    scope=self._scope,
                )
            else:
                raise errors.UnsupportedFeatureError('Only string and integer subscripting allowed')
        elif isinstance(arg, jinja_nodes.Slice):
            none_const = nodes.Const(None, lines=one_line(node.lineno), scope=self._scope)
            return nodes.SliceList(
                self.visit(node.node),
                self.visit(arg.start) if arg.start is not None else none_const,
                self.visit(arg.stop) if arg.stop is not None else none_const,
                self.visit(arg.step) if arg.step is not None else none_const,
                lines=one_line(node.lineno),
                scope=self._scope,
            )
        elif isinstance(arg, (jinja_nodes.Tuple, jinja_nodes.List, jinja_nodes.Dict)):
            raise errors.UnsupportedFeatureError('Only string, integer, and slice subscripting allowed')
        else:
            return nodes.Getitem(
                self.visit(node.node),
                self.visit(arg),
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_NSRef(self, node, declaration=False):
        # for us, an NSRef is explicitly equivalent to a Getattr,
        # in particular, we ignore the declaration parameter
        name = jinja_nodes.Name(node.name, 'store', lineno=node.lineno)
        attr = jinja_nodes.Getattr(name, node.attr, 'store', lineno=node.lineno)
        return self.visit(attr)

    def visit_Name(self, node, declaration=False):
        name = node.name
        if name == 'loop':
            raise errors.UnsupportedFeatureError(
                'The loop variable can only be used with hardcoded attributes, e.g. loop.index0'
            )

        if name in ('varargs', 'kwargs'):
            raise errors.UnsupportedFeatureError('varargs and kwargs aren\'t supported')

        if declaration:
            if self._import_manager.get_module(name) is not None:
                raise errors.UnsupportedFeatureError("Imported modules cannot be re-assigned")
            self._scope.add_name(name, node.ctx == 'param')

        info = self._scope.get_name_info(node.name)
        if info is not None:
            return nodes.LocalName(node.name, info.counter, lines=one_line(node.lineno), scope=self._scope)
        else:
            return nodes.GlobalName(
                node.name,
                None,
                node.ctx,
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_Filter(self, node):
        return nodes.Builtin(
            node.name,
            [self.visit(node.node)] + map(self.visit, node.args),
            map(self.visit, node.kwargs),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Test(self, node):
        builtin = TEST_BUILTINS.get(node.name)
        if builtin is None:
            raise errors.UnsupportedFeatureError('Unsupported test: {}'.format(node.name))

        return nodes.Builtin(
            builtin,
            [self.visit(node.node)] + map(self.visit, node.args),
            map(self.visit, node.kwargs),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Const(self, node):
        assert isinstance(node.value, (bool, int, long, float, basestring, type(None)))
        return nodes.Const(
            node.value,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Tuple(self, node, declaration=False):
        return self.visit_List(node, declaration=declaration)

    def visit_List(self, node, declaration=False):
        # TODO(a-square): find a more clever way to deal with store-tuples
        if declaration:
            items = map(lambda item: self.visit(item, declaration=True), node.items)
        else:
            items = flatmap(self.visit, node.items)

        return nodes.List(
            items,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Dict(self, node):
        return nodes.Dict(
            flatmap(self.visit, node.items),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Pair(self, node):
        if isinstance(node.key, jinja_nodes.Const):
            if not isinstance(node.key.value, basestring):
                raise errors.UnsupportedFeatureError("Only string dict keys are supported")

        # tuples and consts are the only two literals that could be used in a Python dict,
        # here we reject tuples early, like we did with non-string consts above
        if isinstance(node.key, (jinja_nodes.Tuple, jinja_nodes.List, jinja_nodes.Dict)):
            raise errors.UnsupportedFeatureError("Only string dict keys are supported")

        node = nodes.Pair(
            self.visit(node.key),
            self.visit(node.value),
            lines=one_line(node.lineno),
            scope=self._scope,
        )

        return node

    def visit_CaptureBlock(self, node):
        with self.context(allow_output=True):
            return nodes.CaptureBlock(
                flatmap(self.visit, node.body),
                lines=one_line(node.lineno),
                scope=self._scope,
            )

    def visit_Slice(self, node):
        raise errors.UnexpectedNodeError('Unexpected Slice node')

    def visit_EnvironmentAttribute(self, node):
        raise errors.UnexpectedNodeError('Unexpected EnvironmentAttribute node')

    def visit_ExtensionAttribute(self, node):
        raise errors.UnexpectedNodeError('Unexpected ExtensionAttribute node')

    def visit_ImportedName(self, node):
        raise errors.UnexpectedNodeError('Unexpected ImportedName node')

    def visit_InternalName(self, node):
        raise errors.UnexpectedNodeError('Unexpected InternalName node')

    def visit_MarkSafe(self, node):
        raise errors.UnexpectedNodeError('Unexpected MarkSafe node')

    def visit_MarkSafeIfAutoescape(self, node):
        raise errors.UnexpectedNodeError('Unexpected MarkSafeIfAutoescape node')

    def visit_ContextReference(self, node):
        raise errors.UnexpectedNodeError('Unexpected ContextReference node')

    def _try_call_to_type(self, node, call_block=False):
        """We treat some calls of built-in functions as special syntactic constructs."""
        assert isinstance(node, jinja_nodes.Call)

        name_node = node.node
        if isinstance(name_node, jinja_nodes.Name):
            name = name_node.name

            # special case: namespace, treat it like a normal dictionary
            if name in ('dict', 'namespace'):
                return self.visit(transform_namespace(node))

            # special case: range
            if name == 'range':
                if call_block:
                    # even in Python this would blow up so no regerts here
                    raise errors.CallTransformationError('range cannot accept a caller')

                return nodes.Range(
                    *map(self.visit, transform_range_args(node)),
                    **dict(
                        lines=one_line(node.lineno),
                        scope=self._scope,
                    )
                )

    def _try_call_builtin_method(self, node):
        """We treat calls to .keys(), .values(), .items() as special built-in calls."""
        assert isinstance(node, jinja_nodes.Call)

        getattr_node = node.node
        if isinstance(getattr_node, jinja_nodes.Getattr):
            assert getattr_node.ctx == 'load'

            builtin = BUILTIN_METHODS.get(getattr_node.attr)
            if builtin is not None:
                return nodes.Builtin(
                    builtin,
                    [self.visit(getattr_node.node)] + map(self.visit, node.args),
                    map(self.visit, node.kwargs),
                    lines=one_line(node.lineno),
                    scope=self._scope,
                )

    def _try_call_builtin_function(self, node):
        """Some calls are calls to built-in functions, not macros."""
        assert isinstance(node, jinja_nodes.Call)

        name_node = node.node
        if isinstance(name_node, jinja_nodes.Name):
            assert name_node.ctx == 'load'

            if name_node.name in BUILTIN_FUNCTIONS:
                return nodes.Builtin(
                    name_node.name,
                    map(self.visit, node.args),
                    map(self.visit, node.kwargs),
                    lines=one_line(node.lineno),
                    scope=self._scope,
                )

    def _try_module_global(self, node):
        """We treat module.global `Getattr`s as special cases of `GlobalName`s."""
        assert isinstance(node, jinja_nodes.Getattr)

        name_node = node.node
        if isinstance(name_node, jinja_nodes.Name):
            name = name_node.name
            module_info = self._import_manager.get_module(name)
            if module_info is not None:
                module_path = module_info[0]
                return nodes.GlobalName(
                    node.attr,
                    module_path,
                    node.ctx,
                    lines=one_line(node.lineno),
                    scope=self._scope,
                )

    @contextmanager
    def _tmp_scope(self, scope):
        """Some nodes require manual scope management,
        this helper ensures safety while doing it.
        """
        old_scope = self._scope
        self._scope = scope
        try:
            yield
        finally:
            self._scope = old_scope


class LineCounterVisitor(visitor.GenericNodeVisitor):
    """Visits the transformed tree to count the total number of lines
    that could be covered.
    """

    def __init__(self):
        super(LineCounterVisitor, self).__init__()
        self._max_line = 0

    def generic_visit(self, node):
        self._max_line = max(self._max_line, max(node.lines))

    @property
    def num_lines(self):
        return self._max_line + 1


class CoverageSegmentsCollectorVisitor(visitor.GenericNodeVisitor):
    """Visits the transformed tree to collect all the source file segments
    that could be covered. NOTE: it merges all the adjacent segments.
    """

    def __init__(self):
        super(CoverageSegmentsCollectorVisitor, self).__init__()
        self._unique_line_nums = set()

    def generic_visit(self, node):
        if isinstance(node, nodes.TemplateData) and isinstance(node.scope, DynamicScope):
            return
        for line in node.lines:
            self._unique_line_nums.add(line)

    def segments(self):
        segments = []
        start_index = None
        prev_line_num = None
        for line_num in sorted(self._unique_line_nums):
            if start_index is None:
                start_index = line_num - 1
            if prev_line_num is not None and (line_num - prev_line_num) > 1:
                segments.append((start_index, prev_line_num))
                start_index = line_num - 1
            prev_line_num = line_num

        if start_index is not None:
            segments.append((start_index, prev_line_num))

        return segments


class CallTargetVisitor(visitor.NodeVisitor):
    """Statically resolves function calls. Not recursive by design.

    Sometimes it might give a false positive result, for example
    the following template will compile, but the generated
    C++ code will not:

    {% from "common/vars.nlg" import some_string %}
    {{ some_string() }}

    This issue cannot be resolved at the transformation stage
    because it is done independently for all templates.
    """

    base_node_class = jinja_nodes.Node
    visitor_exception = errors.CallTransformationError

    def __init__(self, import_manager, scope):
        self._import_manager = import_manager
        self._scope = scope

    def visit_ExtensionAttribute(self, node):
        """Built-in function call, returns a `BuiltinName`."""
        builtin = EXT_ATTRS.get((node.identifier, node.name))
        if builtin is None:
            raise errors.CallTransformationError(
                "Extension attribute not supported: identifier = {}, name = {}".format(node.identifier, node.name)
            )

        return nodes.BuiltinName(builtin, lines=one_line(node.lineno), scope=self._scope)

    def visit_Getattr(self, node):
        """Module-macro call, resolves module target to a path, returns a `MacroName`.
        Alternatively, a call of either .keys(), .values(), or .items() on a dictonary.
        """
        if not isinstance(node.node, jinja_nodes.Name):
            raise errors.CallTransformationError("Unknown macro call pattern")

        assert node.ctx == 'load'
        assert node.node.ctx == 'load'

        module_target = node.node.name
        module_path, with_context = self._resolve_module_target(module_target)
        macro_name = node.attr

        return nodes.MacroName(
            module_path,
            macro_name,
            with_context,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def visit_Name(self, node):
        """Plain macro call, resolves macro's module path, returns a `MacroName`."""
        assert node.ctx == 'load'

        macro_target = node.name
        if macro_target == 'caller':
            return nodes.Caller(lines=one_line(node.lineno), scope=self._scope)

        module_path, macro_name, with_context = self._resolve_macro_module(macro_target)
        return nodes.MacroName(
            module_path,
            macro_name,
            with_context,
            lines=one_line(node.lineno),
            scope=self._scope,
        )

    def _resolve_module_target(self, module_target):
        result = self._import_manager.get_module(module_target)
        if result is None:
            raise errors.CallTransformationError('Unknown module target: ' + module_target)
        return result

    def _resolve_macro_module(self, macro_name):
        result = self._import_manager.get_module_macro(macro_name)
        if result is None:
            raise errors.CallTransformationError('Unknown macro: ' + macro_name)
        return result


def _non_failing_new(cls, name, bases, d):
    """ "Yandex or not, you must realize... NOBODY creates custom nodes!"
    "Oh I don't think so!"
    """
    for attr in 'fields', 'attributes':
        storage = []
        storage.extend(getattr(bases[0], attr, ()))
        storage.extend(d.get(attr, ()))
        assert len(bases) == 1, 'multiple inheritance not allowed'
        assert len(storage) == len(set(storage)), 'layout conflict'
        d[attr] = tuple(storage)
    d.setdefault('abstract', False)
    return type.__new__(cls, name, bases, d)


# undoing contrib/python/Jinja2/jinja2/nodes.py:999
jinja_nodes.NodeType.__new__ = staticmethod(_non_failing_new)
del _non_failing_new


class CaptureBlock(jinja_nodes.Expr):
    """An AST extension that allows us to rewrite `AssignBlock` and `FilterBlock` nodes
    in a way that makes dealing with them easier.
    """

    fields = ('body',)


class ImportManager(object):
    """Holds metadata for imported modules and macros."""

    def __init__(self):
        self._modules = {}
        self._macros = {}

    def add_import(self, module_path, target, with_context):
        if target in self._modules:
            raise errors.ImportError('Import target redefinition: {!r}'.format(target))

        self._modules[target] = module_path, with_context

    def add_macro(self, module_path, name, target, with_context):
        if target in self._macros:
            if module_path is None:
                raise errors.MacroDefinitionError('Multiple definitions of macro {!r}'.format(target))
            else:
                raise errors.ImportError('FromImport target redefinition: {!r}'.format(target))

        self._macros[target] = module_path, name, with_context

    def get_module(self, module_target):
        return self._modules.get(module_target)

    def get_module_macro(self, macro_target):
        return self._macros.get(macro_target)


class ImportMacrosVisitor(visitor.NodeVisitor):
    """Fills an `ImportManager` with local and imported macros and imported templates."""

    permissive = True
    base_node_class = jinja_nodes.Node

    def __init__(self, import_manager):
        self._import_manager = import_manager

    def visit_Template(self, node):
        for child in node.body:
            self.visit(child)

    def visit_Import(self, node):
        if not isinstance(node.template, jinja_nodes.Const):
            raise errors.UnsupportedFeatureError('Non-constant imports aren\'t supported')

        assert node.target

        if getattr(node, 'import_type', None) == 'nlgimport':
            return  # nlgimport doesn't import anything in the conventional sense

        self._import_manager.add_import(
            node.template.value,
            node.target,
            node.with_context,
        )

    def visit_FromImport(self, node):
        if not isinstance(node.template, jinja_nodes.Const):
            raise errors.UnsupportedFeatureError('Non-constant imports aren\'t supported')

        for name_spec in node.names:
            if isinstance(name_spec, tuple):
                name, target = name_spec
            else:
                name = target = name_spec

            if self._import_manager.get_module(target) is not None:
                raise errors.UnsupportedFeatureError("Imported modules cannot be re-assigned")

            self._import_manager.add_macro(
                node.template.value,
                name,
                target,
                node.with_context,
            )

    def visit_Macro(self, node):
        self._import_manager.add_macro(
            None,  # means current module
            node.name,
            node.name,
            with_context=True,
        )


class AstTransformerContext(object):
    """Context is a stack of frames - instances of `AstTransformerContextFrame`.
    Each frame is a modification of its parent, and holds various flags necessary
    to understand how we should process AST nodes.
    The basic idea is that at certain points (corresponding to nodes)
    we enter a context that changes some rules of node transfomation,
    and when we leave that point (after recursive visit calls are processed)
    the context's state should roll back cleanly as if we never changed anything.
    """

    def __init__(self):
        self._frames = []
        self._okay_to_call = True

    def __call__(self, **kwargs):
        assert self._okay_to_call
        last_frame = self._frames[-1] if self._frames else None
        self._frames.append(AstTransformerContextFrame(last_frame, **kwargs))
        self._okay_to_call = False  # must enter on the new frame
        return self

    def __enter__(self):
        self._okay_to_call = True

    def __exit__(self, exc_type, exc_val, exc_tb):
        self._frames.pop()

    @property
    def frame(self):
        return self._frames[-1]


class AstTransformerContextFrame(object):
    """Represents current state of a `AstTransformerContext` instance.
    It's designed to behave like a defaultdict(lambda: None) but with
    attributes instead of items (to make the code slightly easier on the eyes).
    """

    def __init__(self, parent, **kwargs):
        parent_dict = parent.__dict__ if parent is not None else {}
        self.__dict__.update(parent_dict)
        self.__dict__.update(kwargs)

    def __getattr__(self, name):
        # Since __getattribute__ failed, it means __dict__ doesn't have
        # the attribute we're looking for.
        # For the frame, it's okay so we just return None
        return None


def flatmap(func, values):
    """Like map, but allows some values to expand into multiple
    results or disappear entirely.

    >>> flatmap(lambda x: x / 2 if x % 2 == 0 else [], [1, 2, 3, 4])
    [1, 2]
    """
    result = []
    for value in values:
        item = func(value)
        if isinstance(item, list):
            result.extend(item)
        else:
            result.append(item)
    return result


def one_line(lineno):
    return xrange(lineno, lineno + 1)


def string_lines(lineno, value):
    # TODO(a-square): adjust lines?
    return xrange(lineno, lineno + value.count('\n') + 1)


def extract_substring(name, prefix):
    if name.startswith(prefix):
        return name[len(prefix) :]

    return None


def has_own_scope(node):
    return type(node) in NODES_WITH_OWN_SCOPE


def transform_builtin_args(name, args):
    if name == 'macro__choose_line':
        return []
    else:
        raise errors.CallTransformationError('Unknown builtin: {}'.format(name))


def transform_namespace(node):
    """Transforms a namespace call into a Dict literal.
    We treat the two as equivalent because we don't support
    the full Python object model, and the reason they're separate
    in the first place is the difference between attributes and items,
    which is not important for us because we don't have true attributes.
    """
    assert isinstance(node, jinja_nodes.Call)

    items = [
        jinja_nodes.Pair(
            jinja_nodes.Const(kw.key, lineno=kw.lineno),
            kw.value,
            lineno=kw.lineno,
        )
        for kw in node.kwargs
    ]
    return jinja_nodes.Dict(items, lineno=node.lineno)


def transform_range_args(node):
    assert isinstance(node, jinja_nodes.Call)

    if len(node.kwargs) != 0:
        raise errors.RangeArgumentsError('range doesn\'t accept keyword arguments')

    start = jinja_nodes.Const(0, lineno=node.lineno)
    stop = jinja_nodes.Const(0, lineno=node.lineno)
    step = jinja_nodes.Const(1, lineno=node.lineno)

    args = node.args
    if len(args) == 1:
        stop = args[0]
    elif len(args) == 2:
        start, stop = args
    elif len(args) == 3:
        start, stop, step = args
    else:
        raise errors.RangeArgumentsError("Invalid number of arguments to range(): {}".format(len(args)))

    return [start, stop, step]


def arg_to_protobuf(msg, arg):
    msg.Name = arg.name
    return msg


def const_to_protobuf(proto_value, const_value):
    if isinstance(const_value, six.integer_types):
        proto_value.IntValue = const_value
    elif isinstance(const_value, float):
        proto_value.FloatValue = const_value
    elif isinstance(const_value, basestring):
        proto_value.StringValue = const_value
    elif isinstance(const_value, bool):
        proto_value.BoolValue = const_value
    elif const_value is None:
        proto_value.NullValue = 0
    else:
        raise errors.MacroDefinitionError(
            "Type of default value must be\
            int, float, string, bool, range, list, dict or None"
        )


def lines_to_protobuf(proto_lines, lines):
    for line in lines:
        proto_lines.append(line)


def range_to_protobuf(proto_range, start, stop, step):
    proto_range.Start.Value.IntValue = start.value
    proto_range.Stop.Value.IntValue = stop.value
    proto_range.Step.Value.IntValue = step.value
    lines_to_protobuf(proto_range.Start.Lines, start.lines)
    lines_to_protobuf(proto_range.Stop.Lines, stop.lines)
    lines_to_protobuf(proto_range.Step.Lines, step.lines)


def dict_to_protobuf(proto_dict, items):
    proto_dict.CopyFrom(module_serializer.TStruct())
    for field in items:
        assert isinstance(field, nodes.Pair)
        key, value = field.key, field.value
        proto_field = proto_dict.Fields[key.value]
        lines_to_protobuf(proto_field.Lines, field.lines)
        if isinstance(value, nodes.Const):
            const_to_protobuf(proto_field.Value, value.value)
        elif isinstance(value, nodes.Range):
            range_to_protobuf(proto_field.Value.RangeValue, value.start, value.stop, value.step)
        elif isinstance(value, nodes.List):
            list_to_protobuf(proto_field.Value.ListValue, value.items)
        elif isinstance(value, nodes.Dict):
            dict_to_protobuf(proto_field.Value.StructValue, value.items)
        else:
            raise errors.MacroDefinitionError(
                "Type of default value must be\
                int, float, string, bool, range, list, dict or None"
            )


def list_to_protobuf(proto_list, items):
    proto_list.CopyFrom(module_serializer.TListValue())
    for field in items:
        proto_field = proto_list.Values.add()
        lines_to_protobuf(proto_field.Lines, field.lines)
        if isinstance(field, nodes.Const):
            const_to_protobuf(proto_field.Value, field.value)
        elif isinstance(field, nodes.Range):
            range_to_protobuf(proto_field.Value.RangeValue, field.start, field.stop, field.step)
        elif isinstance(field, nodes.List):
            list_to_protobuf(proto_field.Value.ListValue, field.items)
        elif isinstance(field, nodes.Dict):
            dict_to_protobuf(proto_field.Value.StructValue, field.items)
        else:
            raise errors.MacroDefinitionError(
                "Type of default value must be\
                int, float, string, bool, range, list, dict or None"
            )


def macro_to_protobuf(msg, macro):
    msg.Name = macro.name
    msg.Intent = macro.intent
    if macro.phrase is not None:
        msg.PhraseName = macro.phrase
    if macro.card is not None:
        msg.CardName = macro.card
    for arg in macro.args:
        arg_to_protobuf(msg.Args.add(), arg)
    list_to_protobuf(msg.Defaults, macro.defaults)
    return msg


def import_to_protobuf(msg, import_node):
    msg.Path = import_node.path
    if isinstance(import_node, nodes.Import):
        msg.Type = 0
    elif isinstance(import_node, nodes.FromImport):
        msg.Type = 1
    elif isinstance(import_node, nodes.NlgImport):
        msg.Type = 2
    else:
        errors.ImportError('Wrong type for import_node')
    return msg


def module_to_protobuf(module, out_dir):
    msg = module_serializer.TModule()
    msg.Path = module.path
    msg.Intent = module.intent
    for import_node in module.imports:
        import_to_protobuf(msg.Imports.add(), import_node)
    for macro in module.macros:
        try:
            macro_to_protobuf(msg.Macros.add(), macro)
        except Exception as exc:
            lineno = list(macro.lines)[0]
            errors.TemplateCompilationError.raise_wrapped(exc, module.path, lineno)
    filename = os.path.join(out_dir, module.path) + ".pb.txt"
    with open(filename, 'w') as f:
        f.write(text_format.MessageToString(msg))
    return msg


def proto_value_to_const(proto_value, lines):
    which = proto_value.WhichOneof('kind')
    if which == 'NullValue':
        return nodes.Const(None, lines=lines)
    elif which == 'IntValue':
        return nodes.Const(proto_value.IntValue, lines=lines)
    elif which == 'FloatValue':
        return nodes.Const(proto_value.FloatValue, lines=lines)
    elif which == 'StringValue':
        return nodes.Const(proto_value.StringValue, lines=lines)
    elif which == 'BoolValue':
        return nodes.Const(proto_value.BoolValue, lines=lines)
    elif which is None:
        raise errors.MacroDefinitionError('Value not set')


def proto_field_to_field(proto_field, lines):
    which = proto_field.WhichOneof('kind')
    if which == 'ListValue':
        return proto_list_to_list(proto_field.ListValue, lines)
    elif which == 'StructValue':
        return proto_struct_to_dict(proto_field.StructValue, lines)
    elif which == 'RangeValue':
        proto_range = proto_field.RangeValue
        start = proto_field_to_field(proto_range.Start.Value, proto_range.Start.Lines)
        stop = proto_field_to_field(proto_range.Stop.Value, proto_range.Stop.Lines)
        step = proto_field_to_field(proto_range.Step.Value, proto_range.Step.Lines)
        return nodes.Range(start, stop, step, lines=lines)
    else:
        return proto_value_to_const(proto_field, lines)


def proto_struct_to_dict(proto_dict, lines):
    items = []
    for proto_key in proto_dict.Fields:
        assert isinstance(proto_key, str)
        key = nodes.Const(proto_key)
        proto_value = proto_dict.Fields[proto_key]
        value = proto_field_to_field(proto_value.Value, proto_value.Lines)
        items.append(nodes.Pair(key, value))
    return nodes.Dict(items, lines=lines)


def proto_list_to_list(proto_list, lines=None):
    items = []
    for proto_value in proto_list.Values:
        items.append(proto_field_to_field(proto_value.Value, proto_value.Lines))
    return nodes.List(items, lines=lines)


def protobuf_to_macro(proto_macro):
    phrase_name = proto_macro.PhraseName or None
    card_name = proto_macro.CardName or None
    return nodes.Macro(
        proto_macro.Name,
        proto_macro.Intent,
        phrase_name,
        card_name,
        [nodes.LocalName(arg.Name, 0) for arg in proto_macro.Args],
        proto_list_to_list(proto_macro.Defaults).items,
        None,  # body
        lines=None,
        scope=None,
    )


def protobuf_to_import(proto_import):
    path = proto_import.Path
    if proto_import.Type == 0:
        return nodes.Import(path, None, None, None)
    elif proto_import.Type == 1:
        return nodes.FromImport(path, None, None, None)
    elif proto_import.Type == 2:
        return nodes.NlgImport(path, None)
    else:
        errors.ImportError('Wrong type of proto_import')


def protobuf_to_module(proto_module):
    macros = []
    for macro in proto_module.Macros:
        macros.append(protobuf_to_macro(macro))
    imports = []
    for proto_import in proto_module.Imports:
        imports.append(protobuf_to_import(proto_import))
    return nodes.Module(
        proto_module.Path,
        proto_module.Intent,
        macros,
        None,  # inits
        imports,
        lines=None,
        scope=None,
    )


def collect_foreign_modules(modules, out_dir):
    inner_modules = {module.path for module in modules}

    def add_foreign_imports(imports, module):
        for import_node in module.imports:
            if import_node.path not in inner_modules:
                if not import_node.external:
                    raise errors.ImportError(
                        "Invalid import of a foreign module \
                        '{}', use prefix 'ext_'".format(
                            import_node.path
                        )
                    )
                imports.add(import_node)
            else:
                if import_node.external:
                    raise errors.ImportError(
                        "Invalid import of an inner module \
                        '{}', you may not use prefix 'ext_'".format(
                            import_node.path
                        )
                    )

    imports = set()
    for module in modules:
        try:
            add_foreign_imports(imports, module)
        except Exception as exc:
            errors.TemplateCompilationError.raise_wrapped(exc, module.path, '??')

    result = dict()

    def parse_proto(path):
        foreign_module = module_serializer.TModule()
        filename = os.path.join(out_dir, path) + ".pb.txt"
        if not os.path.exists(filename):
            raise errors.ImportError('Unknown foreign module: {!r}'.format(path))
        with open(filename, 'r') as f:
            foreign_module = text_format.Parse(f.read(), module_serializer.TModule())
        return protobuf_to_module(foreign_module)

    def process_import_node(import_node):
        path = import_node.path
        if path in result:
            return
        module = parse_proto(path)
        assert module.path == path
        result[path] = module
        for module_import in module.imports:
            process_import_node(module_import)

    for import_node in imports:
        process_import_node(import_node)

    return list(result.values())


def build_library(modules, out_dir):
    ensure_dir(out_dir)
    foreign_modules = collect_foreign_modules(modules, out_dir)
    for module in modules:
        module_to_protobuf(module, out_dir)
    return nodes.Library(modules, foreign_modules)
