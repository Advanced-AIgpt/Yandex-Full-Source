# coding: utf-8

from __future__ import unicode_literals
import random
import re
import os
from collections import defaultdict, OrderedDict

import attr
from jinja2 import nodes, TemplateSyntaxError, TemplateAssertionError
from jinja2.loaders import FileSystemLoader
from jinja2.ext import Extension

from vins_core.utils.data import open_resource_file, is_resource_exists
from vins_core.utils.misc import get_short_hash

ONLY_VOICE_MARK = 'vins_only_voice'
ONLY_TEXT_MARK = 'vins_only_text'
MAX_PREVIOUS_HANDCRAFTED_REPLIES = 10


class NLGExtension(Extension):
    tags = {'phrase', 'chooseline', 'chooseitem', 'maybe', 'nlgimport', 'voice', 'vc', 'text', 'tx', 'card',
            'cardtemplate', 'ext_import', 'ext_from', 'ext_nlgimport'}

    def __init__(self, environment):
        super(NLGExtension, self).__init__(environment)
        self._inside_mark_block = False

    def parse(self, parser):
        tok = parser.stream.current

        if tok.test('name:phrase'):
            return self.parse_phrase(parser)
        elif tok.test('name:card'):
            return self.parse_card(parser)
        elif tok.test('name:cardtemplate'):
            return self.parse_cardtemplate(parser)
        elif tok.test('name:chooseline'):
            return self.parse_choose_line(parser)
        elif tok.test('name:chooseitem'):
            return self.parse_choose_item(parser)
        elif tok.test('name:maybe'):
            return self.parse_maybe(parser)
        elif tok.test('name:nlgimport'):
            return self.parse_nlgimport(parser)
        elif tok.test('name:voice') or tok.test('name:vc'):
            return self.parse_mark_block(parser, ONLY_VOICE_MARK, ('name:endvoice', 'name:evc',))
        elif tok.test('name:text') or tok.test('name:tx'):
            return self.parse_mark_block(parser, ONLY_TEXT_MARK, ('name:endtext', 'name:etx',))
        elif tok.test('name:ext_import'):
            return self.parse_ext_import(parser)
        elif tok.test('name:ext_from'):
            return self.parse_ext_from(parser)
        elif tok.test('name:ext_nlgimport'):
            return self.parse_ext_nlgimport(parser)
        else:
            raise TemplateSyntaxError('Unknown tag "%s"' % tok.type, tok.lineno, filename=parser.filename)

    def _parse_block_node(self, parser, type_):
        node = nodes.Macro(lineno=next(parser.stream).lineno)
        node.args = ()
        node.defaults = ()
        node.block_type = type_
        node.original_name = parser.stream.expect('name').value
        node.name = 'nlg_%s__%s' % (type_, node.original_name)
        node.body = parser.parse_statements(('name:end' + type_,), drop_needle=True)
        return node

    def parse_phrase(self, parser):
        """ Create NLG phrase

        {% phrase phrase_id %}
          content
          ...
        {% endphrase %}
        """
        return self._parse_block_node(parser, 'phrase')

    def parse_card(self, parser):
        """ Create NLG phrase

        {% card card_id %}
          content
          ...
        {% endcard %}
        """
        return self._parse_block_node(parser, 'card')

    def parse_cardtemplate(self, parser):
        return self._parse_block_node(parser, 'cardtemplate')

    def parse_choose_line(self, parser):
        """
        Choose one line from all line inside this tag

        {% chooseline %}
          variant 1
          variant 2
          variant 3
        {% endchooseline %}
        """
        lineno = next(parser.stream).lineno
        no_repeat = False
        cycle = False
        if parser.stream.skip_if('comma'):
            mode = parser.parse_expression().value
            if mode == 'no_repeat':
                no_repeat = True
            elif mode == 'cycle':
                no_repeat = True
                cycle = True
            else:
                raise ValueError('Invalid chooseline mode "{}"'.format(mode))
        body = parser.parse_statements(('name:endchooseline',), drop_needle=True)
        ctx = nodes.ContextReference()
        call = self.call_method('choose_line', [ctx, nodes.Const(no_repeat), nodes.Const(cycle)])
        return nodes.CallBlock(call, [], [], body).set_lineno(lineno)

    def choose_line(self, context, no_repeat, cycle, caller):
        items = []
        for i in caller().split('\n'):
            item = i.strip()
            if item:
                items.append(i)
        used_replies = None
        if no_repeat and context['context'] is not None:
            hashed_items = [get_short_hash(reply) for reply in items]
            used_replies = context['context'].get('used_replies', [])
            filtered_items = [reply for reply, hash in zip(items, hashed_items) if hash not in used_replies]
            if not filtered_items and cycle:
                used_hashes_from_this_clause = [hash for hash in used_replies if hash in hashed_items]
                filtered_hashes = used_hashes_from_this_clause[:max(len(used_hashes_from_this_clause) * 2 // 3, 1)]
                used_replies[:] = (hash for hash in used_replies if hash not in filtered_hashes)
                filtered_items = [reply for reply, hash in zip(items, hashed_items) if hash in filtered_hashes]
            items = filtered_items
        # allow chooseline to be called with no lines
        if len(items) == 0:
            items = ['']
        reply = random.choice(items)
        if used_replies is not None and reply:
            used_replies.append(get_short_hash(reply))
            if len(used_replies) > MAX_PREVIOUS_HANDCRAFTED_REPLIES:
                del used_replies[:-MAX_PREVIOUS_HANDCRAFTED_REPLIES]
        return reply

    def parse_choose_item(self, parser):
        """ This tag expands to series of IFs on AST level

        This:
            {% chooseitem 0.2 %}
              a
            {% or 1 %}
              b
            {% or 2 %}
              c
            {% endchooseitem %}

        is translated to this:
            {% set __choose_item_1 = randuniform(0, 3.2) %}
            {% if __choose_item_1 < 0.2 %}
              a
            {% elif __choose_item_1 < 1.2 %}
              b
            {% else %}
              c
            {% endif %}
        """
        lineno = next(parser.stream).lineno
        items = []
        current_sum = 0.0
        weight_count = 0
        while True:
            tok = parser.stream.next_if('float') or parser.stream.next_if('integer')
            if tok:
                weight_count += 1
            weight = float(tok.value) if tok else 1.0
            body = parser.parse_statements(('name:or', 'name:endchooseitem'))
            current_sum += weight

            tok = next(parser.stream)
            items.append((current_sum, tok.lineno, body))

            if tok.test('name:endchooseitem'):
                break

        if 0 < weight_count < len(items):
            raise TemplateSyntaxError(
                'you must either specify all weights or not specify any weights at all',
                lineno,
                filename=parser.filename,
            )

        # create {% set __choose_item_1  = randuniform(0, current_sum) %} tag
        var_name = '__choose_item_%s' % lineno

        set_statement = nodes.Assign(
            nodes.Name(var_name, 'store', lineno=lineno),
            nodes.Call(nodes.Name('randuniform', 'load', lineno=lineno),
                       [nodes.Const(0, lineno=lineno), nodes.Const(current_sum, lineno=lineno)],
                       [], None, None, lineno=lineno),
            lineno=lineno,
        )

        # now create IFs
        def make_if(item_sum, item_lineno, item_body):
            return nodes.If(
                nodes.Compare(
                    nodes.Name(var_name, 'load', lineno=item_lineno),
                    [nodes.Operand('lt', nodes.Const(item_sum, lineno=item_lineno), lineno=item_lineno)],
                    lineno=item_lineno,
                ),
                item_body,
                [],
                [],
                lineno=item_lineno,
            )

        if len(items) == 1:
            # really no choice at all
            _, item_lineno, item_body = items[0]
            return nodes.Scope(item_body, lineno=item_lineno)

        assert len(items) > 1
        ifs = make_if(*items[0])
        ifs.elif_ = [
            make_if(*item)
            for item in items[1:-1]
        ]
        ifs.else_ = make_if(*items[-1]).body

        return nodes.Scope([set_statement, ifs], lineno=lineno)

    def parse_maybe(self, parser):
        """ This tag expands conditional construction on AST level

        This:
            {% maybe 0.2 %}
              a
            {% endmaybe %}

        is translated to this:
            {% set __maybe_1 = randuniform(0, 1) %}
            {% if __maybe_1 < 0.2 %}
              a
            {% endif %}
        """
        lineno = next(parser.stream).lineno
        tok = parser.stream.next_if('float') or parser.stream.next_if('integer')
        probability = float(tok.value) if tok else 0.5
        if probability < 0 or probability > 1:
            raise TemplateAssertionError(
                'expected probability within \'maybe\' construction, got %.2f' % probability,
                lineno
            )
        body = parser.parse_statements(('name:endmaybe',))
        parser.stream.expect('name:endmaybe')

        # create {% set __maybe_1 = random.uniform(0, 1) %} tag
        var_name = '__maybe_%s' % lineno

        set_statement = nodes.Assign(
            nodes.Name(var_name, 'store', lineno=lineno),
            nodes.Call(nodes.Name('randuniform', 'load', lineno=lineno),
                       [nodes.Const(0, lineno=lineno), nodes.Const(1, lineno=lineno)],
                       [], None, None, lineno=lineno),
            lineno=lineno,
        )

        # now create IF
        ifnode = nodes.If(
            nodes.Compare(
                nodes.Name(var_name, 'load', lineno=lineno),
                [nodes.Operand('lt', nodes.Const(probability, lineno=lineno), lineno=lineno)],
                lineno=lineno,
            ),
            body,
            [],
            [],
            lineno=lineno,
        )

        return nodes.Scope([set_statement, ifnode], lineno=lineno)

    def parse_mark_block(self, parser, mark_name, end_tags):
        """
        This:
            {% voice %}
              a
            {% endvoice %}
        is translated to this:
            <vins_only_voice>
              a
            </vins_only_voice>
        """

        lineno = next(parser.stream).lineno
        if self._inside_mark_block:
            raise TemplateAssertionError("nested mark blocks are not allowed", lineno)

        node_sequence = [nodes.Output([nodes.TemplateData('<%s>' % mark_name, lineno=lineno)], lineno=lineno)]
        self._inside_mark_block = True
        try:
            node_sequence.extend(parser.parse_statements(end_tags, drop_needle=True))
        finally:
            self._inside_mark_block = False

        lineno = parser.stream.current.lineno
        node_sequence.append(nodes.Output([nodes.TemplateData('</%s>' % mark_name, lineno=lineno)], lineno=lineno))
        return node_sequence

    def parse_nlgimport(self, parser):
        lineno = next(parser.stream).lineno

        node = nodes.Import(lineno=lineno)
        node.template = parser.parse_expression()
        node.with_context = False

        template = node.template.value
        if parser.stream.skip_if('name:as'):
            node.target = parser.parse_assign_target(name_only=True).name
        else:
            target = re.sub(r'\W', '_', template, flags=re.U).lower()
            node.target = '__%s_%s' % (target, lineno)

        node.import_type = 'nlgimport'
        node.exports = self.environment.loader.find_exports(self.environment, template, lineno)
        return node

    def parse_ext_import(self, parser):
        node = parser.parse_import()
        node.external = True
        return node

    def parse_ext_from(self, parser):
        node = parser.parse_from()
        node.external = True
        return node

    def parse_ext_nlgimport(self, parser):
        node = self.parse_nlgimport(parser)
        node.external = True
        return node


def create_nlgimport(filename):
    return '{%% nlgimport "%s" %%}\n' % filename


def create_chooseline(phrase_id, lines, mode=None):
    if mode is None:
        mode = ''
    else:
        mode = ', \'{}\''.format(mode)
    return ("""
{% phrase """ + phrase_id + """ %}
  {% chooseline""" + mode + """ %}
    """ + '\n    '.join(lines) + """
  {% endchooseline %}
{% endphrase %}
""")


def create_branched_chooseline(phrase_id, blocks, mode=None):
    result = '\n{% phrase ' + phrase_id + ' %}\n'
    first = True
    if mode is None:
        mode = ''
    else:
        mode = ', \'{}\''.format(mode)
    for predicate, lines in blocks.iteritems():
        if predicate == 'else':
            continue
        if_keyword = 'if' if first else 'elif'
        result += '  {% ' + if_keyword + ' ' + predicate + ('' if predicate.endswith(')') else '()') + ' %}\n'
        result += '    {% chooseline' + mode + ' %}\n      '
        result += '\n      '.join(lines)
        result += '\n    {% endchooseline %}\n'
        first = False
    if 'else' in blocks:
        result += '  {% else %}\n'
        result += '    {% chooseline' + mode + ' %}\n      '
        result += '\n      '.join(blocks['else'])
        result += '\n    {% endchooseline %}\n'
    result += '  {% endif %}\n'
    result += '{% endphrase %}\n'
    return result


@attr.s
class Block(object):
    _ast = attr.ib(repr=False)
    template = attr.ib()

    type = attr.ib(init=False)
    name = attr.ib(init=False)
    macro_name = attr.ib(init=False)
    lineno = attr.ib(init=False)

    def __attrs_post_init__(self):
        self.macro_name = self._ast.name
        self.type = self._ast.block_type
        self.lineno = self._ast.lineno
        self.name = self._ast.original_name
        self._ast = None


class TemplateStack(object):
    def __init__(self):
        self._files = []
        self._linenos = []

    def push(self, file_, lineno):
        self._files.append(file_)
        self._linenos.append(lineno)

    def pop(self):
        f = self._files.pop()
        self._linenos.pop()
        return f

    def __contains__(self, value):
        return value in self._files

    def __iter__(self):
        for i in xrange(len(self._files)):
            yield self._files[i], self._linenos[i]

    def __repr__(self):
        return repr(self._files)


class NLGTemplateLoader(FileSystemLoader):
    def __init__(self, paths, *args, **kwargs):
        self._include_paths = paths
        super(NLGTemplateLoader, self).__init__(paths, *args, **kwargs)
        self._ast_cache = {}
        self._export_cache = defaultdict(dict)
        self._stack = TemplateStack()

    def get_source(self, environment, template):
        for include in [""] + self._include_paths:
            full_path = os.path.join(include, template)
            if is_resource_exists(full_path):
                contents = open_resource_file(full_path).read()
                return contents, template, lambda: True

        return super(NLGTemplateLoader, self).get_source(environment, template)

    def get_ast(self, environment, template):
        res = self._ast_cache.get(template)
        if res is None:
            source, _, _ = self.get_source(environment, template)
            res = environment.parse(source)
            self._ast_cache[template] = res

        return res

    def find_exports(self, environment, template, lineno=None):
        """ Depth-first search of exported phrases and cards """
        blocks = OrderedDict()

        def _recur(template, lineno):
            if template in self._stack:
                self._stack.push(template, lineno)
                raise TemplateAssertionError(
                    message=('Cyclic import found.\nTraceback:\n\t%s' %
                             '\n\t'.join('%s:%s' % line for line in self._stack)),
                    lineno=lineno,
                )

            self._stack.push(template, lineno)

            try:
                if template in self._export_cache:
                    blocks.update(self._export_cache[template])
                    return

                ast = self.get_ast(environment, template)

                for node in ast.find_all((nodes.Macro, nodes.Import)):
                    block_type = getattr(node, 'block_type', None)
                    import_type = getattr(node, 'import_type', None)

                    if block_type is not None:
                        block = Block(node, template)
                        blocks[(block.type, block.name)] = block
                        self._export_cache[template][(block.type, block.name)] = block
                    elif import_type is not None:
                        _recur(node.template.value, node.lineno)
            finally:
                self._stack.pop()

        _recur(template, lineno)

        return blocks.values()
