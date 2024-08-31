import json
import os.path
import pytest
import re
import yatest.common

from alice.nlg.library.python.nlg_renderer import (
    create_nlg_renderer_from_nlg_library_path,
    create_alice_rng, create_render_context_data,
    Language
)


def read_nlg_context_file(nlg_context_file_name):
    nlg_context_file_path = os.path.join('alice/hollywood/tests/nlg_tests/data', nlg_context_file_name)
    lines = []
    with open(yatest.common.source_path(nlg_context_file_path)) as f:
        lines = f.readlines()
    return [json.loads(line) for line in lines if line.strip()]


def format_render_report(nlg_render_contexts, render_phrase_results):
    report_lines = []
    indexes = range(len(nlg_render_contexts))
    for index, nlg_render_context, render_phrase_result in zip(indexes, nlg_render_contexts, render_phrase_results):
        render_context_info = '#{}: template = "{}", phrase = "{}"'.format(
            index, nlg_render_context['template_name'], nlg_render_context['phrase_name'])
        report_lines.append(render_context_info)

        assert not re.search('[а-я]', render_phrase_result.Text, re.IGNORECASE), render_context_info
        report_lines.append('Text: ' + render_phrase_result.Text)

        assert not re.search('[а-я]', render_phrase_result.Voice, re.IGNORECASE), render_context_info
        report_lines.append('Voice: ' + render_phrase_result.Voice)

    return '\n'.join(report_lines)


class TestScenarioNlg(object):
    @pytest.mark.parametrize('nlg_library_path, nlg_context_file_name', [
        pytest.param('alice/hollywood/library/scenarios/weather/nlg', 'weather_ar.json', id='weather'),
    ])
    def test_scenario_arabic_nlg_voice(self, nlg_library_path, nlg_context_file_name):
        nlg_renderer = create_nlg_renderer_from_nlg_library_path(nlg_library_path, create_alice_rng(1))
        nlg_render_contexts = read_nlg_context_file(nlg_context_file_name)
        render_phrase_results = []

        for nlg_render_context in nlg_render_contexts:
            render_context_data = create_render_context_data(
                nlg_render_context['context'], nlg_render_context['form'], nlg_render_context['req_info'])

            render_phrase_result = nlg_renderer.render_phrase(
                nlg_render_context['template_name'], nlg_render_context['phrase_name'], Language.AR,
                create_alice_rng(2), render_context_data)

            render_phrase_results.append(render_phrase_result)

        return format_render_report(nlg_render_contexts, render_phrase_results)
