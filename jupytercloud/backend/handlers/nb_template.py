import itertools
import pathlib
import urllib.parse
from collections import OrderedDict

import jupytext
from jinja2 import Environment
from jupyterhub.handlers.static import ResourceLoader
from jupyterhub.utils import url_path_join
from nbconvert import HTMLExporter
from nbconvert.filters.markdown_mistune import IPythonRenderer, MarkdownWithMath
from pygments.formatters import HtmlFormatter
from tornado import web

from jupytercloud.backend.handlers.base import JCPageHandler
from jupytercloud.backend.handlers.utils import NAME_RE, admin_or_self
from jupytercloud.backend.lib.util.paths import get_safe_path
from jupytercloud.backend.static import get_nb_templates_path


class PatchedIPythonRenderer(IPythonRenderer):
    def header(self, text, level, raw=None):
        return super(IPythonRenderer, self).header(text, level, raw=raw)

    def link(self, link, title, text):
        link = super().link(link, title, text)
        return link.replace('<a', '<a target="_blank" ')

    @classmethod
    def markdown2html(cls, source):
        return MarkdownWithMath(
            renderer=cls(
                escape=False,
            ),
        ).render(source)


class NBTemplateHandlerBase(JCPageHandler):
    nb_get_parameters = ()
    nb_template_prefix = None
    nb_save_dir = None
    nb_template_ext = '.pyt'
    nb_template_format = {
        'format_name': 'percent',
        'extension': '.py',
        'comment_magics': False,
    }
    nb_vars = {}
    highlight_style = 'default'

    @property
    def nb_template_dir(self):
        return pathlib.Path(get_nb_templates_path()) / self.nb_template_prefix

    # NB: It stores cache on NBTemplateHandlerBase object!
    # It basicly cached class property with exception that you
    # can access it only via class instance
    _html_exporter_cache = {}
    _highlight_css_cache = {}
    _env_cache = {}
    _templates_cache = {}

    @property
    def html_exporter(self):
        key = self.__class__.__name__
        if key not in self._html_exporter_cache:
            loader = ResourceLoader([str(get_nb_templates_path())])

            html_exporter = HTMLExporter(
                extra_loaders=[loader],
                template_file='nbconvert.tpl',
                filters={
                    'markdown2html': PatchedIPythonRenderer.markdown2html,
                },
            )

            self._html_exporter_cache[key] = html_exporter

        return self._html_exporter_cache[key]

    @property
    def highlight_css(self):
        key = self.__class__.__name__
        if key not in self._highlight_css_cache:
            formatter = HtmlFormatter(style=self.highlight_style)
            pygments_css = formatter.get_style_defs('.highlight')
            self._highlight_css_cache[key] = pygments_css

        return self._highlight_css_cache[key]

    @property
    def nb_environment(self):
        key = self.__class__.__name__
        if key not in self._env_cache:
            loader = ResourceLoader([str(self.nb_template_dir)])
            self._env_cache[key] = Environment(
                loader=loader,
                enable_async=True,
            )

        return self._env_cache[key]

    def load_nb_template(self, template_filename):
        template = self.nb_environment.get_template(template_filename)

        template_path = self.nb_template_dir / template_filename
        content = self.nb_environment.loader.get_source(
            environment=self.nb_environment,
            template=template_filename,
        )[0]
        raw_ipynb = jupytext.reads(content, fmt=self.nb_template_format)

        nb_meta = raw_ipynb['metadata'].get('nb_template', {})
        id_ = template_path.stem

        return {
            'template': template,
            'description': nb_meta.get('description_ru') or nb_meta.get('description_en'),
            'name': nb_meta.get('name_ru') or nb_meta.get('name_en') or id_,
            'id': id_,
            'default_weight': nb_meta.get('default_weight', 0),
        }

    @property
    def nb_templates(self):
        name = self.__class__.__name__
        if name not in self._templates_cache:
            templates = self.nb_environment.list_templates()
            raw_dict = {
                pathlib.PurePath(template).stem: self.load_nb_template(template)
                for template in templates
                if template.endswith(self.nb_template_ext)
            }
            self._templates_cache[name] = OrderedDict(
                sorted(raw_dict.items()),  # sort by filename
            )

        return self._templates_cache[name]

    def get_nb_parameters(self, user, template_name):
        nb_parameters = {
            name: self.get_argument(name)
            for name in self.nb_get_parameters
        }
        if 'username' not in nb_parameters:
            nb_parameters['username'] = user.name

        return nb_parameters

    def get_upload_path(self, template_name, nb_parameters):
        raise NotImplementedError

    async def render_and_upload(self, user, template_name):
        if template_name not in self.nb_templates:
            raise web.HTTPError(400, f'unknown template name {template_name}')

        nb_parameters = self.get_nb_parameters(user, template_name)

        template_info = self.nb_templates[template_name]
        template = template_info['template']
        nb_percent = await template.render_async(**nb_parameters)
        nb_dict = jupytext.reads(nb_percent, fmt=self.nb_template_format)
        nb_ipynb = jupytext.writes(nb_dict, fmt='ipynb')

        raw_target_path = self.get_upload_path(template_name, nb_parameters)
        target_path = await self.write_file_for_user(user, raw_target_path, nb_ipynb)
        next_url = self.get_next_url_notebook(user, target_path)

        self.redirect(next_url)

    async def render_ipynb(self, user, template_name):
        nb_parameters = self.get_nb_parameters(user, template_name)

        template_info = self.nb_templates[template_name]
        template = template_info['template']
        nb_percent = await template.render_async(**nb_parameters)
        nb_dict = jupytext.reads(nb_percent, fmt=self.nb_template_format)

        (body, resources) = self.html_exporter.from_notebook_node(nb_dict)

        return body

    def make_template_link(self, template_name):
        base = self.request.uri
        url = urllib.parse.urlparse(base)
        assert url.path.rstrip('/').endswith(self.nb_template_prefix)
        new_url = url._replace(
            path=url_path_join(url.path, template_name),
        )
        return new_url.geturl()

    @web.authenticated
    @admin_or_self
    async def get(self, name, template_name=None):
        user = self.user_from_username(name)

        if not user.spawner.active:
            next_url = self.get_next_url_through_spawn(
                name, self.request.uri,
            )
            self.redirect(next_url)
        elif template_name:
            await self.render_and_upload(user, template_name)
        else:
            # todo: change to asyncio.gather
            preview = {
                name: await self.render_ipynb(user, name)
                for name in self.nb_templates
            }
            template_links = {
                name: self.make_template_link(name)
                for name in self.nb_templates
            }

            default_template_name, _ = max(
                self.nb_templates.items(),
                key=lambda kv: kv[1]['default_weight'],
            )

            html = await self.render_template(
                'nb_template.html',
                nb_templates=self.nb_templates,
                preview=preview,
                template_links=template_links,
                highlight_css=self.highlight_css,
                default_template_name=default_template_name,
            )
            self.write(html)


class NBTemplateHandlerYT(NBTemplateHandlerBase):
    nb_get_parameters = ('cluster', 'path')
    nb_template_prefix = 'yt'
    nb_save_dir = 'OpenFromYT'

    def get_upload_path(self, template_name, nb_parameters):
        return get_safe_path(
            self.nb_save_dir,
            '{}_{}_{}.ipynb'.format(
                template_name,
                nb_parameters['cluster'],
                nb_parameters['path'],
            ),
        )


class NBTemplateHandlerPulsarExperiment(NBTemplateHandlerBase):
    nb_get_parameters = ('experiment_id', )
    nb_template_prefix = 'pulsar_experiments'
    nb_save_dir = 'OpenFromPulsar'

    def get_upload_path(self, template_name, nb_parameters):
        return get_safe_path(
            self.nb_save_dir,
            '{}_{}.ipynb'.format(
                template_name,
                nb_parameters['experiment_id'],
            ),
        )


class NBTemplateHandlerPulsarInstance(NBTemplateHandlerBase):
    nb_get_parameters = ('instance_id', )
    nb_template_prefix = 'pulsar_instances'
    nb_save_dir = 'OpenFromPulsar'

    def get_upload_path(self, template_name, nb_parameters):
        return get_safe_path(
            self.nb_save_dir,
            '{}_{}.ipynb'.format(
                template_name,
                nb_parameters['instance_id'],
            ),
        )


class AnonymousNBTemplateHandler(JCPageHandler):
    @web.authenticated
    async def get(self, nb_template_prefix, template_name=None):
        if self.current_user is None:
            raise web.HTTPError(403)

        arguments = urllib.parse.urlencode(self.request.query_arguments, doseq=True)

        url = f'/nb_template/{self.current_user.name}/{nb_template_prefix}'
        if template_name:
            url += f'/{template_name}'
        url += f'?{arguments}'

        next_url = self.get_next_url(default=url)

        self.redirect(next_url)


TEMPLATE_TYPE_RE = r'(?P<nb_template_prefix>[^/]+)'
TEMPLATE_NAME_RE = r'(?P<template_name>[^/]+)'
HANDLERS = [NBTemplateHandlerYT, NBTemplateHandlerPulsarExperiment, NBTemplateHandlerPulsarInstance]


def _patterns(handler):
    return (
        (f'/nb_template/{NAME_RE}/{handler.nb_template_prefix}', handler),
        (f'/nb_template/{NAME_RE}/{handler.nb_template_prefix}/{TEMPLATE_NAME_RE}', handler),
    )


# Тут мы гегерим ряд хендеров на каждый класс вида:
# /nb_template/<username>/<template_class>
# /nb_template/<username>/<template_class>/<template_name>
# Идет размножение по template_class, который берется из Handler.nb_template_prefix
default_handlers = list(
    itertools.chain(
        *(
            _patterns(Handler) for Handler in HANDLERS
        ),
    ),
)

default_handlers.extend([
    (
        f'/redirect/nb_template/{TEMPLATE_TYPE_RE}',
        AnonymousNBTemplateHandler,
    ),
    (
        f'/redirect/nb_template/{TEMPLATE_TYPE_RE}/{TEMPLATE_NAME_RE}',
        AnonymousNBTemplateHandler,
    ),
])
