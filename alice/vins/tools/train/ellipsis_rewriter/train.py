# coding: utf-8

import attr
import argparse
import os
import numpy as np
import logging
import json
import random

from model import BatchGenerator, RnnTaggerTrainer


logger = logging.getLogger(__name__)


SPECIAL_SYMBOLS = [
    '[PAD]',
    '[SEP]'
]


@attr.s(frozen=True)
class DatasetConfig(object):
    name = attr.ib()
    paths = attr.ib()
    fraction = attr.ib(default=1., converter=float)


def dataset_list_converter(configs):
    return [DatasetConfig(**config) for config in configs]


@attr.s(frozen=True)
class Config(object):
    train_data = attr.ib(converter=dataset_list_converter)
    test_data = attr.ib(converter=dataset_list_converter)


@attr.s(frozen=True)
class Example(object):
    tokens = attr.ib()
    token_ids = attr.ib()
    tags = attr.ib()
    tag_ids = attr.ib()

    def __len__(self):
        return len(self.tokens)


def get_tags(context, markup):
    context = [line.decode('utf8') for line in context]

    shifts = [0]
    for line in context[:-1]:
        shifts.append(shifts[-1] + len(line.split()) + 1)

    context_length = shifts[-1] + len(context[-1].split())
    tags = ['O' for _ in xrange(context_length)]

    if markup == 'NOTHING_TO_TAG':
        return tags

    for position in markup.split('|'):
        line_index, begin, length = map(int, position.split('~'))
        line, token_index_shift = context[line_index], shifts[line_index]

        if (begin < 0
            or begin + length > len(line)
            or (begin > 0 and line[begin - 1] != ' ')
            or (begin + length < len(line) and line[begin + length] != ' ')
        ):
            logger.warning('Wrong coordinates: %s, %s, %s, %s', line, markup, begin, length)
            return []

        token_index = token_index_shift + len(line[:begin].split())
        token_count = len(line[begin: begin + length].split())

        for i in xrange(token_index, token_index + token_count):
            assert 0 <= i < len(tags), '{} {}'.format(line, markup)
            tags[i] = 'sense'

    return tags


def _read_source_items(path):
    source_items = []

    with open(path) as f:
        f.readline()
        for line in f:
            _, context, markup = line.rstrip().split('\t')[:3]
            context = context.split(',')

            tags = get_tags(context, markup)
            has_only_sense = all(tag.endswith('sense') for tag in tags)
            has_only_nonsense = all(not tag.endswith('sense') for tag in tags)
            context = ' [SEP] '.join(context)
            if (len(tags) != len(context.split())
                or has_only_sense
                or has_only_nonsense
            ):
                logger.warning('Filtered sample: %s, %s', context, markup)
                continue

            source_items.append((context, tags))

    return source_items


def _prepare_examples(source_items, word_to_index):
    examples = []
    for context, tags in source_items:
        tokens = context.split()
        token_ids = [word_to_index.get(token, 0) for token in tokens]

        assert all(tag == 'O' or tag == 'sense' for tag in tags)
        # TODO: check whether we need +1 here (right now it's to address padding elements)
        tag_ids = [int(tag == 'sense') + 1 for tag in tags]

        assert len(tokens) == len(tags) == len(token_ids) == len(tag_ids)

        examples.append(Example(
            tokens=tokens,
            token_ids=token_ids,
            tags=tags,
            tag_ids=tag_ids
        ))

    return examples


def _read_data(data_dir, dataset_configs, word_to_index):
    examples = []
    for dataset_config in dataset_configs:
        source_items = []
        for path in dataset_config.paths:
            source_items.extend(_read_source_items(os.path.join(data_dir, path)))

        if dataset_config.fraction != 1.:
            random.shuffle(source_items)
            source_items = source_items[: int(len(source_items) * dataset_config.fraction)]

        examples.extend(_prepare_examples(source_items, word_to_index))

    return examples


def _get_embeddings(embeddings_dir):
    embeddings_matrix = np.load(os.path.join(embeddings_dir, 'embeddings.npy'))
    embeddings_matrix = np.concatenate((np.zeros((len(SPECIAL_SYMBOLS), 300)), embeddings_matrix), 0)
    with open(os.path.join(embeddings_dir, 'embeddings.dict')) as f:
        index_to_word = SPECIAL_SYMBOLS + [line.rstrip() for line in f]
        word_to_index = {word: index for index, word in enumerate(index_to_word)}

    return embeddings_matrix, index_to_word, word_to_index


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--data-dir', required=True)
    parser.add_argument('--embeddings-dir', required=True)
    parser.add_argument('--output-path', default='models')

    args = parser.parse_args()

    with open(os.path.join(args.data_dir, 'train_config.json')) as f:
        train_config = Config(**json.load(f))

    embeddings_matrix, _, word_to_index = _get_embeddings(args.embeddings_dir)

    examples = _read_data(args.data_dir, train_config.train_data, word_to_index)

    logger.info('Examples:')
    for example in examples[0:1000:50]:
        logger.info(json.dumps(attr.asdict(example), ensure_ascii=False, encoding='utf-8'))

    logger.info('Example count: %s', len(examples))

    generator = BatchGenerator(examples, batch_size=32)

    logger.info('Batch: %s', next(generator))

    trainer = RnnTaggerTrainer(embeddings_matrix=embeddings_matrix, epochs_count=5)
    trainer.fit(generator)
    trainer.save(args.output_path)


if __name__ == "__main__":
    main()
