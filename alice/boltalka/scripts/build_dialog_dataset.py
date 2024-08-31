#!/usr/bin/python
# coding=utf-8
import sys
import codecs
import argparse

import regex

#PUNCT_REGEX = ur'(\p{P}|`|~)'
PUNCT_REGEX = ur'([^а-яa-zѣéóà])' #ur'([^а-яА-Яa-zA-Zѣ])'

def token_or_unk(token, dct):
    return token if dct is None or token in dct else '_UNK_'

def parse_dialog_line(args, line, dct):
    res_tokens = []

    tokens = line.split()
    if args.has_speakers:
        speaker = tokens[0]
        res_tokens.append(speaker)
        tokens = tokens[1:]

    for token in tokens:
        #TODO(alipov): get rid of this if
        if False and token[0] == '_' and token[-1] == '_' or not args.separate_punctuation: # special tokens _TOKEN_
            if token == '_BOS_' or token == '_EOS_': # for twitter dataset from tddy@
                continue
            res_tokens.append(token_or_unk(token, dct))
        else:
            if args.lower:
                token = token.lower()
            for subtoken in regex.sub(PUNCT_REGEX, ur' \1 ', token).split():
                res_tokens.append(token_or_unk(subtoken, dct))
    res_tokens.append('_EOS_')
    return ' '.join(res_tokens)

def file_line_iterator(file_name):
    line_number = 0
    for line in codecs.open(file_name, 'r', 'utf-8'):
        line_number += 1
        if line_number % 100000 == 0:
            print >> sys.stderr, line_number, 'lines processed\r',
        yield line.rstrip()

def parse_dialog_line_iterator(args, dialog_lines, dct):
    for line in dialog_lines:
        yield parse_dialog_line(args, line, dct)

def parse_dialogs(args):
    dct = None
    if args.dict_file:
        dct = set()
        for line in codecs.open(args.dict_file, 'r', 'utf-8'):
            dct.add(line.rstrip())
        assert '_UNK_' in dct and '_EOS_' in dct

    if args.dialog_per_line:
        for line in file_line_iterator(args.text_file):
            yield parse_dialog_line_iterator(args, line.split('\t'), dct)
    else:
        yield parse_dialog_line_iterator(args, file_line_iterator(args.text_file), dct)

def limit_num_tokens(context, max_num_tokens):
    num_tokens = 0
    for i in xrange(len(context) - 1, -1, -1):
        c = context[i]
        if c == ' ':
            num_tokens += 1
            if num_tokens == max_num_tokens:
                return context[i + 1:]
    return context

def generate_samples(args):
    with codecs.open(args.output_file, 'w', 'utf-8') as out:
        for dialog in parse_dialogs(args):
            context = []
            context_speakers = []
            if args.allow_empty_context:
                assert not args.speaker_for_every_token
                context.append('_EOS_')

            for line in dialog:
                target_speaker = ''
                speaker_for_reply_tokens = ''
                if args.strip_speakers or args.target_speaker or args.speaker_for_every_token:
                    speaker, line = line.split(' ', 1)
                    if args.target_speaker:
                        target_speaker = ' ' + speaker
                    if args.speaker_for_every_token:
                        speaker_for_reply_tokens = ' '.join([speaker] * (line.count(' ') + 1))

                if context and (not args.only_cartman or speaker == '_Cartman_'):
                    min_context_len = 1 if args.all_contexts else len(context)
                    for context_len in xrange(min_context_len, len(context) + 1):
                        all_context = limit_num_tokens(' '.join(context[-context_len:]) + target_speaker, args.max_context_tokens)
                        speakers_for_tokens = ''
                        if args.speaker_for_every_token:
                            speakers_for_tokens = '\t'
                            speakers_for_tokens += limit_num_tokens(' '.join(context_speakers[-context_len:]), args.max_context_tokens)
                            speakers_for_tokens += '\t'
                            speakers_for_tokens += speaker_for_reply_tokens
                        out.write(all_context + '\t' + line + speakers_for_tokens + '\n')

                if not args.strip_speakers and args.target_speaker:
                    target_speaker = speaker + ' '

                context.append(target_speaker + line)
                context_speakers.append(speaker_for_reply_tokens)

                while len(context) > args.max_context_turns:
                    context.pop(0)
                    context_speakers.pop(0)

def main(args):
    generate_samples(args)

if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--text-file', metavar='FILE', required=True,
                        help='A file with the text dataset')
    parser.add_argument('--output-file', metavar='FILE', required=True,
                        help='A file with the resulting dataset')
    parser.add_argument('--dict-file', metavar='FILE', default='',
                        help='A file with dictionary. Leave all tokens, if this argument is omitted.')
    parser.add_argument('--max-context-tokens', metavar='NUM', type=int, default=sys.maxint,
                        help='Maximal number of tokens in context for predicting next reply')
    parser.add_argument('--max-context-turns', metavar='NUM', type=int, default=1,
                        help='Maximal number of turns in context for predicting next reply')
    parser.add_argument('--all-contexts', action='store_true',
                        help='If specified, produce all possible contexts for each reply')
    parser.add_argument('--dialog-per-line', action='store_true',
                        help='If specified, treat each line as a tabseparated lines of dialog')
    parser.add_argument('--lower', action='store_true',
                        help='If specified, lowercase tokens')
    parser.add_argument('--separate-punctuation', action='store_true',
                        help='If specified, punctuation will be treated as separate tokens')
    parser.add_argument('--strip-speakers', action='store_true',
                        help='If specified, strips first token from every reponse')
    parser.add_argument('--target-speaker', action='store_true',
                        help='If specified, appends speaker from reply to context')
    parser.add_argument('--speaker-for-every-token', action='store_true',
                        help='If specified, appends two columns to each line - speakers for every token in context and reply correspondingly')
    parser.add_argument('--allow-empty-context', action='store_true',
                        help='If specified, adds first lines of dialogs as target to dataset')
    parser.add_argument('--only-cartman', action='store_true')
    args = parser.parse_args()

    args.has_speakers = args.target_speaker or args.strip_speakers or args.speaker_for_every_token
    if args.has_speakers:
        assert ((args.target_speaker or args.strip_speakers) ^ args.speaker_for_every_token)


    main(args)
