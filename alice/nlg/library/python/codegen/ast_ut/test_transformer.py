# coding: utf-8

import codecs
import json
import os
import pytest
import yatest.common

from collections import OrderedDict

from alice.nlg.library.python.codegen_tool import dump_ast, parser, smart_unicode, transform_ast
from alice.nlg.library.python.codegen.ast_ut.common import NLG_FILES, TEMPLATES_DIR


MOCKED_PATH_TO_NLG = 'MOCKED_DIR_TO_NLG/some.nlg'


EXPECTED_LOCALIZED_NODE = OrderedDict(
    [
        ('_t', '(T)LocalizedNode'),
        (u':lines', [7, 8, 9]),
        (
            u':scope',
            "LexicalScope(OrderedDict([('available_alarms', ScopedVariable(name='available_alarms', counter=0, param=True))]))",
        ),
        (u'data', u'Сейчас установлено несколько будильников: \\n'),
        (u'key_name', 'MOCKED_DIR_TO_NLG_d1522992eb0b89601d692c426e2f4c6e'),
        (u'placeholders', {}),
        (u'whitespace_prefix', ' '),
        (u'whitespace_suffix', ''),
    ]
)


@pytest.mark.parametrize('nlg', NLG_FILES)
@pytest.mark.parametrize('is_dump_with_localized', [
    pytest.param(False, id='unlocalized'),
    pytest.param(True, id='localized')
])
def test_ast_transformer(nlg, is_dump_with_localized):
    template_path = os.path.join(TEMPLATES_DIR, nlg)
    with codecs.open(template_path, encoding='utf-8') as template_file:
        data = smart_unicode(template_file.read())
        ast = parser.parse(data, template_path)
        transformed_ast = transform_ast(ast, MOCKED_PATH_TO_NLG, nlg)

        ast_dict = dump_ast(transformed_ast, with_localized=is_dump_with_localized)

        if nlg == 'alarm_cancel.nlg' and is_dump_with_localized:
            actual_localized_nodes = ast_dict['macros'][0]['body'][1]['otherwise'][0]['localized_node']
            assert actual_localized_nodes == EXPECTED_LOCALIZED_NODE

        canon_path = 'result.json'
        with codecs.open(canon_path, 'w', encoding='utf-8') as output_file:
            json.dump(ast_dict, output_file, sort_keys=True, indent=4, ensure_ascii=False)

        return yatest.common.canonical_file(canon_path, local=True)
