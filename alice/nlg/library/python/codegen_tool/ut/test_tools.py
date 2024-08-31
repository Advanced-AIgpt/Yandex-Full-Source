# coding: utf-8

from __future__ import unicode_literals

import yatest.common

import alice.nlg.library.python.codegen_tool as codegen_tool


def test_render():
    hello = 'Привет'

    template_path = yatest.common.source_path('alice/nlg/library/python/codegen_tool/ut/nlg/tests_1.nlg')
    context = {'from_microintent': {'data': {'text': hello}}}
    actual_result = codegen_tool.render_template(
        template_path,
        phrase='render_suggest_caption__from_microintent',
        context=context
    )

    expected_result = {'voice': hello, 'text': hello}
    assert expected_result == actual_result


def test_dump_ast():
    template_path = yatest.common.source_path('alice/nlg/library/python/codegen_tool/ut/nlg/tests_1.nlg')
    result = codegen_tool.dump_file_ast(template_path)
    assert result['_t'] == 'Template'
