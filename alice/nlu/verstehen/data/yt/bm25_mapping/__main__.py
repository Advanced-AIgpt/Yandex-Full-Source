import argparse

import yt.wrapper as yt

from verstehen.preprocess import text_to_sequence, lemmatize_token

yt.config.set_proxy('hahn.yt.yandex.net')


def mapper(row):
    text = row['text'].decode('utf-8')
    sequence = text_to_sequence(text)
    sequence = [lemmatize_token(token) for token in sequence]

    for token in sequence:
        yield {
            'token': token,
            'verstehen_id': row['verstehen_id']
        }


def reducer(key, rows):
    yield {'token': key['token'], 'verstehen_id': key['verstehen_id'], 'occurrences': sum([1 for _ in rows])}


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--input_table', type=str, required=True,
        help='Input table to process. Must have `text` text column and `verstehen_id` token.'
    )
    parser.add_argument(
        '--output_table', type=str, required=True,
        help='Output table to store results in.'
    )
    args = parser.parse_args()

    yt.run_map(
        mapper, args.input_table, args.output_table,
        job_io={'control_attributes': {'enable_row_index': True}}
    )

    yt.run_sort(args.output_table, sort_by=['token', 'verstehen_id'])

    yt.run_reduce(
        reducer, args.output_table, args.output_table, reduce_by=['token', 'verstehen_id']
    )
