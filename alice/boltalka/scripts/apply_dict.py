#!/usr/bin/python
import sys
import codecs
import argparse
import regex

sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

def main(args):
    dct = set()
    for line in codecs.open(args.dict_file, 'r', 'utf-8'):
        dct.add(line.rstrip())
    assert '_UNK_' in dct and '_EOS_' in dct

    for line in sys.stdin:
        if args.separate_punctuation:
            line = regex.sub(ur'(\p{P}|`|~)', ur' \1 ', line)
        tokens = []
        for token in line.split():
            if args.lower:
                token = token.lower()
            if not token in dct:
                token = '_UNK_'
            tokens.append(token)
        tokens.append('_EOS_')
        print ' '.join(tokens)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help = True)
    parser.add_argument('--dict-file', metavar='FILE', required=True,
                        help='A file with dict')
    parser.add_argument('--lower', dest='lower', action='store_true',
                        help='If specified, lowercase tokens')
    parser.add_argument('--separate-punctuation', dest='separate_punctuation', action='store_true',
                        help='If specified, punctuation will be treated as separate tokens')
    args = parser.parse_args()

    main(args)
