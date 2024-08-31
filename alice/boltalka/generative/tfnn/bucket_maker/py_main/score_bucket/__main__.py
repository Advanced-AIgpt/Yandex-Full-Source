import argparse
import json

from alice.boltalka.generative.tfnn.bucket_maker.lib import score_bucket


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--model-class', default='tfnn.task.seq2seq.models.transformer.Model')
    parser.add_argument('--model-path', required=True)
    parser.add_argument('--token-to-id-voc-path', required=True)
    parser.add_argument('--bpe-voc-path', required=True)
    parser.add_argument('--hp', type=lambda x: json.loads(x), default=None)
    parser.add_argument('--hp-path', default=None)
    parser.add_argument('--model-prefix-name', default='mod')
    parser.add_argument('--device', default='')

    parser.add_argument('--sampling-temperature', default=0.6, type=float)

    parser.add_argument('--input-table', required=True)
    parser.add_argument('--yt-proxy', default='hahn')
    parser.add_argument('--tokenize-reply-column', action='store_true')
    parser.add_argument('--reply-column', default='tokenized_reply')
    parser.add_argument('--context-columns-prefix', default='context')
    parser.add_argument('--ignore-context', action='store_true')

    parser.add_argument('--separator-token', default='[SPECIAL_SEPARATOR_TOKEN]')
    parser.add_argument('--output-table', required=True)
    parser.add_argument('--batch-size', type=int, default=50)

    args = parser.parse_args()

    if (args.hp is None) == (args.hp_path is None):
        raise ValueError('Either --hp or --hp-path must be specified')

    if args.hp_path is not None:
        with open(args.hp_path) as f:
            args.hp = json.load(f)
    del args.hp_path

    if args.ignore_context:
        args.context_columns_prefix = None
    del args.ignore_context

    score_bucket(**vars(args))


if __name__ == '__main__':
    main()
