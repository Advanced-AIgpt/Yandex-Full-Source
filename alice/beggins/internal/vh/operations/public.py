import json
import pathlib
import re
import os
from collections import Counter
from typing import List, Set, NamedTuple, Sequence, Union

import vh3
import yaml
from alice.beggins.cmd.manifestator.lib.process import (
    process_manifest as process_manifest_impl,
    process_dataset_manifest as process_dataset_manifest_impl,
)

from vh3 import MRTable, Number, runtime, mr_run_base, MultipleStrings, Binary, File, String, JSON
from vh3.decorator import operation, nirvana_output_names, autorelease_to_nirvana_on_trunk_commit

import yt.wrapper as yt

import pulsar


class RegexpGenerator:
    def __init__(self, samples: List[str], exclude_words: Set[str], word_similarity_threshold: float = 0.75):
        samples = samples
        words = [word.lower() for sample in samples for word in sample.split(' ') if word]
        if len(words) == 0:
            raise ValueError("samples don't have any word")
        dictionary = [word for word, _ in
                      sorted(Counter(words).items(), key=lambda word_freq: word_freq[1], reverse=True)]
        self._word_similarity_threshold = word_similarity_threshold
        self._patterns = list()
        self._generate_patterns(dictionary, samples, exclude_words)

    def _generate_patterns(self, dictionary: List[str], samples: List[str], exclude_words: Set[str]):
        word_set = set()
        for word in dictionary:
            if len(word) == 1 or word in exclude_words:
                continue
            words_to_add = set()
            words_to_rm = set()
            for w in word_set:
                similarity = self._get_word_similarity(w, word)
                score = similarity / max(len(w), len(word))
                if score >= self._word_similarity_threshold:
                    words_to_add.add(word[:similarity])
                    words_to_rm.add(w)
            if words_to_add:
                for w in words_to_rm:
                    word_set.remove(w)
                for w in words_to_add:
                    word_set.add(w)
            else:
                word_set.add(word)
            pattern = RegexpGenerator._word_set_to_pattern(word_set)
            coverage = RegexpGenerator._coverage(pattern, samples)
            self._patterns.append((pattern, coverage))
        if len(self._patterns) == 0:
            raise ValueError('unable to produce any pattern')

    @staticmethod
    def _get_word_similarity(lhs, rhs) -> int:
        return sum(left == right for left, right in zip(lhs, rhs))

    def get_pattern(self, threshold: float) -> str:
        if threshold <= 0 or threshold > 1:
            raise ValueError('threshold value must be in range (0, 1]')
        left, right = 0, len(self._patterns) - 1
        while left + 1 < right:
            pivot = (left + right) // 2
            pattern, coverage = self._patterns[pivot]
            if coverage < threshold:
                left = pivot
            else:
                right = pivot
        return self._patterns[right][0]

    @staticmethod
    def _word_set_to_pattern(word_set: Set[str]) -> str:
        return '({})'.format('|'.join(word_set))

    @staticmethod
    def _coverage(pattern: str, samples: List[str]) -> float:
        if RegexpGenerator.test(pattern, sample='') or len(samples) == 0:
            return 0
        return sum(RegexpGenerator.test(pattern, sample) for sample in samples) / len(samples)

    @staticmethod
    def test(pattern, sample) -> bool:
        return bool(re.search(pattern, sample.lower()))


@operation(
    mr_run_base,
    owner='robot-voiceint',
    deterministic=True,
)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/beggins_search_similarities_by_regexp/0.0.4',
    ya_make_folder_path='alice/beggins/internal/vh/cmd/process_manifest',
)
@nirvana_output_names('matches')
def search_similarities_by_regexp(
    samples_table: MRTable[yt.TablePath],
    corpus_table: MRTable[yt.TablePath],
    exclude_words: MultipleStrings = ('алиса',),
    threshold: Number = 0.8,
) -> MRTable:
    samples: List[str] = [row['text'] for row in yt.read_table(samples_table)]
    generator = RegexpGenerator(samples, exclude_words={word.lower() for word in exclude_words})
    pattern = generator.get_pattern(threshold)
    yt.write_table(runtime.get_mr_output_path(),
                   input_stream=(row for row in yt.read_table(corpus_table) if generator.test(pattern, row['text'])))


class ProcessManifestResult(NamedTuple):
    train: MRTable
    accept: MRTable


@operation(
    mr_run_base,
    owner='robot-voiceint',
    deterministic=True,
)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/beggins_process_manifest/0.0.25',
    ya_make_folder_path='alice/beggins/internal/vh/cmd/process_manifest',
)
def process_manifest(
    model_name: String,
    pulsar_token: vh3.Secret,
    manifest: Union[JSON, File, Binary] = None,
    dev: Sequence[MRTable] = (),
    accept: Sequence[MRTable] = (),
) -> ProcessManifestResult:
    os.environ['PULSAR_TOKEN'] = pulsar_token

    if not manifest:
        manifest = dict()
    else:
        with open(manifest.absolute(), 'r') as stream:
            manifest = yaml.load(stream, yaml.Loader)

    if 'data' not in manifest:
        manifest['data'] = {
            'train': {
                'sources': []
            },
            'accept': {
                'sources': []
            }
        }

    for split, tables in (
        ('train', dev),
        ('accept', accept),
    ):
        for table in tables:
            assert isinstance(table, pathlib.Path)
            with open(table.absolute(), 'r') as stream:
                table_meta = json.load(stream)

            manifest['data'][split]['sources'].append({
                'type': 'yt',
                'table': table_meta['table'],
                'proxy': table_meta['cluster'],
                'parser': {
                    'type': 'standard_parser',
                }
            })

    with open('manifest.yaml', 'w') as stream:
        yaml.dump(manifest, stream)

    process_manifest_impl(
        'manifest.yaml',
        runtime.get_mr_output_path('train'),
        runtime.get_mr_output_path('accept'),
    )


@operation(mr_run_base, owner='robot-voiceint', deterministic=True)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/beggins_dataset_storing/0.0.4',
    ya_make_folder_path='alice/beggins/internal/vh/cmd/process_manifest',
)
def dataset_storing(model_name: vh3.String, train: vh3.MRTable, accept: vh3.MRTable, pulsar_token: vh3.Secret) -> vh3.File:
    process_id = runtime.get_run_context().meta.process_uid
    yt_folder = f'//home/alice/beggins/artifacts/classification/{model_name}/{process_id}'

    # storing
    for alias, table in (
        ('train', train),
        ('accept', accept),
    ):
        with open(table.absolute(), 'r') as stream:
            meta = json.load(stream)
            source = meta['table']
            destination = f'{yt_folder}/{alias}'
            yt.create_table(destination, recursive=True, ignore_existing=True)
            yt.write_table(destination, yt.read_table(source))

    # pulsaring :)
    new_dataset = [
        pulsar.datasets.DatasetFile(
            name=f'{alias}/{alias}',
            source_type=pulsar.ESourceType.Yt,
            data_cluster='hahn',
            data_path=f'{yt_folder}/{alias}'
        )
        for alias in ('train', 'accept')
    ]
    client = pulsar.PulsarClient(pulsar_token)
    try:
        dataset = client.datasets.get(alias=model_name)
        dataset.files = new_dataset
        dataset.version = client.datasets.new_version(dataset)
    except Exception:  # It means the specified dataset does not exist
        dataset = pulsar.datasets.Dataset(
            name=f'{model_name} beggins model',
            alias=model_name,
            description=f'Dataset for {model_name} model',
            files=new_dataset,
        )
        dataset.uid = client.datasets.add(dataset)  # Assignment?! For what? Create Pulsar Ticket
        dataset.version = 1  # Again

    # manifesting
    manifest = {'data': {}}
    for alias in ('train', 'accept'):
        table = f'{yt_folder}/{alias}'
        manifest['data'][alias] = {
            'sources': [{
                'type': 'yt',
                'table': table,
                'parser': {
                    'type': 'standard_parser',
                }
            }]
        }
    with open(runtime.get_output_path().absolute(), 'w') as stream:
        stream.write(f'# {runtime.get_run_context().meta.workflow_url}\n\n')
        stream.write(f'# https://pulsar.yandex-team.ru/datasets/{dataset.uid}\n# version: {dataset.version}\n\n')
        yaml.dump(manifest, stream)


@operation(
    mr_run_base,
    owner='robot-voiceint',
    deterministic=True,
)
@autorelease_to_nirvana_on_trunk_commit(
    version='https://nirvana.yandex-team.ru/alias/operation/beggins_process_dataset_manifest/0.0.3',
    ya_make_folder_path='alice/beggins/internal/vh/cmd/process_manifest',
)
def process_dataset_manifest(
    manifest: Union[JSON, File, Binary] = None,
    standard_dataset: Sequence[MRTable] = (),
    analytics_basket: Sequence[MRTable] = (),
    source_name: String = None,
) -> MRTable:
    if not manifest:
        manifest = {
            'sources': []
        }
    else:
        assert isinstance(manifest, pathlib.Path)
        with open(manifest.absolute(), 'r') as stream:
            manifest = yaml.load(stream, yaml.Loader)

    for parser, tables in (
        ('standard_parser', standard_dataset),
        ('analytics_basket_parser', analytics_basket),
    ):
        for table in tables:
            assert isinstance(table, pathlib.Path)
            with open(table.absolute(), 'r') as stream:
                table_meta = json.load(stream)
            manifest['sources'].append({
                'type': 'yt',
                'table': table_meta['table'],
                'proxy': table_meta['cluster'],
                'parser': {
                    'type': parser,
                    'source': source_name,
                }
            })

    output_path = runtime.get_mr_output_path()
    process_dataset_manifest_impl(source_name, manifest, output_path)
