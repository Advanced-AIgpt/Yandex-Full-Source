# coding=utf-8
import yt.wrapper as yt
import argparse, os
import itertools, struct
import numpy as np

yt.config.set_proxy("hahn.yt.yandex.net")


def hex2dec(emb):
    return [struct.unpack('f', emb[idx:idx+4])[0] for idx in xrange(0, len(emb), 4)]


class Joiner(object):
    def __init__(self, num_contexts, are_contexts_unique=True):
        self.num_contexts = num_contexts
        self.are_contexts_unique = are_contexts_unique

    def __call__(self, key, rows):
        rows = reversed(list(rows))
        contexts = []
        contexts_set = set()
        contexts_emb = []
        pool_row = next(rows)
        reply_emb = hex2dec(pool_row['reply_embedding'])
        while 'shard_id' in pool_row:
            context = '\t'.join([pool_row['context_2'], pool_row['context_1'], pool_row['context_0']])
            if not (self.are_contexts_unique and context in contexts_set):
                contexts_set.add(context)
                contexts.append(context)
                contexts_emb.append(hex2dec(pool_row['context_embedding']))
            pool_row = next(rows)

        if len(contexts_emb) == 0:
            return

        contexts_emb_T = np.array(contexts_emb).T

        for row in itertools.chain([pool_row], rows):
            query_emb = hex2dec(row['context_embedding'])
            qr_score = np.dot(query_emb, reply_emb)
            qc_scores = np.dot(query_emb, contexts_emb_T)
            #rc_scores = np.dot(reply_emb, contexts_emb_T)
            for idx in np.argsort(-qc_scores)[:self.num_contexts]:
                row.update({k: v for k, v in zip(['reply_context_2', 'reply_context_1', 'reply_context_0'],
                                                 contexts[idx].split('\t'))})
                # ranking score
                row['score'] = (qc_scores[idx] + qr_score) / 2.
                #row['score_qr'] = qr_score
                #row['score_qc'] = qc_scores[idx]
                #row['score_rc'] = rc_scores[idx]
                del row['context_embedding'], row['reply_embedding']
                yield row


def main(args):
    with yt.TempTable(os.path.dirname(args.src)) as temp_src, \
         yt.TempTable(os.path.dirname(args.src)) as temp_pool:

        yt.run_sort(args.pool, temp_pool, sort_by=['reply'])
        yt.run_sort(args.src, temp_src, sort_by=['reply'])
        yt.run_reduce(Joiner(args.num_contexts), [temp_src, '<foreign=%true>'+temp_pool], args.dst,
                      join_by=['reply'], reduce_by=['reply'], sort_by=['reply'],
                      memory_limit=10000000000)

    yt.run_sort(args.dst, args.dst, sort_by=['uuid', 'client_time'])



if __name__ == '__main__':
    parser = argparse.ArgumentParser(add_help=True)
    parser.add_argument('--src', required=True)
    parser.add_argument('--pool', required=True)
    parser.add_argument('--dst', required=True)
    parser.add_argument('--num-contexts', type=int, default=1)
    args = parser.parse_args()
    main(args)
