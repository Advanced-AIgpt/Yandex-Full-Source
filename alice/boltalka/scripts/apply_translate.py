#!/usr/bin/python
import sys
import codecs
import argparse
import regex

sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

def separate_punctuation(s):
    return regex.sub(ur'(\p{P}|`|~)', ur' \1 ', s)

def main(args):
    dct = {}
    for line in codecs.open(args.dict_file, 'r', 'utf-8'):
        frm, to = line.rstrip().split('\t')
        frm = separate_punctuation(frm.lower())
        to = separate_punctuation(to.lower())
        assert not frm in dct
        dct[frm] = to

    for line_number, line in enumerate(sys.stdin):
        line_number += 1
        if line_number % 100000 == 0:
            print >> sys.stderr, line_number, 'lines processed\r',

        dialog = []
        for dialog_line in line.rstrip().split('\t'):
            if args.separate_punctuation:
                dialog_line = separate_punctuation(dialog_line)
            if args.lower:
                dialog_line = dialog_line.lower()
            tokens = dialog_line.rstrip()
            start_window = 2 if args.bigrams else 1
            for window in xrange(start_window, 0, -1):
                tokens = tokens.split()
                cur_tokens = []
                i = 0
                while i < len(tokens):
                    token = ' '.join(tokens[i:i + window])
                    if token in dct:
                        token = dct[token]
                        cur_tokens.append(token)
                        i += window
                    else:
                        cur_tokens.append(tokens[i])
                        i += 1
                tokens = ' '.join(cur_tokens)
            dialog.append(tokens)
        print '\t'.join(dialog)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help = True)
    parser.add_argument('--dict-file', metavar='FILE', required=True,
                        help='A file with dict')
    parser.add_argument('--lower', action='store_true',
                        help='If specified, lowercase tokens')
    parser.add_argument('--separate-punctuation', action='store_true',
                        help='If specified, punctuation will be treated as separate tokens')
    parser.add_argument('--bigrams', action='store_true',
                        help='If specified, translate bigrams too')
    args = parser.parse_args()

    main(args)
