import argparse
import yt.wrapper as yt
yt.config.set_proxy("hahn.yt.yandex.net")

from alice.boltalka.tools.dssm_preprocessing.preprocessing.lib.main import (
    TwitterCleaner,
    LogsCleaner,
    TextNormalizer,
    PunctEncoder,
    EmoticonsRemover,
)


class Mapper(object):
    def __init__(self, preprocessor, text_keys):
        self.preprocessor = preprocessor
        self.text_keys = text_keys

    def __call__(self, row):
        for k in self.text_keys:
            if k in row:
                preprocessed_text = self.preprocessor(row[k])
                if preprocessed_text is None and row[k] is not None:
                    return
                row[k] = preprocessed_text
        yield row


def run_mapper(args):
    preprocessor = args.preprocessor(args)
    yt.run_map(Mapper(preprocessor, args.text_keys.split(',')), args.src, args.dst, format=yt.YsonFormat(encoding=None))


def main():
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--text-keys', required=True, help='comma separated values')
    subparsers = parser.add_subparsers()

    for parser_name, preprocessor in [('clean-twitter', TwitterCleaner),
                                      ('clean-logs', LogsCleaner),
                                      ('normalize', TextNormalizer),
                                      ('encode-punct', PunctEncoder),
                                      ('strip-emoji', EmoticonsRemover)]:
        subparser = subparsers.add_parser(parser_name)
        preprocessor.add_args(subparser)
        subparser.set_defaults(preprocessor=preprocessor)

    args = parser.parse_args()
    run_mapper(args)
