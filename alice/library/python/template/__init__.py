import json
from pathlib import Path
from operator import itemgetter

import jinja2
from library.python import resource


def _jsonify(value):
    return json.dumps(value, sort_keys=True)


def _to_list(value):
    return [_ for _ in value]


def _sort_tuple(value, *indexes):
    value.sort(key=itemgetter(*indexes))
    return value


class Renderer(object):

    def __init__(self, templates, use_deprecated=False):
        self._path = Path(templates)
        self._deprecated = use_deprecated
        self._jinja_env = jinja2.Environment(
            extensions=['jinja2.ext.do'],
            loader=jinja2.FunctionLoader(self._loader),
        )
        self._jinja_env.filters['jsonify'] = _jsonify
        self._jinja_env.filters['to_list'] = _to_list
        self._jinja_env.filters['sort_tuple'] = _sort_tuple

    @property
    def path(self):
        return f'resfs/file/{self._path}'

    def _loader(self, name):
        path = self._path / 'deprecated' / name
        if not self._deprecated or not resource.resfs_file_exists(str(path)):
            path = self._path / name
        return resource.resfs_read(str(path)).decode()

    def render(self, name, *args, deprecated=None, **kwargs):
        deprecated = self._deprecated if deprecated is None else deprecated
        self._deprecated, deprecated = deprecated, self._deprecated
        filename = jinja2.Template(name).render(*args, **kwargs)
        template = self._jinja_env.get_template(name).render(*args, **kwargs)
        self._deprecated, deprecated = deprecated, self._deprecated
        return filename, template
