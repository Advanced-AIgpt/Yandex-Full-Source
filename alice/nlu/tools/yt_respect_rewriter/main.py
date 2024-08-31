# -*- coding: utf-8 -*-

import argparse
import os
import yt.wrapper as yt

from alice.nlu.py_libs.respect_rewriter import RespectRewriter


_MODEL_PATH = 'data/model.tar'
_CLASSIFIER_PATH = 'data/classifier_model.tar'
_EMBEDDINGS_PATH = 'data/embeddings.tar'


class Mapper(object):
    def __init__(self, source_key, target_key, to_plural, to_gender=RespectRewriter.MASC_GENDER):
        self._rewriter = None
        self._source_key = source_key
        self._target_key = target_key
        self._to_plural = to_plural
        self._to_gender = to_gender

    def start(self):
        self._rewriter = RespectRewriter(
            parser_path=os.path.basename(_MODEL_PATH),
            to_plural=self._to_plural,
            to_gender=self._to_gender,
            classifier_path=os.path.basename(_CLASSIFIER_PATH) if self._to_plural else None,
            embeddings_path=os.path.basename(_EMBEDDINGS_PATH) if self._to_plural else None,
        )

    def __call__(self, input_row):
        text = input_row[self._source_key]
        input_row[self._target_key] = self._rewriter(text)

        yield input_row


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input-table-path', required=True)
    parser.add_argument('--output-table-path', required=True)
    parser.add_argument('--source-key', default='rewritten_reply')
    parser.add_argument('--target-key', default='rewritten_reply')
    parser.add_argument('--reduce-respect', default=False, action='store_true')
    parser.add_argument('--target-gender', default='masc', choices=['masc', 'femn'])
    parser.add_argument('--job-count', default=1000, type=int)
    parser.add_argument('--cpu-limit', default=8, type=int)
    parser.add_argument('--memory-limit', default=16 * 1024, type=int)

    args = parser.parse_args()

    spec = {
        'mapper': {
            'cpu_limit': args.cpu_limit,
        }
    }

    local_files = [_MODEL_PATH]
    if not args.reduce_respect:
        local_files.extend([_CLASSIFIER_PATH, _EMBEDDINGS_PATH])

    yt.run_map(
        Mapper(
            source_key=args.source_key,
            target_key=args.target_key,
            to_plural=not args.reduce_respect,
            to_gender=args.target_gender
        ),
        args.input_table_path,
        args.output_table_path,
        local_files=local_files,
        memory_limit=args.memory_limit * (1024 ** 2),
        spec=spec,
        job_count=args.job_count
    )


if __name__ == '__main__':
    main()
