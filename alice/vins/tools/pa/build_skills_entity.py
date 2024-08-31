# coding: utf-8
"""
This script is used by sandbox task BUILD_VINS_CUSTOM_ENTITY.
Please make sure you fix task code before moving or renaming this file

"""

import argparse
import logging
import os
import sys
import time

from vins_core.ner.fst_normalizer import DEFAULT_RU_NORMALIZER_NAME
from vins_core.utils.data import TarArchive
from vins_core.ext.base import BaseHTTPAPI
from vins_core.nlu.samples_extractor import SamplesExtractor
from vins_core.nlu.sample_processors.normalize import NormalizeSampleProcessor
from vins_tools.nlu.ner.fst_custom import NluFstCustomMorphyConstructor


logger = logging.getLogger(__name__)
session = BaseHTTPAPI(
    timeout=30,
    max_retries=3,
    headers={'content-type': 'application/json'},
)


def compile_data(name, data, path):
    samples_extractor = SamplesExtractor([NormalizeSampleProcessor(DEFAULT_RU_NORMALIZER_NAME)])

    input_data = {id_: [x.text for x in samples_extractor(phrases)]
                  for id_, phrases in data.iteritems()}

    entity = NluFstCustomMorphyConstructor(
        fst_name=name,
        data=input_data,
        inflect_numbers=True,
    ).compile()

    with TarArchive(path, mode='w') as arch:
        entity.save_to_archive(arch)


def collect_data(endpoint):
    request = {
        'jsonrpc': '2.0',
        'id': 666,
        'method': 'getPublishingSkills'
    }
    res = session.post(endpoint, json=request, timeout=30, verify=False)
    res.raise_for_status()

    data = res.json()

    if 'error' in data:
        raise RuntimeError('Error response %s' % data)

    return data['result']


def notify_skills(endpoint, ids):
    request = {
        'jsonrpc': '2.0',
        'id': 666,
        'method': 'setPublishedSkills',
        'params': [ids],
    }

    res = session.post(endpoint, json=request, timeout=30, verify=False)
    res.raise_for_status()

    data = res.json()

    if 'error' in data:
        raise RuntimeError('Error response %s' % data)

    return data['result'] == 'ok'


def compile_custom_entity(endpoint, entity_name, path):
    data = collect_data(endpoint)
    compile_data(entity_name, {i['id']: i['activationPhrases'] for i in data}, path)
    return map(lambda x: x['id'], data)


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument(
        '--name', metavar='NAME',
        help="Custom entity name",
        required=True,
    )
    parser.add_argument(
        '--endpoint', metavar='URL',
        help="Paskills API endpoint",
        required=True,
    )
    parser.add_argument(
        '--out', metavar='PATH',
        help="Path to output file",
        required=False,
    )
    parser.add_argument(
        '--notify', help="Notify paskills API about successful build",
        required=False, default=False, action='store_true',
    )
    parser.add_argument(
        '--notification-timeout', metavar='SEC',
        help="Delay in seconds before notifying skills API",
        default=60 * 5, type=int,
    )

    args = parser.parse_args()

    if not args.out:
        filename = args.name + '.tar.gz'
    else:
        filename = args.out
    ids = compile_custom_entity(args.endpoint, args.name, filename)

    if not ids:
        logger.warning('No new skills avalibale')
        sys.exit(1)
    else:
        if args.notify:
            time.sleep(args.notification_timeout)
            notify_skills(args.endpoint, ids)

        print os.path.abspath(filename)


if __name__ == '__main__':
    import logging
    logging.basicConfig(level=logging.DEBUG)
    main()
