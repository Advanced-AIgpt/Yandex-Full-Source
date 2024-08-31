# coding=utf-8
import sys
import codecs
import regex
from background_generator import *
from bpe_utils import *

PUNCT_REGEX = ur'([^а-яa-zѣéóà])' #ur'([^а-яА-Яa-zA-Zѣ])'

_PAD = '_PAD_'
_GO = '_GO_'
_EOS = '_EOS_'
_UNK = '_UNK_'
_SPECIAL_TOKENS = [_PAD, _GO, _EOS, _UNK]

def load_dictionary(filename):
    dct = {}
    tokens = []
    for i, line in enumerate(codecs.open(filename, 'r', 'utf-8')):
        token = line.rstrip()
        dct[token] = i
        tokens.append(token)
    for token in _SPECIAL_TOKENS:
        if token not in dct:
            new_id = len(dct)
            dct[token] = new_id
            tokens.append(token)
            print >> sys.stderr, 'INFO: {} token is not in dictionary, adding it with id = {}'.format(token, new_id)
    return dct, tokens

def load_bpe_dictionary(filename):
    merges, bps, alphabet = load_bpe(filename)
    if len(alphabet) == 0:
        print >> sys.stderr, 'WARNING: No alphabet was found in BPE dict. Trying to infer it.'
        alphabet = set()
        for pair in bps:
            alphabet.update(regex.findall('.', pair))
        alphabet = sorted(alphabet)

    special_tokens = list(_SPECIAL_TOKENS)
    special_tokens.remove(_UNK)
    tokens = alphabet + bps + special_tokens
    dct = { token: i for i, token in enumerate(tokens) }
    return dct, tokens, merges

def dataset_line_to_ids(line, dct):
    ids = []
    unk = dct.get(_UNK, None)
    for part in line.split('\t'):
        token_ids = []
        for token in part.split(' '):
            if not token:
                continue
            token_id = dct.get(token, unk)
            if not token_id is None:
                token_ids.append(token_id)
        ids.append(token_ids)
    return ids

def text_to_ids(text, dct):
    text = text.lower()
    text = regex.sub(PUNCT_REGEX, ur' \1 ', text)
    ids = dataset_line_to_ids(text, dct)[0]
    return ids + [dct[_EOS]]

def ids_to_text(ids, tokens):
    result = []
    for i in ids:
        result.append(tokens[i])
        if tokens[i] == _EOS:
            break
    return ' '.join(result)

def get_context_for_decoder(context, dct):
    context_ids = sum((text_to_ids(text, dct) for text in context), [])
    dummy_reply_ids = []
    return [[context_ids, dummy_reply_ids]]

def apply_bpe_to_dataset_line(line, merges):
    parts = []
    for part in line.split('\t'):
        tokens = []
        for token in part.split(' '):
            if not token:
                continue
            if token in _SPECIAL_TOKENS:
                tokens.append(token)
            else:
                tokens.append(apply_bpe(merges, token))
        parts.append(' '.join(tokens))
    return '\t'.join(parts)

@background
def iterate_batches(filename, dct, batch_size, infinite=False, bpe_merges=None, num_skip_batches=0):
    run = True
    batch_id = 0
    while run:
        run = infinite
        with codecs.open(filename, 'r', 'utf-8') as inp:
            batch = []
            for line in inp:
                if batch_id < num_skip_batches:
                    ids = []
                else:
                    if bpe_merges is not None:
                        line = apply_bpe_to_dataset_line(line.rstrip(), bpe_merges)
                    ids = dataset_line_to_ids(line.rstrip(), dct)

                batch.append(ids)
                if len(batch) == batch_size:
                    if batch_id >= num_skip_batches:
                        yield batch
                    batch_id += 1
                    batch = []
            if batch:
                if batch_id >= num_skip_batches:
                    yield batch
                batch_id += 1
