# coding=utf-8
import sys
import codecs

_BPE_EOS = '_EOS_'
_BPE_UNK = '_UNK_'

# full_dict is not needed in order to encode a string into ids.
# But it is necessary in order to produce bpe-segmented text.
def load_bpe(alphabet_filename, merges_filename, full_dict=False, skip_unk=False):
    dct = {}
    tokens = []
    token_id = 0
    with codecs.open(alphabet_filename, 'r', 'utf-8') as inp:
        for line in inp:
            token = line.rstrip()
            assert not token in dct
            dct[token] = token_id
            tokens.append(token)
            token_id += 1

    dct[_BPE_EOS] = token_id
    tokens.append(_BPE_EOS)
    token_id += 1

    if not skip_unk:
        dct[_BPE_UNK] = token_id
        tokens.append(_BPE_UNK)
        token_id += 1

    merges = {}
    with codecs.open(merges_filename, 'r', 'utf-8') as inp:
        for line in inp:
            l, r, _ = map(int, line.rstrip().split('\t'))
            assert not (l, r) in merges
            merges[(l, r)] = token_id
            if full_dict:
                token = tokens[l] + ' ' + tokens[r]
                dct[token] = token_id
                tokens.append(token)
            token_id += 1

    return dct, merges, tokens

def apply_bpe(s, dct, merges):
    s = s.split(' ') + [_BPE_EOS]

    skip_unk = not _BPE_UNK in dct
    if skip_unk:
        s = [dct[token] for token in s if token in dct]
    else:
        unk = dct[_BPE_UNK]
        s = [dct.get(token, unk) for token in s]

    while len(s) > 1:
        min_id = len(merges)
        for i in xrange(len(s) - 1):
            pair = (s[i], s[i + 1])
            if not pair in merges:
                continue
            if merges[pair] < min_id:
                min_id = merges[pair]
                best_pair = pair
        if min_id == len(merges):
            break
        i, j = 0, 0
        while i < len(s):
            if i + 1 < len(s) and (s[i], s[i + 1]) == best_pair:
                s[j] = min_id
                i += 2
            else:
                s[j] = s[i]
                i += 1
            j += 1
        del s[j:]
    return s

if __name__ == '__main__':
    import time
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    alphabet_filename = sys.argv[1]
    merges_filename = sys.argv[2]

    dct, merges, tokens = load_bpe(alphabet_filename, merges_filename, full_dict=True)
    start_time = time.time()
    # input should comprise tab-separated columns of space-separated tokens
    # each column will be segmented independently
    for i, line in enumerate(sys.stdin):
        parts = []
        for part in line.rstrip().split('\t'):
            ids = apply_bpe(part, dct, merges)
            # `tokens[j]` is the only place where full_dict is required
            parts.append(' '.join('<' + tokens[j] + '>' for j in ids))
        print '\t'.join(parts)
        i += 1
        if i % 1000 == 0:
            dt = time.time() - start_time
            print >> sys.stderr, i, 'documents done with', i / dt, 'docs per sec\r',

