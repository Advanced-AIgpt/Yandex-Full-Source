# coding: utf-8

from __future__ import unicode_literals

import itertools
import operator


class Node(object):
    # Determines if code tracing and coverage should be injected into the node's expression.
    # Statements are responsible for their own tracing and coverage
    has_coverage = False

    # TODO(a-square): what makes something a field or an attribute?
    attributes = ('lines', 'scope')
    fields = ()

    def __init__(self, *fields, **attributes):
        assert len(fields) == len(self.fields), 'Invalid initialization of {}'.format(self.__class__.__name__)

        for name, value in itertools.izip(self.fields, fields):
            setattr(self, name, value)

        for name in self.attributes:
            setattr(self, name, attributes.get(name))

    def __eq__(self, other):
        if type(self) != type(other):
            return False

        comparator = operator.attrgetter(*(self.attributes + self.fields))
        return comparator(self) == comparator(other)

    def __repr__(self):
        return '{}({!r}, {!r})'.format(
            type(self).__name__,
            tuple(getattr(self, field) for field in self.fields),
            {attr: getattr(self, attr, None) for attr in self.attributes},
        )


class Library(Node):
    fields = ('modules', 'foreign_modules')


class Module(Node):
    attributes = Node.attributes + ('total_lines', 'coverage_segments')
    fields = ('path', 'intent', 'macros', 'init', 'imports')


class Import(Node):
    fields = ('path', 'target', 'with_context', 'external')


class FromImport(Node):
    fields = ('path', 'names', 'with_context', 'external')


class NlgImport(Node):
    """Corresponds to nlgimport statements.
    Re-registers phrases and cards from the given template into the current one.
    """

    fields = ('path', 'external')


class Macro(Node):
    fields = ('name', 'intent', 'phrase', 'card', 'args', 'defaults', 'body')


class Output(Node):
    fields = ('nodes',)  # TODO: add localized_node, see more ALICEINFRA-946


class LocalizedNode(Node):
    fields = ('data', 'key_name', 'placeholders', 'whitespace_prefix', 'whitespace_suffix')


class ExprStmt(Node):
    fields = ('nodes',)


class TemplateData(Node):
    has_coverage = True
    fields = ('data',)


class ChooselineTemplateData(Node):
    has_coverage = True
    fields = ('data',)


class TextFlag(Node):
    FLAG_TEXT = 0
    FLAG_VOICE = 1

    ACTION_BEGIN = 0
    ACTION_END = 1

    has_coverage = True
    fields = ('flag', 'action')


class CallBlock(Node):
    fields = ('call', 'args', 'body')


class Call(Node):
    has_coverage = True
    attributes = Node.attributes + ('direct_output', 'suppress_output')
    fields = ('target', 'args', 'kwargs')


class Keyword(Node):
    fields = ('key', 'value')


class Const(Node):
    has_coverage = True
    fields = ('value',)


class List(Node):
    has_coverage = True
    fields = ('items',)


class Dict(Node):
    has_coverage = True
    fields = ('items',)


class Pair(Node):
    """Pair of dict key and value.
    Key must evaluate to a string.
    """

    fields = ('key', 'value')


class Range(Node):
    has_coverage = True
    fields = ('start', 'stop', 'step')


class Getattr(Node):
    has_coverage = True
    fields = ('node', 'attr', 'ctx')


class GetListItem(Node):
    """Subscripts a list with a constant integer."""

    has_coverage = True
    fields = ('node', 'index')


class Getitem(Node):
    """Generic subscript operator."""

    has_coverage = True
    fields = ('node', 'arg')


class SliceList(Node):
    """Subscripts a list with a slice."""

    has_coverage = True
    fields = ('node', 'start', 'stop', 'step')


class GlobalName(Node):
    fields = ('name', 'module_path', 'ctx')


class LocalName(Node):
    """Because of the scoping differences between C++ and Python,
    we declare our variables up-front when the Python scope would start,
    and un-shadow names manually.
    `name` represents the original name, and `counter` is used to
    generate actual C++ names that don't shadow each other.
    """

    fields = ('name', 'counter')


class BuiltinName(Node):
    fields = ('name',)


class MacroName(Node):
    """Represents a statically resolved macro name.
    Can only appear as the `target` attribute of a `Call`.
    """

    fields = ('module_path', 'name', 'with_context')


class Caller(Node):
    """Represents a call to the macro caller.
    Currently, caller parameters are not supported.
    """


class Builtin(Node):
    has_coverage = True
    fields = ('name', 'args', 'kwargs')


class BinaryOp(Node):
    has_coverage = True
    fields = ('lhs', 'op', 'rhs')


class UnaryOp(Node):
    has_coverage = True
    fields = ('op', 'target')


class Concat(Node):
    has_coverage = True
    fields = ('nodes',)


class CondExpr(Node):
    """Represents a conditional **expression**,
    e.g. `then if test else otherwise`.
    """

    has_coverage = True
    fields = ('test', 'then', 'otherwise')


class If(Node):
    """Represents a conditional **statement**.

    `test` is the conditional expression
    `then` is a list of then-statements.
    `else_ifs` is a list of else-if type conditions with empty `else_ifs` or `else`.
    `otherwise` is a list of else-statements.
    """

    fields = ('test', 'then', 'else_ifs', 'otherwise')


class For(Node):
    attributes = Node.attributes + ('body_scope', 'otherwise_scope')
    fields = ('target', 'collection', 'body', 'otherwise')


class LoopAttr(Node):
    """Represents expressions like loop.index0 inside a for loop."""

    has_coverage = True
    fields = ('attr',)


class Scope(Node):
    fields = ('body',)


class Assign(Node):
    fields = ('target', 'node')


class CaptureBlock(Node):
    """Captures output of the statements in its body to a string value."""

    fields = ('body',)
