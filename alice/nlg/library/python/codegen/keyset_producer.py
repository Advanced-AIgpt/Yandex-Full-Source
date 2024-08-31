# coding: utf-8

import json

from alice.nlg.library.python.codegen import nodes


LOCALIZED_NODES = (nodes.Output,)

RU_LANGUAGE = 'ru-RU'


def get_keyset(nlg, node, template_path, language):
    assert isinstance(node, nodes.Node)

    keyset_producer = KeySetProducer(nlg, template_path, language)
    keyset_producer.visit(node)

    return keyset_producer.get_keyset()


class KeyWithContext(object):
    _CTX_SHIFT = 6

    def __init__(self, body, key_name, lines, template_path, language):
        self.body = body
        self.key_name = key_name
        self.lines = lines
        self.template_path = template_path
        self.language = language
        self.context = ""

    def set_context(self, nlg, ctx_start, ctx_end):
        context_lines = []

        start = self.lines[0]
        finish = self.lines[-1]

        for line in xrange(max(ctx_start, start - self._CTX_SHIFT), min(ctx_end, finish + self._CTX_SHIFT) + 1):
            pos_line = line - 1
            if line in self.lines:
                context_lines.append(u"{:3d} -->> {}".format(line, nlg[pos_line]))
            elif (line < start and start - line < self._CTX_SHIFT) or (
                line > finish and line - finish < self._CTX_SHIFT
            ):
                context_lines.append(u"{:3d}      {}".format(line, nlg[pos_line]))
            elif (line < start and start - line == self._CTX_SHIFT) or (
                line > finish and line - finish == self._CTX_SHIFT
            ):
                context_lines.append(u"...")

        self.context = "\n".join(context_lines)

    def __unicode__(self):
        return json.dumps(
            dict(
                key_name=self.key_name,
                body=self.body,
                lines=str(self.lines),
                template_path=self.template_path,
                language=self.language or RU_LANGUAGE,
                context=self.context,
            ),
            ensure_ascii=False,
            sort_keys=True,
        )

    def __str__(self):
        return unicode(self).encode('utf-8')


class KeySetProducer(object):
    def __init__(self, nlg, template_path, language):
        self.template_path = template_path
        self.language = language

        self._keyset = []
        self._nlg = self._prepare_nlg(nlg)  # lines of raw nlg
        self._keyset_batch = []  # для фраз в рамках одного контекста
        self._current_ctx_start = 0
        self._current_ctx_end = 0

    def _prepare_nlg(self, data):
        return data.split('\n')

    def get_keyset(self):
        return self._keyset

    def visit(self, node):
        if not isinstance(node, nodes.Node):
            return

        self.pre_visit(node)
        for field_name in node.fields:
            field = getattr(node, field_name)
            if isinstance(field, list):
                for element in field:
                    self.visit(element)
            else:
                self.visit(field)
        self.post_visit(node)

    def pre_visit(self, node):
        if isinstance(node, nodes.Macro):
            self._current_ctx_start = min(node.lines)

    def post_visit(self, node):
        self._current_ctx_end = max(self._current_ctx_end, max(node.lines))

        if isinstance(node, LOCALIZED_NODES):
            if node.localized_node:
                key = KeyWithContext(
                    node.localized_node.data,
                    node.localized_node.key_name,
                    node.localized_node.lines,
                    self.template_path,
                    self.language,
                )
                self._keyset_batch.append(key)

        if isinstance(node, nodes.Macro):
            while self._keyset_batch:
                key = self._keyset_batch.pop(0)
                key.set_context(self._nlg, self._current_ctx_start, self._current_ctx_end)
                self._keyset.append(key)
