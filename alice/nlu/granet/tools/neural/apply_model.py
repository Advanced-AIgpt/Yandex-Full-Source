# coding: utf-8

from __future__ import unicode_literals

import argparse
import attr
import codecs
import logging
import os

from multiprocessing import Queue, Process
from utils import TokenEmbedder, iterate_lines, parse_nlu_item, to_nlu_line, load_embeddings

_SIMPLE_TAGGER = False

if _SIMPLE_TAGGER:
    from vins_models_tf import TfClassifyingSimpleRnnTagger as Tagger
else:
    from vins_models_tf import TfClassifyingRnnTagger as Tagger

logger = logging.getLogger(__name__)


@attr.s
class Example(object):
    tokens = attr.ib()
    weight = attr.ib()
    expected_is_positive = attr.ib()
    expected_tags = attr.ib()
    predicted_is_positive = attr.ib(default=None)
    prediction_prob = attr.ib(default=0.)
    predicted_tags = attr.ib(default=None)


@attr.s(frozen=True)
class Sample(object):
    tokens = attr.ib()
    tags = attr.ib()


@attr.s(frozen=True)
class SampleFeatures(object):
    sample = attr.ib()
    dense_seq = attr.ib(factory=dict)
    sparse_seq = attr.ib(factory=dict)


def _read_data(path, expected_is_positive):
    examples = []
    for weight, text in iterate_lines(path):
        tokens, tags = parse_nlu_item(text)
        examples.append(Example(
            tokens=tokens,
            weight=int(weight),
            expected_is_positive=expected_is_positive,
            expected_tags=tags
        ))

    return examples


def _extract_features(example, embedder):
    embeddings = embedder(example.tokens)

    return SampleFeatures(
        sample=Sample(tokens=example.tokens, tags=example.expected_tags),
        dense_seq={'alice_requests_emb': embeddings}
    )


def _process_example(model, embedder, example):
    sample_features = _extract_features(example, embedder)
    assert len(sample_features.dense_seq['alice_requests_emb']) == len(example.tokens)

    if _SIMPLE_TAGGER:
        prediction = model.predict([sample_features])[0]
    else:
        prediction = model.predict([sample_features], 1, 10)[0][0]

    example.prediction_prob = prediction.class_probability
    example.predicted_is_positive = prediction.is_from_this_class
    example.predicted_tags = prediction.tags

    return example


def _examples_processor(model, embedder, process_id, in_queue, out_queue):
    counter = 0
    while True:
        example = in_queue.get()
        if example is None:
            return

        out_queue.put(_process_example(model, embedder, example))

        counter += 1
        if counter > 0 and counter % 10000 == 0:
            logger.info('[Process %s] Processed: %s examples', process_id, counter)


def _process_examples(model, embedder, examples, num_procs):
    if num_procs == 1:
        return [_process_example(model, embedder, example) for example in examples]

    in_queue, out_queue = Queue(), Queue()

    processes = [
        Process(target=_examples_processor, args=(model, embedder, process_id, in_queue, out_queue))
        for process_id in xrange(num_procs)
    ]
    for process in processes:
        process.daemon = True
        process.start()

    for sample in examples:
        in_queue.put(sample)

    for _ in xrange(num_procs):
        in_queue.put(None)

    processed_examples = [out_queue.get() for _ in xrange(len(examples))]

    for process in processes:
        process.join()

    return processed_examples


def _write_results(examples, output_path):
    if not os.path.isdir(output_path):
        os.makedirs(output_path)

    examples = sorted(examples, key=lambda example: example.weight, reverse=True)

    with codecs.open(os.path.join(output_path, 'full_results.tsv'), 'w', encoding='utf8') as f:
        f.write('Weight\tExpected parse\tPredicted parse\t'
                'Expected positiveness\tPredicted positiveness\tProbability\n')
        for example in examples:
            f.write('{weight}\t{expected_parse}\t{predicted_parse}\t'
                    '{expected_is_positive}\t{predicted_is_positive}\t{prob}\n'.format(
                        weight=example.weight,
                        expected_parse=to_nlu_line(example.tokens, example.expected_tags),
                        predicted_parse=to_nlu_line(example.tokens, example.predicted_tags),
                        expected_is_positive=example.expected_is_positive,
                        predicted_is_positive=example.predicted_is_positive,
                        prob=example.prediction_prob
                    ))

    with codecs.open(os.path.join(output_path, 'lost_examples.tsv'), 'w', encoding='utf8') as f:
        f.write('Weight\tText\tProbability\n')
        for example in examples:
            if example.expected_is_positive and not example.predicted_is_positive:
                f.write('{weight}\t{text}\t{prob}\n'.format(
                    weight=example.weight,
                    text=to_nlu_line(example.tokens, example.expected_tags),
                    prob=example.prediction_prob
                ))

    with codecs.open(os.path.join(output_path, 'excess_examples.tsv'), 'w', encoding='utf8') as f:
        f.write('Weight\tText\tProbability\n')
        for example in examples:
            if not example.expected_is_positive and example.predicted_is_positive:
                f.write('{weight}\t{text}\t{prob}\n'.format(
                    weight=example.weight,
                    text=to_nlu_line(example.tokens, example.predicted_tags),
                    prob=example.prediction_prob
                ))

    with codecs.open(os.path.join(output_path, 'mistagged_examples.tsv'), 'w', encoding='utf8') as f:
        f.write('Weight\tExpected\tPredicted\n')
        for example in examples:
            if not example.expected_is_positive or not example.predicted_is_positive:
                continue
            if example.predicted_tags != example.expected_tags:
                f.write('{weight}\t{expected}\t{predicted}\n'.format(
                    weight=example.weight,
                    expected=to_nlu_line(example.tokens, example.expected_tags),
                    predicted=to_nlu_line(example.tokens, example.predicted_tags),
                ))


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--positives-path', required=True)
    parser.add_argument('--negatives-path', required=True)
    parser.add_argument('--tagger-path', required=True)
    parser.add_argument('--embeddings-dir', required=True)
    parser.add_argument('--num-procs', default=1, type=int)
    parser.add_argument('--output-path', default='results')

    args = parser.parse_args()

    logger.info('Loading embeddings...')
    embeddings_matrix, _, word_to_index = load_embeddings(args.embeddings_dir)

    logger.info('Reading data...')
    examples = _read_data(args.positives_path, expected_is_positive=True)
    examples.extend(_read_data(args.negatives_path, expected_is_positive=False))

    logger.info('Processing %s examples...', len(examples))

    model = Tagger(args.tagger_path)
    embedder = TokenEmbedder(embeddings_matrix, word_to_index)

    examples = _process_examples(model, embedder, examples, args.num_procs)

    _write_results(examples, args.output_path)


if __name__ == "__main__":
    main()
