from random import Random
from typing import List

import pulsar

import yaml
import yt.wrapper as yt

from alice.beggins.cmd.manifestator.internal.data import YtDataSource, LocalFileDataSource
from alice.beggins.cmd.manifestator.internal.dispatcher import (
    Dispatcher, EntriesLimiter as EntriesLimiterDispatcher, Chain as ChainDispatcher,
    PositivesFilter as PositivesFilterDispatcher, NegativesFilter as NegativesFilterDispatcher,
    InvertModifier as InvertModifierDispatcher, Identity as IdentityDispatcher, RandomLimiter
)
from alice.beggins.cmd.manifestator.internal.parser import (
    StandardParser, UnmarkedParser, AnalyticsGeneralParser, Parser, AnalyticsBasketParser,
)
from alice.beggins.cmd.manifestator.internal.manifest import Manifest, Dataset, DataSource, Data


def process(entry):
    known_entries = set()
    for source in entry.sources:
        for item in source.get_items():
            if item.text in known_entries:
                continue
            known_entries.add(item.text)
            yield item.as_dict()


def read_parser(config: dict) -> Parser:
    parser_type = config.get('type')
    if parser_type == 'standard_parser':
        return StandardParser(
            text_key=config.get('text_key'),
            target_key=config.get('target_key'),
            source=config.get('source'),
        )
    if parser_type == 'analytics_general_parser':
        return AnalyticsGeneralParser(
            text_key=config.get('text_key'),
            target_key=config.get('target_key'),
            source=config.get('source'),
        )
    if parser_type == 'unmarked_parser':
        target = config.get('target')
        assert target in (0, 1), 'target must be either 0 or 1'
        return UnmarkedParser(
            target=target,
            text_key=config.get('text_key'),
            source=config.get('source'),
        )
    if parser_type == 'analytics_basket_parser':
        return AnalyticsBasketParser(
            source=config.get('source'),
        )


def read_dispatcher(config: dict, random: Random) -> Dispatcher:
    dispatcher_type = config.get('type')
    if dispatcher_type == 'entries_limiter':
        limit = config.get('limit')
        assert isinstance(limit, int) and limit > 0, f'limit must be positive integer, but found: {limit}'
        return EntriesLimiterDispatcher(limit=limit)
    if dispatcher_type == 'positives_filter':
        return PositivesFilterDispatcher()
    if dispatcher_type == 'negatives_filter':
        return NegativesFilterDispatcher()
    if dispatcher_type == 'invert_modifier':
        return InvertModifierDispatcher()
    if dispatcher_type == 'random_limiter':
        threshold = config.get('threshold')
        assert 0 < threshold < 1, 'threshold must be within the range (0, 1)'
        return RandomLimiter(threshold=threshold, rnd=random)
    raise ValueError(f'unknown dispatcher type: {dispatcher_type}')


def read_dispatchers(configs: List[dict], random: Random) -> Dispatcher:
    if not configs:
        return IdentityDispatcher()
    return ChainDispatcher([read_dispatcher(config, random) for config in configs])


def read_data_source(config: dict, random: Random) -> DataSource:
    kwargs = {
        'dispatcher': read_dispatchers(config.get('dispatchers') or [], random),
        'parser': read_parser(config.get('parser')),
    }
    data_source_type = config.get('type')
    if data_source_type == 'yt':
        table = config.get('table')
        assert table, 'table must be provided to yt data source'
        return YtDataSource(
            table=table,
            proxy=config.get('proxy'),
            **kwargs,
        )
    if data_source_type == 'local_file':
        filepath = config.get('filepath')
        assert filepath, 'filepath must be provided to local file data source'
        return LocalFileDataSource(
            filepath=filepath,
            **kwargs,
        )
    raise ValueError(f'unknown data source type: {data_source_type}')


def read_dataset(name: str, config: dict, random: Random) -> Dataset:
    random = Random(config.get('random_seed', random))
    sources = config.get('sources') or []
    dataset = Dataset(name=name, sources=[read_data_source(source, random) for source in sources])
    return dataset


def process_pulsar(manifest: dict):
    if 'pulsar' not in manifest:
        return

    pulsar_info = manifest['pulsar']

    uid = None
    alias = None
    version = None

    if 'link' in pulsar_info:
        link = pulsar_info['link']
        if not link.startswith('https://pulsar.yandex-team.ru/datasets/'):
            raise Exception('Pulsar link must to start with <https://pulsar.yandex-team.ru/datasets/>')
        buffer = link.removeprefix('https://pulsar.yandex-team.ru/datasets/')
        buffer = buffer.split('?')
        uid = buffer[0]
        if len(buffer) > 1:
            version = buffer[1]
            version = version.removeprefix('version=')
            version = int(version)

    if 'uid' in pulsar_info:
        uid = pulsar_info['uid']
    if 'alias' in pulsar_info:
        alias = pulsar_info['alias']
    if 'version' in pulsar_info:
        version = pulsar_info['version']

    if version is None:
        raise Exception('Not enough dataset info: please specify version of dataset')
    if uid is None and alias is None:
        raise Exception('Not enough dataset info: please specify uid or alias of dataset')

    client = pulsar.PulsarClient()
    if uid is not None:
        dataset = client.datasets.get(uid=uid, version=version)
    else:
        dataset = client.datasets.get(alias=alias, version=version)

    for file in dataset.files:
        if file.source_type is pulsar.ESourceType.Yt:
            dataset_type = None
            if file.name.startswith('train/'):
                dataset_type = 'train'
            if file.name.startswith('accept/'):
                dataset_type = 'accept'
            if dataset_type is None:
                continue
            manifest['data'][dataset_type]['sources'].append({
                'type': 'yt',
                'table': file.data_path,
                'proxy': file.data_cluster,
                'parser': {
                    'type': 'standard_parser',
                    'source': file.name,
                }
            })


def read_manifest(filepath: str) -> Manifest:
    with open(filepath, 'r') as stream:
        config = yaml.load(stream, yaml.Loader)
    process_pulsar(config)
    data = config.get('data')
    assert data is not None, 'no data entry provided in manifest.yaml'
    random = Random(config.get('random_seed', 42))
    return Manifest(
        random=random,
        data=Data(
            train=read_dataset('train', data.get('train'), random),
            accept=read_dataset('accept', data.get('accept'), random),
        )
    )


def make_sources_generator(sources: List[DataSource]):
    known_entries = set()
    for source in sources:
        for item in source.get_entries():
            if item.text in known_entries:
                continue
            known_entries.add(item.text)
            yield item.as_dict()


def write_dataset_table(dataset: Dataset, path):
    yt.create_table(path=path, recursive=True, ignore_existing=True)
    yt.alter_table(path=path, schema=[
        {
            'name': 'text',
            'type': 'string',
            'required': True,
        },
        {
            'name': 'target',
            'type': 'int32',
            'required': True,
        },
        {
            'name': 'source',
            'type': 'string',
        },
    ])
    yt.write_table(path, make_sources_generator(dataset.sources))


def process_manifest(filepath: str, train_table, accept_table):
    manifest = read_manifest(filepath)
    write_dataset_table(manifest.data.train, train_table)
    write_dataset_table(manifest.data.accept, accept_table)


def process_dataset_manifest(name: str, config: dict, table_path):
    dataset = read_dataset(name, config, random=Random(42))
    write_dataset_table(dataset, table_path)
