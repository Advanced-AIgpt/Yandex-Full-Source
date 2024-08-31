from collections import defaultdict
from typing import Dict, FrozenSet, List, Optional, Set

import itertools
import logging
import multiprocessing as mp
import random

import alice.nlu.proto.dataset_info.dataset_info_pb2 as dataset_info_pb
import yt.wrapper

import click
import google.protobuf.json_format as pb_json


logger = logging.getLogger(__name__)


@yt.wrapper.yt_dataclass
class RawQueryRow:
    is_query: Optional[str]

    # query columns
    query: Optional[str]
    utterance_text_translated: Optional[str]

    # translation columns
    query_en_translation: Optional[str]
    query_ru_translation: Optional[str]
    utterance_text: Optional[str]  # expected to be original non-translated query


@yt.wrapper.yt_dataclass
class Query:
    text: str
    en_translated_text: Optional[str]
    ru_translated_text: Optional[str]
    source: str

    def __hash__(self) -> int:
        return hash(self.text)

    def __eq__(self, other) -> bool:
        return isinstance(other, Query) and self.text == other.text


@yt.wrapper.yt_dataclass
class ClassifiedQuery(Query):
    intents: FrozenSet[str]

    def __hash__(self) -> int:
        return hash((super().__hash__(), self.intents))

    def __eq__(self, other) -> bool:
        return isinstance(other, ClassifiedQuery) and super().__eq__(other) and self.intents == other.intents


@yt.wrapper.yt_dataclass
class QueryWithSingleIntent(Query):
    intent: str

    def __hash__(self) -> int:
        return hash((super().__hash__(), self.intent))

    def __eq__(self, other) -> bool:
        return isinstance(other, QueryWithSingleIntent) and super().__eq__(other) and self.intent == other.intent


@yt.wrapper.yt_dataclass
class IntentQueries:
    dev: List[Query]
    accept: List[Query]
    kpi: List[Query]


def postprocess_text(text: str) -> str:
    return text.strip().lower()


def read_queries_from_yt(client: yt.wrapper.YtClient, table_path: str) -> List[Query]:
    ret = []

    if not client.exists(table_path):
        raise ValueError(f'{table_path} does not exist')

    if client.is_empty(table_path):
        raise ValueError(f'{table_path} is empty')

    for row in client.read_table_structured(table_path, RawQueryRow, unordered=True):
        row: RawQueryRow
        if row.is_query is None and not ret:
            logger.warning('is_query column not present in table %s', table_path)
        elif row.is_query is not None and row.is_query != 'yes':
            continue

        if row.query is not None:
            text = row.query
        elif row.utterance_text_translated is not None:
            text = row.utterance_text_translated
        else:
            logger.error('query column not present in table %s', table_path)
            break

        text = postprocess_text(text)

        en_text = None
        if row.query_en_translation is not None:
            en_text = row.query_en_translation

        ru_text = None
        if row.query_ru_translation is not None:
            ru_text = row.query_ru_translation
        elif row.utterance_text is not None:
            ru_text = row.utterance_text

        ret.append(Query(text, en_text, ru_text, table_path))

    if not ret:
        logger.warning('no positive queries were read from %s', table_path)

    return ret


def read_single_intent_queries(cluster: str, intent_name: str, intent_datasets: dataset_info_pb.TDatasetInfoList) -> IntentQueries:
    client = yt.wrapper.YtClient(proxy=cluster)

    dev, accept, kpi = [], [], []

    logging.info('reading %s intent datasets', intent_name)

    for intent_dataset in intent_datasets.DatasetInfos:
        intent_dataset: dataset_info_pb.TDatasetInfo

        if intent_dataset.Deprecated:
            continue

        if intent_dataset.HasField('DevDataTable'):
            dev.extend(read_queries_from_yt(client, intent_dataset.DevDataTable))

        if intent_dataset.HasField('AcceptDataTable'):
            accept.extend(read_queries_from_yt(client, intent_dataset.AcceptDataTable))

        if intent_dataset.HasField('KpiDataTable'):
            kpi.extend(read_queries_from_yt(client, intent_dataset.KpiDataTable))

    logging.info('finished reading %s intent datasets', intent_name)

    return IntentQueries(dev, accept, kpi)


def read_queries(
    cluster: str,
    datasets_info: dataset_info_pb.TIntentDatasets,
    ignore_intents: List[str],
    processes: Optional[int],
) -> Dict[str, IntentQueries]:

    intent_names, intent_infos = [], []

    for intent_name, intent_info in datasets_info.IntentDatasets.items():
        if intent_name in ignore_intents:
            logging.info('ignoring %s intent', intent_name)
            continue

        intent_names.append(intent_name)
        intent_infos.append(intent_info)

    with mp.Pool(processes) as pool:
        intent_queries = pool.starmap(read_single_intent_queries, zip(itertools.repeat(cluster), intent_names, intent_infos))

    return dict(zip(intent_names, intent_queries))


def classify_texts(
    intent2queries: Dict[str, IntentQueries],
    negative_intent_sources: List[str],
    negative_intent_name: str,
) -> Dict[str, Set[str]]:

    text2target = defaultdict(set)

    for intent, intent_queries in intent2queries.items():
        if intent in negative_intent_sources:
            intent = negative_intent_name

        for query in itertools.chain(intent_queries.dev, intent_queries.accept, intent_queries.kpi):
            text2target[query.text].add(intent)

    for query, intents in text2target.items():
        if negative_intent_name in intents and len(intents) > 1:
            intents.discard(negative_intent_name)

    return text2target


def take_dev_texts(intent2queries: Dict[str, IntentQueries], text2intents: Dict[str, Set[str]]) -> Dict[str, Set[str]]:
    dev_target = {}

    for intent, queries in intent2queries.items():
        for query in queries.dev:
            dev_target[query.text] = text2intents[query.text]

    return dev_target


def intent_stratified_split(text2target: Dict[str, Set[str]], train_size: float, val_size: float, seed: int):
    intents2texts: Dict[FrozenSet[str], List[str]] = defaultdict(list)

    for query, intents in text2target.items():
        intents2texts[frozenset(intents)].append(query)

    train, val, test = {}, {}, {}

    logger.info('\t'.join(['intents', 'train_elements', 'val_elements', 'test_elements']))

    for intents in sorted(intents2texts):
        texts = intents2texts[intents]
        texts.sort()
        random.Random(seed).shuffle(texts)

        train_elements = int(train_size * len(texts))
        val_elements = int(val_size * len(texts))
        test_elements = len(texts) - train_elements - val_elements

        train.update(dict.fromkeys(texts[:train_elements], intents))
        val.update(dict.fromkeys(texts[train_elements: train_elements + val_elements], intents))
        test.update(dict.fromkeys(texts[train_elements + val_elements:], intents))

        logger.info('%s\t%d\t%d\t%d', ';'.join(intents), train_elements, val_elements, test_elements)

    return train, val, test


def make_classified_queries(texts2target: Dict[str, Set[str]], intent2queries: Dict[str, IntentQueries]) -> Set[ClassifiedQuery]:
    classified_queries = set()

    for intent, queries in intent2queries.items():
        for query in queries.dev:
            target = texts2target.get(query.text)

            if target is None:
                continue

            classified_queries.add(ClassifiedQuery(
                text=query.text,
                en_translated_text=query.en_translated_text,
                ru_translated_text=query.ru_translated_text,
                source=query.source,
                intents=frozenset(target),
            ))

    return classified_queries


def validate_part_size(size: float):
    if not (0 <= size <= 1):
        raise ValueError(f'part size not in [0, 1]: {size}')


def upload_queries(cluster: str, table_path: str, queries: Set[ClassifiedQuery]):
    queries_with_single_intent = []

    for query in queries:
        for intent in query.intents:
            single_intent_query = QueryWithSingleIntent(
                text=query.text,
                intent=intent,
                en_translated_text=query.en_translated_text,
                ru_translated_text=query.ru_translated_text,
                source=query.source,
            )
            queries_with_single_intent.append(single_intent_query)

    client = yt.wrapper.YtClient(proxy=cluster)
    client.write_table_structured(table_path, QueryWithSingleIntent, queries_with_single_intent)
    client.run_sort(table_path, sort_by=['intent', 'text'])


@click.command()
@click.option('--datasets-info-file', '-i', required=True, type=click.Path(exists=True, dir_okay=False))
@click.option('--output-train-table', required=True)
@click.option('--output-val-table', required=True)
@click.option('--output-test-table', required=True)
@click.option('--negative-source', '-n', multiple=True)
@click.option('--ignore-intent', multiple=True)
@click.option('--negative-intent-name', default='other')
@click.option('--train-size', type=float, default=0.6)
@click.option('--val-size', type=float, default=0.2)
@click.option('--cluster', default='hahn')
@click.option('--seed', default=101, type=int)
@click.option('--processes', default=4, type=int)
def main(
    datasets_info_file, output_train_table, output_val_table, output_test_table,
    negative_source, ignore_intent, negative_intent_name, train_size, val_size, cluster, seed, processes
):
    logging.basicConfig(level=logging.INFO)

    with open(datasets_info_file, 'r', encoding='utf-8') as f:
        datasets_info = pb_json.Parse(f.read(), dataset_info_pb.TIntentDatasets())

    validate_part_size(train_size)
    validate_part_size(val_size)

    intent2queries = read_queries(cluster, datasets_info, ignore_intent, processes)

    # we make target for texts using dev + accept + kpi as split of each basket is independent from other splits
    text2target = classify_texts(intent2queries, negative_source, negative_intent_name)
    dev_target = take_dev_texts(intent2queries, text2target)
    train_texts, val_texts, test_texts = intent_stratified_split(dev_target, train_size, val_size, seed)

    assert not set(train_texts).intersection(val_texts)
    assert not set(train_texts).intersection(test_texts)
    assert not set(val_texts).intersection(test_texts)

    for part_texts, part_output_table in [
        (train_texts, output_train_table),
        (val_texts, output_val_table),
        (test_texts, output_test_table),
    ]:
        upload_queries(cluster, part_output_table, make_classified_queries(part_texts, intent2queries))
