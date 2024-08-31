import argparse
import string

import yt.wrapper as yt

yt.config.set_proxy('hahn.yt.yandex.net')


def mapper(row):
    row['processed_text'] = string.lower(row['text'].decode('utf-8'))
    yield row


class DuplicatesReducer:
    def __init__(self, row_count):
        self.row_count = row_count

    def __call__(self, key, rows):
        to_leave = None  # a row to leave after deduplication
        reducing_row_count = 0  # total number of `rows`
        for row in rows:
            if to_leave is None:
                to_leave = row

            # if dates are available in data, then we use the latest request to leave
            date = row.get('request_time_ms', None)
            if date is not None and to_leave['request_time_ms'] < date:
                to_leave = row

            reducing_row_count += 1

        to_leave.pop('processed_text')
        to_leave['occurrence_rate'] = float(reducing_row_count) / self.row_count
        yield to_leave


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--input_table', type=str, required=True,
        help='Input table to process. Must have `text` text column.'
    )
    parser.add_argument(
        '--output_table', type=str, required=True,
        help='Output table to store results in.'
    )
    args = parser.parse_args()

    yt.run_map_reduce(
        mapper, DuplicatesReducer(yt.row_count(args.input_table)),
        [args.input_table], [args.output_table],
        reduce_by=['processed_text']
    )
