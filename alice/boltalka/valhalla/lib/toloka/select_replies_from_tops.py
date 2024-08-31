import codecs, json, argparse, sys
from collections import Counter
import hashlib
import yt.wrapper as yt
import vh


HASH_DENOM = 1000000000


def hash_context(context, salt):
    return int(hashlib.md5((context + salt).encode('utf-8')).hexdigest(), 16)


@vh.lazy.arcadia_executable_path('alice/boltalka/valhalla/pack')
@vh.lazy.nirvana_operation('ee84b77b-7bd0-4335-b4ef-f96cc838299f')
@vh.lazy.from_annotations
def select_replies_from_tops(
        src: vh.mkinput(vh.TSVFile),
        context_len: vh.mkoption(int),
        top_size: vh.mkoption(int),
        sample_top_size: vh.mkoption(int),
        unique_top: vh.mkoption(bool),
        salt: vh.mkoption(str),
        keep_scores: vh.mkoption(bool),
        sample_policy: vh.mkoption(bool),
        proba_scores: vh.mkoption(bool),
        dst_tsv: vh.mkoutput(vh.TSVFile),
        dst_json: vh.mkoutput(vh.JSONFile),
        dst_yt: vh.mkoutput(vh.YTTable),
    ):
    assert not (sample_policy and sample_top_size is not None), "sample_policy is not compatible with sizes sample_top_size"
    assert top_size is None or sample_top_size is None, 'Use either sample_top_size or top_size, not both'
    contexts = []
    tops = []
    with codecs.open(src, 'r', 'utf-8') as f:
        for line in f:
            turns = line.rstrip('\n').split('\t')
            source = turns[-1]
            reply = turns[-2]
            context = '\t'.join(turns[-context_len - 2:-2])
            if not contexts or context != contexts[-1]:
                contexts.append(context)
                tops.append([])
            tops[-1].append({'reply': reply, 'source': source})
            if keep_scores or sample_policy or proba_scores:
                tops[-1][-1]['score'] = int(turns[0])

    if unique_top:
        for k in range(len(tops)):
            used_replies = set()
            insert_idx = 0
            for dct in tops[k]:
                if dct['reply'] not in used_replies:
                    tops[k][insert_idx] = dct
                    insert_idx += 1
                    used_replies.add(dct['reply'])
            del tops[k][insert_idx:]

    if sample_top_size is not None:
        for k in range(len(tops)):
            del tops[k][sample_top_size:]
            idx = hash_context(contexts[k], salt) % len(tops[k])
            tops[k] = [tops[k][idx]]

    if top_size is not None:
        for k in range(len(tops)):
            del tops[k][top_size:]

    if proba_scores or sample_policy:
        for k in range(len(tops)):
            denom = sum([el['score'] for el in tops[k]])
            for el in tops[k]:
                el['proba_score'] = el['score'] / float(denom) if denom > 0 else 0.0

    if sample_policy:
        for k in range(len(tops)):
            pos = (hash_context(contexts[k], salt) % HASH_DENOM) / float(HASH_DENOM)
            cumsum = 0
            for el in tops[k]:
                cumsum += el['proba_score']
                if cumsum >= pos:
                    tops[k] = [el]
                    break
            else:
                tops[k] = tops[k][-1:]  # can only happen due to bad rounding

    if dst_tsv:
        with codecs.open(dst_tsv, 'w', 'utf-8') as f:
            for context, top in zip(contexts, tops):
                for dct in top:
                    line = context + '\t' + dct['reply'] + '\t' + dct['source']
                    if keep_scores:
                        line = str(dct['score']) + '\t' + line
                    elif proba_scores:
                        line = str(dct['proba_score']) + '\t' + line
                    f.write(line + '\n')
    if dst_json or dst_yt:
        dialogues = []
        context_columns = ['context_%d' % i for i in reversed(range(context_len))]
        for context, top in zip(contexts, tops):
            for dct in top:
                dialog = {'source': 'model'}
                dialog.update(zip(context_columns, context.split('\t')))
                dialog.update(dct)
                dialogues.append(dialog)
    if dst_json:
        with codecs.open(dst_json, 'w', 'utf-8') as f:
            json.dump(dialogues, f, ensure_ascii=False, indent=4)
    if dst_yt:
        yt.write_table(dst_yt, iter(dialogues))
