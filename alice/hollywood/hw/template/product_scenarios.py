import jinja2
from alice.library.python.utils.arcadia import arcadia_path


_item_template = 'inline const TString {{name|upper}} = "{{name}}";\n\n'
_template = '{{header|join}}{{items|unique|sort|join}}{{footer|join}}'


def _parse_tokens(path, token):
    items = []
    header = []
    footer = []
    with arcadia_path(path).open() as stream:
        for line in stream:
            while line.startswith(token):
                items.append(line)
                line = next(stream)
            if items:
                footer.append(line)
            else:
                header.append(line)
    return header, items, footer


def render(path, scenario_name):
    header, items, footer = _parse_tokens(path, 'inline const')
    items.append(jinja2.Template(_item_template).render(name=scenario_name))
    return jinja2.Template(_template).render(header=header, items=items, footer=footer)
