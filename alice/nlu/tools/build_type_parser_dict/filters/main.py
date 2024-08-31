# coding: utf-8

import argparse
import logging
import json

from multiprocessing import Queue, Process

from vins_core.common.sample import Sample
from vins_core.ner.fst_presets import FstParserFactory


logger = logging.getLogger(__name__)


_FST_CONFIG = {
    'parsers': ['time'],
    'resource': 'resource://fst'
}

_NUM_PROCS = 64


def _process_sample(line, parser):
    text = line.split('\t')[0]
    sample = Sample.from_string(text)
    parsed_entities = parser(sample)
    parsed_entities = [
        entity for entity in parsed_entities
        if entity.start == 0 and entity.end == len(sample.tokens)
    ]

    assert len(parsed_entities) <= 1
    if len(parsed_entities) == 1:
        parsed_entities = json.dumps(parsed_entities[0].value)
    else:
        parsed_entities = ''

    return line, parsed_entities


def _worker(process_id, in_queue, out_queue, parser):
    counter = 0
    while True:
        sample = in_queue.get()
        if sample is None:
            return

        out_queue.put(_process_sample(sample, parser))

        counter += 1
        if process_id == 0 and counter > 0 and counter % 10000 == 0:
            logger.info('Processed: ~%s samples', counter * _NUM_PROCS)


def _collect_fst_entities(lines):
    factory = FstParserFactory.from_config(_FST_CONFIG)
    factory.load()
    parser = factory.create_parser(fst_parsers=['time'])

    in_queue, out_queue = Queue(), Queue()

    processes = [
        Process(target=_worker, args=(process_id, in_queue, out_queue, parser))
        for process_id in xrange(_NUM_PROCS)
    ]
    for process in processes:
        process.daemon = True
        process.start()

    for line in lines:
        in_queue.put(line)

    for _ in xrange(_NUM_PROCS):
        in_queue.put(None)

    results = [out_queue.get() for _ in xrange(len(lines))]
    results = [sample for sample in results if sample]

    for process in processes:
        process.join()

    return results


def main():
    logging.basicConfig(level=logging.INFO, format='[%(asctime)s] [%(name)s] [%(levelname)s] %(message)s')

    parser = argparse.ArgumentParser()
    parser.add_argument('--input', required=True)

    args = parser.parse_args()

    with open(args.input) as f:
        lines = [line.strip() for line in f if line.strip()]

    lines_with_fst_entities = _collect_fst_entities(lines)

    with open(args.input, 'w') as f:
        for line, parsed_entity in lines_with_fst_entities:
            entity_source_text, entity_value = line.split('\t')
            if parsed_entity:
                entity_value = parsed_entity
            f.write('{}\t{}\n'.format(entity_source_text, entity_value))


if __name__ == '__main__':
    main()
