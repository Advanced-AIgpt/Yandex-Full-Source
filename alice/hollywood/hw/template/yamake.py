import os

import jinja2
from alice.library.python.utils.arcadia import arcadia_path


all_shard_template = """LIBRARY()

OWNER(
    g:hollywood
)

{% for name, paths in items|dictsort('key') -%}
IF (SCENARIO_{{name|upper}} OR NOT DISABLE_ALL_SCENARIOS)
    PEERDIR(
        {%- for path in paths|unique|sort %}
        {{path}}
        {%- endfor %}
    )
ENDIF()

{% endfor -%}
END()

"""


common_template = """LIBRARY()

OWNER(
    g:hollywood
)

PEERDIR(
    {%- for path in items|unique|sort %}
    {{path}}
    {%- endfor %}
)

END()

"""


tests_template = """OWNER(
    g:hollywood
)

RECURSE_FOR_TESTS(
    {%- for path in items|unique|sort %}
    {{path}}
    {%- endfor %}
)

"""


def _parse_tokens(path, token):
    peerdirs = []
    with arcadia_path(path).open() as stream:
        for line in stream:
            line = line.strip()
            if line.startswith(token):
                line = line.lstrip(token)
                peerdir = [line.lstrip('(').rstrip(')')]
                while line and not line.endswith(')'):
                    peerdir.append(line.lstrip('(').rstrip(')'))
                    line = next(stream).strip()
                peerdirs.append([_ for _ in peerdir if _])
    return peerdirs


def render(path, scenario_path):
    peerdirs = _parse_tokens(path, 'PEERDIR')
    template = common_template if len(peerdirs) == 1 else all_shard_template
    if len(peerdirs) == 1:
        peerdirs = peerdirs[0]
        peerdirs.append(str(scenario_path))
    else:
        def _get_key(paths):
            common_path = os.path.commonpath(paths)
            return os.path.basename(common_path)
        peerdirs.append([scenario_path])
        peerdirs = {_get_key(_): _ for _ in peerdirs}
    return jinja2.Template(template).render(items=peerdirs)


def render_for_test(path, scenario_path):
    tests = _parse_tokens(path, 'RECURSE_FOR_TESTS')[0]
    tests.append(scenario_path)
    return jinja2.Template(tests_template).render(items=tests)
