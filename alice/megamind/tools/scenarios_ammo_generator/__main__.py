# coding: utf-8

import json
import logging
import os

from typing import List
from urllib.parse import urljoin

import attr
import click

from tqdm import tqdm

from alice.megamind.protos.scenarios.request_pb2 import (
    TScenarioApplyRequest, TScenarioRunRequest
)

logging.basicConfig(format='%(asctime)s > %(message)s')

logger = logging.getLogger(__name__)
logger.setLevel(logging.INFO)


def filter_list(list_object, filter_object):
    if filter_object:
        return [e for e in list_object if e in filter_object]
    return list_object


@attr.s
class Config:
    handler: str = attr.ib(default='/dev/null')
    allowed_scenarios: List = attr.ib(default=None)
    allowed_methods: List = attr.ib(default=None)
    experiments: dict = attr.ib(default=attr.Factory(dict))
    output_path: str = attr.ib(default='scenario_requests')
    host: str = attr.ib(default='127.0.0.1')
    headers: dict = attr.ib(default=attr.Factory(dict))

    def __attrs_post_init__(self):
        if not self.handler.endswith('/'):
            self.handler += '/'

    @classmethod
    def load(cls, config):
        if config is not None:
            return cls(**json.load(config))
        return cls()


class AmmoGenerator:
    def __init__(self, path: str, config: Config):
        self._config: Config = config
        self._path: str = path
        self._request_deserializers = {
            'run': TScenarioRunRequest.FromString,
            'apply': TScenarioApplyRequest.FromString,
            'commit': TScenarioApplyRequest.FromString,
        }
        headers = {
            'Content-Type': 'application/protobuf',
            'HOST': config.host,
            **config.headers
        }
        self._headers = '\r\n'.join(f'{k}: {v}' for k, v in headers.items())

    def process(self):
        for scenario in self._load_scenarios():
            scenario_path = os.path.join(self._path, scenario)
            output_folder = os.path.join(self._config.output_path, scenario)
            if not os.path.exists(output_folder):
                os.makedirs(output_folder)
            for method in self._load_methods(scenario_path):
                logger.info(f'Processing method {method} for {scenario} scenario')
                method_path = os.path.join(scenario_path, method)
                output_path = os.path.join(output_folder, f'{method}.ammo')
                with open(output_path, 'wb+') as out:
                    for request in self._load_requests(method_path):
                        out.write(self._make_bullet(method, request))
                    out.write(b'0\r\n')

    def _load_scenarios(self):
        scenarios = tuple(os.listdir(self._path))
        for scenario in filter_list(scenarios, self._config.allowed_scenarios):
            yield scenario

    def _load_methods(self, path):
        methods = tuple(os.listdir(path))
        for method in filter_list(methods, self._config.allowed_methods):
            yield method

    @staticmethod
    def _load_requests(path):
        for request in tqdm(os.listdir(path)):
            request_path = os.path.join(path, request)
            if not os.path.isfile(request_path):
                continue
            with open(request_path, 'rb') as f:
                yield f.read()

    def _make_bullet(self, method, request):
        request_body = self._request_deserializers[method](request)
        for k, v in self._config.experiments.items():
            request_body.BaseRequest.Experiments[k] = v

        content = request_body.SerializeToString()
        result = (
            f'POST {urljoin(self._config.handler, method)} HTTP/1.1\r\n'
            f'{self._headers}\r\n'
            f'Content-Length: {len(content)}\r\n'
            '\r\n'
        ).encode('utf-8')
        result += content
        result += '\r\n\r\n'.encode('utf-8')
        return f'{len(result)} {method}\n'.encode('utf-8') + result


@click.command()
@click.argument('dumps_path', type=click.Path(exists=True, file_okay=False))
@click.option('--config', type=click.File('r'), help='Path to config')
def main(dumps_path, config):
    config = Config.load(config)
    logger.info(f'Using following config: {json.dumps(attr.asdict(config), ensure_ascii=False, indent=2)}')
    AmmoGenerator(dumps_path, config).process()


if __name__ == '__main__':
    main()
