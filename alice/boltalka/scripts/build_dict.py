#!/usr/bin/python
# coding=utf-8
import sys
import codecs
import argparse

import regex

sys.stdin = codecs.getreader('utf-8')(sys.stdin)
sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

def enumerate_tokens(args):
    ignore_columns = set(args.ignore_columns)
    line_number = 0
    column_count = None
    for line in codecs.open(args.text_file, 'r', 'utf-8'):
        line_number += 1
        if line_number % 100000 == 0:
            print >> sys.stderr, line_number, 'lines processed\r',
        if line_number > args.max_lines:
            break

        parts = line.rstrip().split('\t')
        if False and column_count is not None and len(parts) != column_count:
            print '%d columns found, but %d were expected in the following line:' % (len(parts), column_count)
            print line
            raise ValueError('Invalid column count')
        column_count = len(parts)

        for part_idx in xrange(len(parts)):
            if part_idx in ignore_columns:
                continue

            part = parts[part_idx]
            if args.lower:
                part = part.lower()
            if args.separate_punctuation:
                part = regex.sub(ur'([^а-яa-zѣéóà])', ur' \1 ', part)
                #part = regex.sub(ur'(\p{P}|`|~)', ur' \1 ', part)
            tokens = part.split()
            if args.skip_speaker_id:
                tokens = tokens[1:]
            for token in tokens:
                if args.char_ngrams is None:
                    yield token
                else:
                    token = args.char_ngrams_sentinel + token + args.char_ngrams_sentinel
                    for i in xrange(len(token) - args.char_ngrams + 1):
                        yield token[i:i+args.char_ngrams]


def main():
    parser = argparse.ArgumentParser(add_help = True)
    parser.add_argument('--text-file', metavar='FILE', dest='text_file', required=True,
                        help='A file with the text dataset')
    parser.add_argument('--dict-file', metavar='FILE', dest='dict_file', required=True,
                        help='A file to save the built dictionary to')
    parser.add_argument('--dict-size', metavar='NUM', dest='dict_size', type=int, required=True,
                        help='Dictionary size')
    parser.add_argument('--max-coverage', metavar='NUM', dest='max_coverage', type=float, required=False, default=1.0,
                        help='If specified, dictionary will be trimmed to meet coverage requirements')
    parser.add_argument('--min-freq', metavar='NUM', dest='min_freq', type=int, default=1,
                        help='Minimum frequency of a word in the dataset to be considered for the dictionary')
    parser.add_argument('--max-lines', metavar='NUM', dest='max_lines', type=int, default=sys.maxint,
                        help='Maximum number of dataset lines to look through (no restriction by default)')
    parser.add_argument('--char-ngrams', metavar='NUM', dest='char_ngrams', type=int, default=None,
                        help='If specified, a dictionary of character n-grams instead of words will be built')
    parser.add_argument('--char-ngrams-sentinel', metavar='NUM', dest='char_ngrams_sentinel', type=str, default='',
                        help='Sentinel symbol used for character n-grams (empty by default)')
    parser.add_argument('--ignore-columns', dest='ignore_columns', type=int, nargs='*', default=[],
                        help='If specified, the columns in this list will be ignored. Column indices are zero-based')
    parser.add_argument('--lower', dest='lower', action='store_true',
                        help='If specified, lowercase tokens before building the dict')
    parser.add_argument('--separate-punctuation', dest='separate_punctuation', action='store_true',
                        help='If specified, punctuation will be treated as separate tokens')
    parser.add_argument('--skip-speaker-id', dest='skip_speaker_id', action='store_true',
                        help='If specified, first token in each reply will be skipped')
    args = parser.parse_args()

    # Compute token frequencies
    tokens_total = 0
    token_to_frq = {}
    for token in enumerate_tokens(args):
        tokens_total += 1
        token_to_frq[token] = token_to_frq.get(token, 0) + 1

    # Build dict
    token_with_frq = list((t, f) for t, f in token_to_frq.iteritems() if f >= args.min_freq)
    token_with_frq.sort(key=lambda x:x[1], reverse=True)

    final_dict_size = 0
    tokens_covered = 0
    print tokens_total
    for token, frq in token_with_frq:
        print str(frq) + '\t' + token

    with codecs.open(args.dict_file, 'w', 'utf-8') as dict_file:
        for token, frq in token_with_frq[:args.dict_size]:
            dict_file.write('%s\n' % token)
            final_dict_size += 1
            tokens_covered += frq
            if float(tokens_covered) / tokens_total >= args.max_coverage:
                break

    print 'Final dictionary size:', final_dict_size
    print 'Dataset token coverage: %.2f' % (float(tokens_covered) / tokens_total)

if __name__ == '__main__':
    main()
