import argparse
import os

import vh
from alice.boltalka.generative.training.data.nn.util.lib import tokenize_columns
from alice.boltalka.generative.training.data.nn.util.ops import get_yt_file, mr_copy, to_tsv, mr_upload, get_yt_table, \
    mr_mkdir, train_test_split


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument(
        '--input-table', required=True
    )
    parser.add_argument(
        '--tokenize-columns', help='Columns to tokenize separated by a comma', default='context_0,reply'
    )
    parser.add_argument(
        '--use-preprocessing', help='Whether to use preprocessing before tokenization', action='store_true'
    )
    parser.add_argument(
        '--input-column', required=True, default='context_0'
    )
    parser.add_argument(
        '--output-column', required=True, default='reply'
    )
    parser.add_argument(
        '--output-folder', required=True
    )
    parser.add_argument(
        '--test-size-ratio', default=0.0, type=float, help='if not 0, table will be split in train/test'
    )
    parser.add_argument(
        '--bpe-voc-path', default='//home/voice/artemkorenev/boltalka/bart_lm/bpe.voc'
    )
    parser.add_argument(
        '--quota', default='dialogs'
    )
    parser.add_argument(
        '--yt_token_secret', default='artemkorenev_yt_token'
    )
    args = parser.parse_args()

    with vh.Graph() as g:
        bpe_voc = get_yt_file(args.bpe_voc_path)
        x = get_yt_table(args.input_table)

        if len(args.tokenize_columns) != 0:
            x = tokenize_columns(
                x, bpe_voc, columns_to_tokenize=args.tokenize_columns.split(','),
                skip_preprocessing=not args.use_preprocessing
            )

        if args.test_size_ratio != 0.0:
            train, val = train_test_split(x, test_size=args.test_size_ratio)
            tables = [train, val]
            table_names = ['train', 'val']
        else:
            tables = [x]
            table_names = ['train']

        target_folder = os.path.join(args.output_folder)
        mr_mkdir(target_folder)

        for table, name in zip(tables, table_names):
            mr_copy(table, os.path.join(target_folder, name))

            if name == 'val':
                mr_upload(
                    to_tsv(table, columns=[args.input_column]), dst=os.path.join(target_folder, 'model_input.txt')
                )
                mr_upload(
                    to_tsv(table, columns=[args.output_column]), dst=os.path.join(target_folder, 'model_output.txt')
                )

    info = vh.run(g, quota=args.quota, yt_token_secret=args.yt_token_secret, yt_proxy='hahn').get_workflow_info()
    print('https://nirvana.yandex-team.ru/flow/{}'.format(info.workflow_id))
