# coding=utf-8
import sys
import codecs
import regex

def load_bpe(filename):
    merges = {}
    bps = []
    alphabet = []
    for i, line in enumerate(codecs.open(filename, 'r', 'utf-8')):
        parts = line.rstrip().split('\t')
        if len(parts) == 2: # alphabet
            assert i == len(alphabet)
            ch, _ = parts
            alphabet.append(ch)
            continue
        assert len(parts) == 3
        l, r, _ = parts
        assert not (l, r) in merges
        merges[(l, r)] = i - len(alphabet)
        bps.append(l + r)
    return merges, bps, alphabet

def apply_bpe(merges, s, eow='-'):
    s = regex.sub(ur'(\s|$)', eow + ur'\1', s)
    s = regex.findall('.', s)
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
                s[j] = s[i] + s[i + 1]
                i += 2
            else:
                s[j] = s[i]
                i += 1
            j += 1
        del s[j:]
    return ' '.join([c for c in s if c != ' '])


if __name__ == '__main__':
    import time
    sys.stdin = codecs.getreader('utf-8')(sys.stdin)
    sys.stdout = codecs.getwriter('utf-8')(sys.stdout)

    merges, bps, alphabet = load_bpe('../tools/train_bpe/lib.test4')
    start_time = time.time()
    for i, line in enumerate(sys.stdin):
        parts = []
        for part in line.rstrip().split('\t'):
            tokens = []
            for token in part.split():
                tokens.append(apply_bpe(merges, token))
            parts.append(' '.join(tokens))
        print '\t'.join(parts)
        #print apply_bpe(merges, line.rstrip())
        i += 1
        if i % 1000 == 0:
            dt = time.time() - start_time
            print >> sys.stderr, i, 'documents done with', i / dt, 'docs per sec\r',

