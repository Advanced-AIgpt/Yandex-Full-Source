# coding: utf-8

import hashlib
import os
import string
import re

from collections import Counter
from copy import deepcopy

from alice.nlg.library.python.codegen import nodes


GENERAL_NODES = (
    nodes.ExprStmt,
    nodes.Keyword,
    nodes.List,
    nodes.Range,
    nodes.SliceList,
    nodes.Caller,
    nodes.CondExpr,
    nodes.Concat,
    nodes.UnaryOp,
    nodes.BinaryOp,
    nodes.CaptureBlock,
)

KNOWN_NODES = (
    nodes.Output,
    nodes.TextFlag,
    nodes.MacroName,
    nodes.Const,
    nodes.Dict,
    nodes.Pair,
)


def get_library(template_path):
    path = os.path.normpath(os.path.dirname(template_path))
    parts = iter(path.split(os.path.sep))
    part = next(parts, None)
    while part and part != 'scenarios':
        part = next(parts, None)

    part = next(parts, None)
    if part:
        return part
    else:
        parts = path.split(os.path.sep)
        assert parts
        return parts[-1]


class Key(object):
    def __init__(self, body, library, lines):
        self.body = body
        self.key_name = self.make_key_name(library)
        self.lines = sorted(lines)

    def make_key_name(self, library):
        key_name = hashlib.md5()
        key_name.update(self.body.encode('utf-8'))
        return "{}_{}".format(library, key_name.hexdigest())


class KeyBuilder(object):
    def __init__(self):
        self._body_parts = []
        self._placeholders = {}
        self._start_line = None
        self._end_line = None
        self._current_functors = Counter()
        self._has_text = False

    def _prepare_text(self, s):
        # delete spaces before punctuation
        s = re.sub(r'\s+(?=[.,:!?;»])', '', s, flags=re.UNICODE)
        # collapse spaces
        return re.sub(r'\s+', ' ', s, flags=re.UNICODE)

    def _prepare_chooseline_text(self, s):
        parts = []
        for part in s.split('\n'):
            # delete spaces before punctuation
            part = re.sub(r'\s+(?=[.,:!?;»])', '', part, flags=re.UNICODE)
            # collapse spaces
            part = re.sub(r'\s+', ' ', part, flags=re.UNICODE)
            parts.append(part)
        s = "\n".join(parts)
        return re.sub(r'\s*\n\s*', '\n', s, flags=re.UNICODE)

    def _common_add(self, body_part, lines):
        self._body_parts.append(body_part)
        self._start_line = min(lines) if self._start_line is None else min(min(lines), self._start_line)
        self._end_line = max(lines) if self._end_line is None else max(max(lines), self._end_line)

    def _is_not_punction(self, text):
        text = text.replace('\\n', '')
        stripped_text = text.strip(string.punctuation + string.whitespace + u"«»")
        return bool(stripped_text) and stripped_text != 'n'  # check \\n

    def add_text(self, text, lines):
        text = self._prepare_text(text)
        if text:
            self._common_add(text, lines)
            self._has_text |= self._is_not_punction(text)

    def add_chooseline_text(self, text, lines):
        text = self._prepare_chooseline_text(text)
        if text:
            self._common_add(text, lines)
            self._has_text |= self._is_not_punction(text)

    def add_attr(self, attr, lines, node=None):
        attr = self._prepare_text(attr)
        if attr:
            placeholder_name = attr
            placeholder = u"{{{}}}".format(attr)
            self._common_add(placeholder, lines)
            if node:
                self._placeholders[placeholder_name] = node

    def add_call(self, call, lines, node=None, attr=""):
        call = self._prepare_text(call)
        if call:
            name = call.lstrip('{').rstrip('}()')
            self._current_functors[name] += 1
            ind = self._current_functors[name]
            ind_str = u"_" + str(ind) if ind > 1 else ""
            placeholder_name = u"{name}{ind}({attr})".format(name=name, ind=ind_str, attr=attr)
            placeholder = u"{{{}}}".format(placeholder_name)
            self._common_add(placeholder, lines)
            if node:
                self._placeholders[placeholder_name] = node

    def _build_body_raw(self):
        return "".join(self._body_parts)

    def build_key(self, library):
        if self._has_text:
            body = self._build_body_raw().strip()
            lines = xrange(self._start_line, self._end_line + 1)
            return Key(body, library, lines)
        return None

    def build_localized_node(self, library, scope):
        key = self.build_key(library)
        if key:
            body_raw = self._build_body_raw()
            whitespace_prefix = re.search(r'^\s*', body_raw, flags=re.UNICODE).group(0)
            whitespace_suffix = re.search(r'\s*$', body_raw, flags=re.UNICODE).group(0)
            return nodes.LocalizedNode(
                key.body,
                key.key_name,
                self._placeholders,
                whitespace_prefix,
                whitespace_suffix,
                lines=key.lines,
                scope=scope,
            )


def get_first_builtin_arg(node):
    if len(node.args) == 0:
        return ""
    wrap_node(node.args[0])
    if hasattr(node.args[0], 'attr'):
        return node.args[0].attr
    else:
        return ""


def wrap_node(node):
    if isinstance(node, (nodes.GlobalName, nodes.LocalName, nodes.MacroName)):
        node.attr = node.name
    elif isinstance(node, nodes.Const):
        node.attr = node.value
    elif isinstance(node, nodes.Builtin):
        node.first_attr = get_first_builtin_arg(node)
        node.attr = u"{name}({arg})".format(name=node.name, arg=node.first_attr)
    elif isinstance(node, nodes.Call):
        wrap_node(node.target)
        node.attr = node.target.attr
    elif isinstance(node, nodes.GetListItem):
        wrap_node(node.node)
        node.attr = u'{}.{}'.format(node.node.attr, node.index)
    elif isinstance(node, nodes.Getitem):
        wrap_node(node.node)
        node.attr = u"{}[]".format(node.node.attr)
    elif isinstance(node, (nodes.Getattr)):
        wrap_node(node.node)
        node.attr = u'{}.{}'.format(node.node.attr, node.attr)


class LocalizedOutputBuilder(object):
    def init(self, template_path):
        self._template_path = template_path
        self._key_builder = KeyBuilder()

    def build_node(self, node, template_path):
        self.init(template_path)
        self.visit(node)
        library = get_library(self._template_path)
        return self._key_builder.build_localized_node(library, node.scope)

    def visit(self, node):
        if not isinstance(node, nodes.Node):
            return

        mutable_node = deepcopy(node)
        self.pre_visit(mutable_node)
        try:
            for field_name in mutable_node.fields:
                mutable_field = getattr(mutable_node, field_name)
                if isinstance(mutable_field, list):
                    for mutable_element in mutable_field:
                        self.visit(mutable_element)
                else:
                    self.visit(mutable_field)
        finally:
            self.post_visit(node, mutable_node)

    def pre_visit(self, mutable_node):
        # игнорируем части обхода дерева
        if isinstance(mutable_node, nodes.Call):
            mutable_node.args = None
            mutable_node.kwargs = None
        elif isinstance(mutable_node, nodes.CallBlock):
            mutable_node.args = None
        elif isinstance(mutable_node, nodes.If):
            mutable_node.test = None
        elif isinstance(mutable_node, nodes.Macro):
            mutable_node.args = None
            mutable_node.kwargs = None
        elif isinstance(mutable_node, nodes.Builtin):
            wrap_node(mutable_node)
            mutable_node.args = None
            mutable_node.kwargs = None
        elif isinstance(mutable_node, nodes.Getattr):
            wrap_node(mutable_node)
            mutable_node.node = None
        elif isinstance(mutable_node, nodes.Getitem):
            wrap_node(mutable_node)
            mutable_node.arg = None
            mutable_node.node = None
        elif isinstance(mutable_node, nodes.GetListItem):
            wrap_node(mutable_node)
            mutable_node.node = None
        elif isinstance(mutable_node, GENERAL_NODES):
            for field in mutable_node.fields:
                mutable_node.__setattr__(field, None)

    def post_visit(self, node, mutable_node):
        # добавлем в текущую фразу текст или placeholders
        if isinstance(mutable_node, nodes.TemplateData):
            self._key_builder.add_text(mutable_node.data, mutable_node.lines)

        elif isinstance(mutable_node, nodes.ChooselineTemplateData):
            self._key_builder.add_chooseline_text(mutable_node.data, mutable_node.lines)

        elif isinstance(mutable_node, nodes.Getattr):
            self._key_builder.add_attr(mutable_node.attr, mutable_node.lines, node=node)

        elif isinstance(mutable_node, nodes.Getitem):
            self._key_builder.add_attr(mutable_node.attr, mutable_node.lines, node=node)

        elif isinstance(mutable_node, nodes.GetListItem):
            self._key_builder.add_attr(mutable_node.attr, mutable_node.lines, node=node)

        elif isinstance(mutable_node, (nodes.GlobalName, nodes.LocalName)):
            self._key_builder.add_attr(mutable_node.name, mutable_node.lines, node=node)

        elif isinstance(mutable_node, nodes.LoopAttr):
            self._key_builder.add_attr("loop." + mutable_node.attr, mutable_node.lines, node=node)

        elif isinstance(mutable_node, nodes.Call):
            if isinstance(mutable_node.target, nodes.MacroName):
                self._key_builder.add_call(mutable_node.target.name, mutable_node.lines, node=node)

        elif isinstance(mutable_node, nodes.Builtin):
            self._key_builder.add_call(mutable_node.name, mutable_node.lines, node=node, attr=mutable_node.first_attr)

        elif isinstance(mutable_node, GENERAL_NODES):
            self._key_builder.add_call(str(type(mutable_node).__name__), mutable_node.lines, node=node)

        elif isinstance(mutable_node, KNOWN_NODES):
            pass

        else:
            raise LookupError('Unknown node type of "{}" in {}'.format(type(mutable_node).__name__, self.template_path))
