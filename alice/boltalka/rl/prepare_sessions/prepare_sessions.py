import numpy as np
import argparse
import yt.wrapper as yt
from alice.boltalka.py_libs.apply_nlg_dssm import apply_nlg_dssm

CONTEXT = 3


class Mapper:
    def __init__(self, args):
        self.applier = None
        self.args = args

    def start(self):
        self.applier = apply_nlg_dssm.NlgDssmApplier('model')
        self.dsat_applier = apply_nlg_dssm.NlgDssmApplier('dsat.model')

    def __call__(self, row):
        if row[b'feedback'] == -1:
            return
        session = [
            el.replace('<SKIP>', '')
            for el in row[b'session'].decode().split('\t')
        ]
        contexts, replies = [[
            session[i - CONTEXT:i] for i in range(1, len(session), 2)
        ], session[1::2]]
        dsat_contexts, dsat_replies = [[
            session[i - CONTEXT:i] for i in range(2, len(session), 2)
        ], session[2::2]]
        embeddings = np.concatenate([
            np.array(part).reshape((-1, 300))
            for part in self.applier.get_embeddings(contexts, replies)
        ], axis=1)
        dsat = self.dsat_applier.get_scores(dsat_contexts, dsat_replies)
        yield {
            b'embeddings': embeddings.tobytes(),
            b'dsat': dsat,
            b'session': row[b'session'],
            b'feedback': row[b'feedback']
        }


YT_CLOUD_SPEC = {
    'pool_trees': ['physical'],
    'tentative_pool_trees': ['cloud'],
    'job_count': 10000
}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--input', default='//home/voice/nzinov/gc_sessions')
    parser.add_argument('--output', required=True)
    parser.add_argument(
        '--model',
        default='/mnt/storage/nzinov/index/insight_c3_rus_lister/model')
    parser.add_argument('--dsat-model',
                        default='/mnt/storage/nzinov/rl/dsat.model')
    args = parser.parse_args()
    yt.run_map(Mapper(args),
               args.input,
               args.output,
               local_files=[args.model, args.dsat_model],
               memory_limit=20 * 1024**3,
               format=yt.YsonFormat(encoding=None),
               spec=YT_CLOUD_SPEC)
