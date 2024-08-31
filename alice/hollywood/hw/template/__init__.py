import getpass
import json

import alice.library.python.template as template
import alice.library.python.utils as utils
from library.python import resource

from . import product_scenarios
from . import shards
from . import scenario_config
from . import yamake


__all__ = ['product_scenarios', 'shards', 'yamake']


_template_config = """
{%- set scenario_name_camel = scenario_name|camel_case %}
{%- set scenario_name_snake = scenario_name_camel|snake_case %}
{%- macro frame_name() %}{{scenario_name_snake}}_hello_world{%- endmacro %}
{
    "username": "{{username}}",
    "ScenarioName": "{{scenario_name_camel}}",
    "SCENARIONAME": "{{scenario_name_camel|upper}}",
    "scenario_name": "{{scenario_name_snake}}",
    "SCENARIO_NAME": "{{scenario_name_snake|upper}}",
    "frame_name": "personal_assistant.scenarios.{{frame_name()}}",
    "FRAME_NAME": "{{frame_name()|upper}}"
}
"""


class ScenarioRenderer(template.Renderer):
    _templates = {
        'config': _template_config,
        scenario_config.filename: scenario_config.template,
    }

    def __init__(self, *args, **kwargs):
        super().__init__(*args, **kwargs)
        self._jinja_env.filters['camel_case'] = utils.to_camel_case
        self._jinja_env.filters['snake_case'] = utils.to_snake_case

    def _loader(self, name):
        if name in self._templates:
            return self._templates[name]
        return super()._loader(name)


_template = ScenarioRenderer('scenario_templates')
_template_vars = None


def _make_template_vars(scenario_name):
    global _template_vars
    if not _template_vars:
        _, template = _template.render(
            'config', scenario_name=scenario_name, username=getpass.getuser(),
        )
        _template_vars = json.loads(template)
    return _template_vars


def render_hollywood_scenario(scenario_name):
    template_vars = _make_template_vars(scenario_name)
    for name in resource.iterkeys(_template.path, strip_prefix=True):
        yield _template.render(name.lstrip('/'), template_vars)


def render_scenario_config(scenario_name, **kwargs):
    template_vars = _make_template_vars(scenario_name)
    return _template.render(scenario_config.filename, template_vars, **kwargs)
