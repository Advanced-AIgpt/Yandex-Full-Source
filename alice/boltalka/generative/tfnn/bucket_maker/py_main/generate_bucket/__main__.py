import argparse
import json

from alice.boltalka.generative.tfnn.bucket_maker.lib import generate_bucket


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model-class', default='tfnn.task.seq2seq.models.transformer.Model')
    parser.add_argument('--model-path', required=True)
    parser.add_argument('--token-to-id-voc-path', required=True)
    parser.add_argument('--bpe-voc-path', required=True)
    parser.add_argument('--hp', type=lambda x: json.loads(x), required=False, default=None)
    parser.add_argument('--hp-path', required=False, default=None)
    parser.add_argument('--model-prefix-name', default='mod')
    parser.add_argument('--device', default='')

    parser.add_argument('--sampling-temperature', default=0.6, type=float)
    parser.add_argument('--sampling-topk', default=50, type=int)
    parser.add_argument('--sampling-hypothesis-per-input', default=1, type=int)

    # TODO change the table path to the new table!
    parser.add_argument('--input-table',
                        default='//home/voice/krom/bucket_14.02.2019/bucket_searchappprod.5000.preprocessed')
    parser.add_argument('--yt-proxy', default='hahn')

    parser.add_argument('--separator-token', default='[SPECIAL_SEPARATOR_TOKEN]')
    parser.add_argument('--output-table', required=True)
    parser.add_argument('--batch-size', type=int, default=50)
    parser.add_argument('--process_n_first_rows', type=int, default=None)
    parser.add_argument('--contexts-columns', type=lambda x: x.split(',') if x else None, default=None)

    args = parser.parse_args()

    if (args.hp is None) == (args.hp_path is None):
        raise ValueError('Either --hp or --hp-path must be specified')

    if args.hp_path is not None:
        with open(args.hp_path) as f:
            args.hp = json.load(f)
    del args.hp_path

    generate_bucket(**vars(args))


if __name__ == '__main__':
    main()
