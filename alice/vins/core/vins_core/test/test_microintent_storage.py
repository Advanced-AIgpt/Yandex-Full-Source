# coding: utf-8

from __future__ import unicode_literals

import pytest

from vins_core.config.app_config import MicrointentStorage, Project


@pytest.mark.parametrize('config,result', [
    (
        {
            'nlu': ['пример'],
            'nlg': ['ответ 1', 'ответ 2'],
        },
        """
{% phrase render_result %}
  {% chooseline, 'cycle' %}
    ответ 1
    ответ 2
  {% endchooseline %}
{% endphrase %}
"""
    ),
    (
        {
            'nlu': ['пример'],
            'nlg': ['ответ 1', 'ответ 2'],
            'gc_fallback': True
        },
        """
{% phrase render_result %}
  {% chooseline, 'no_repeat' %}
    ответ 1
    ответ 2
  {% endchooseline %}
{% endphrase %}
"""
    ),
    (
        {
            'nlu': ['пример'],
            'nlg': ['ответ 1', 'ответ 2'],
            'allow_repeats': True
        },
        """
{% phrase render_result %}
  {% chooseline %}
    ответ 1
    ответ 2
  {% endchooseline %}
{% endphrase %}
"""
    ),
    (
        {
            'nlu': ['пример'],
            'nlg': {
                'is_smth': ['ответ 1', 'ответ 2'],
                'else': ['ответ 3']
            }
        },
        """
{% phrase render_result %}
  {% if is_smth() %}
    {% chooseline, 'cycle' %}
      ответ 1
      ответ 2
    {% endchooseline %}
  {% else %}
    {% chooseline, 'cycle' %}
      ответ 3
    {% endchooseline %}
  {% endif %}
{% endphrase %}
"""
    ),
])
def test_microintents_nlg(config, result):
    assert MicrointentStorage._get_nlg('render_result', config) == result


@pytest.mark.parametrize('microintents_config', [
    'correct_config_with_everything.yaml',
    pytest.param('incorrect_config_without_nlu.yaml', marks=pytest.mark.xfail()),
    pytest.param('incorrect_config_with_wrong_nlu.yaml', marks=pytest.mark.xfail()),
    pytest.param('incorrect_config_with_wrong_nlg.yaml', marks=pytest.mark.xfail()),
    pytest.param('incorrect_config_with_wrong_suggests.yaml', marks=pytest.mark.xfail()),
    # the conditions within nlg cannot be interpreted, because the checks are not found (probably not here?)
])
def test_microintent_validation(microintents_config):
    project_config = {
        'name': 'grandparent_name',
        'microintents': [
            {
                'name': 'parent1_name',
                'path': 'vins_core/test/test_data/microintents/{}'.format(microintents_config)
            }
        ]
    }
    Project.from_dict(project_config)
    # we are expecting the wrong configs to fail during the project building


def test_microintent_settings_inheritance():
    project_config = {
        'name': 'grandparent_name',
        'microintents': [
            {
                'name': 'parent1_name',
                'path': 'vins_core/test/test_data/microintents/test_microintent_cfg1.yaml'
            },
            {
                'path': 'vins_core/test/test_data/microintents/test_microintent_cfg2.yaml',
                'allowed_apps': 'app1'
            }
        ]
    }
    project = Project.from_dict(project_config)
    intents_dict = {intent.name: intent for intent in project.intents}
    # this is the default default value
    assert intents_dict['grandparent_name.parent1_name.type1_intent1_name'].allowed_apps is None
    # this value was set on the intent level
    assert intents_dict['grandparent_name.parent1_name.type1_intent2_name'].allowed_apps == 'app1'
    # this is a file-wise default value
    assert intents_dict['grandparent_name.type2_intent1_name'].allowed_apps == 'app1'
    # this is the individually overwritten default value
    assert intents_dict['grandparent_name.type2_intent2_name'].allowed_apps == '.*'
