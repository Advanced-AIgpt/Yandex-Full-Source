import argparse

import yt.wrapper as yt

from verstehen.preprocess import replace_characters_with_pad_and_trim

yt.config.set_proxy('hahn.yt.yandex.net')


class Mapper(object):
    def __init__(self, max_len):
        self.max_len = max_len

    def __call__(self, row):
        text = row['text']

        if text is None:
            return

        text = text.decode('utf-8')
        text = replace_characters_with_pad_and_trim(text)
        text = text[:self.max_len]

        if len(text) > 0:
            row['text'] = text
            yield row


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
    parser.add_argument(
        '--max_len', type=int, default=4096,
        help='The maximum length of the text to which the text is truncated to.'
    )
    args = parser.parse_args()

    yt.run_map(Mapper(max_len=args.max_len), [args.input_table], [args.output_table])
