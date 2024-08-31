import json

import requests
import yarl
from tornado.web import StaticFileHandler

from jupytercloud.backend.lib.util.misc import get_arcadia_root


PREFIX = 'resfs/file/'
JUPYTERCLOUD_PATH = 'jupytercloud/backend'
RESOURCE_ROOT = PREFIX + JUPYTERCLOUD_PATH
STATIC_PATH = RESOURCE_ROOT + '/static'


def get_templates_path():
    return STATIC_PATH + '/templates'


def get_static_path():
    return STATIC_PATH


def get_nb_templates_path():
    return STATIC_PATH + '/nb_templates'


class WebpackAssets:
    arcadia_dist_dir = 'jupytercloud/backend/static/node/dist'
    local_dist_prefix = yarl.URL('/local_dist/')

    def __init__(self, *, webpack_assets_url, external_js):
        self.local_dist_dir = None
        self.webpack_assets_data = None
        self.webpack_assets_url = webpack_assets_url

        if webpack_assets_url:
            response = requests.get(webpack_assets_url)
            response.raise_for_status()
            self.webpack_assets_data = response.json()

            base_url = self.webpack_assets_url.parent
        else:
            arcadia_root = get_arcadia_root()
            self.local_dist_dir = arcadia_root / self.arcadia_dist_dir

            webpack_path = self.local_dist_dir / 'webpack-assets.json'
            if not webpack_path.exists():
                raise RuntimeError(
                    f'missing webpack-assets.json file at {webpack_path} or missing '
                    'webpack_assets_url config parameter',
                )

            with webpack_path.open() as file_:
                self.webpack_assets_data = json.load(file_)

            base_url = self.local_dist_prefix

        self.external_js = external_js
        self.js = [
            base_url / path.lstrip('/')
            for path in [self.webpack_assets_data['main']['js']]
        ]

        self.css = [
            base_url / path.lstrip('/')
            for path in [self.webpack_assets_data['main']['css']]
        ]

        assert self.js
        assert self.css

    def build_js(self):
        return '\n'.join(
            f'<script type="text/javascript" charset="utf-8" src="{js_path}"></script>'
            for js_path in self.js
        )

    def build_external_js(self):
        return '\n'.join(
            f'<script type="text/javascript" charset="utf-8" src="{js_path}" crossorigin></script>'
            for js_path in self.external_js
        )

    def build_css(self):
        return '\n'.join(
            f'<link rel="stylesheet" href="{css_path}" type="text/css"/>'
            for css_path in self.css
        )

    def generate_handlers(self):
        if self.webpack_assets_url:
            return []

        return [
            (rf'{self.local_dist_prefix}(.*)', StaticFileHandler, {'path': str(self.local_dist_dir)}),
        ]
