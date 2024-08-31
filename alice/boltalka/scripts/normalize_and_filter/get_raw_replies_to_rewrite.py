import argparse
import yt.wrapper as yt
import codecs
import re
import string
import os

yt.config.set_proxy("hahn.yt.yandex.net")


def normalize_text(text):
    s = re.sub(u"([" + string.punctuation + ur"\\])", ur" \1 ", text).strip()
    s = re.sub(ur"\s+", " ", s, flags=re.U).strip()
    return s.lower()


class Mapper(object):
    def __init__(self, utterances):
        self.utterances = utterances
    def __call__(self, row):
        rewritten_reply = normalize_text(row['rewritten_reply'].decode('utf-8'))
        for u in self.utterances:
            if u in rewritten_reply:
                yield row


def read_utterances(filename):
    utterances = []
    with codecs.open(filename, 'r', 'utf-8') as f:
        for line in f:
            line = normalize_text(line.rstrip('\n'))
            utterances.append(line)
    return utterances


def main(args):
    utterances = read_utterances(args.utterances)

    replies = []
    with yt.TempTable(os.path.dirname(args.index)) as tmp:
        yt.run_map(Mapper(utterances), args.index, tmp)
        for row in yt.read_table(tmp):
            replies.append(row['reply'])

    for reply in sorted(replies):
        print reply


if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--index', required=True)
    parser.add_argument('--utterances', required=True)
    args = parser.parse_args()
    main(args)
