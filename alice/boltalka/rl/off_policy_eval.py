import sys
import itertools
from data_loader import TextDataLoader
import torch
import numpy as np
import pickle
from tqdm import tqdm
from util import DATA_DIR
from policy_approximator.train import Experiment as ApproxExp
from util import normalize


MODEL = 'insight_c3_rus_lister'
FILE = DATA_DIR + '/off_policy.data.npy'


def prepare_sessions(nlg, applier):
    data = TextDataLoader('//home/voice/nzinov/gc_sessions_feedback', 1)
    NUM = 10000
    sessions = []
    approx = ApproxExp(name='train-18.10-16:07').model
    for session, target in tqdm(itertools.islice(data._iter_items(), NUM), total=NUM):
        data = []
        for i in range(1, len(session), 2):
            context = [el[0] for el in session[max(i - 3, 0):i]]
            reply, skip = session[i]
            if not skip:
                reply = normalize(reply)
                context_embedding, candidate_embeddings, candidates = nlg.get_candidates(MODEL, context)
                reply_embedding = np.array(applier.get_embeddings([context], [reply]))[1]
                candidate_embeddings = np.concatenate([candidate_embeddings, [reply_embedding]])
                batch = np.concatenate((np.tile(context_embedding, (len(candidate_embeddings), 1)), candidate_embeddings), axis=1)
                data.append((batch, len(candidate_embeddings)))
        if not data:
            continue
        baseline_log_proba = approx.forward(torch.FloatTensor(np.stack([el[0] for el in data])).cuda())[:, -1].cpu().detach().numpy()
        data = [el[0] + (el[1],) for el in zip(data, baseline_log_proba)]
        sessions.append((data, (target, len(session))))
    return sessions


DATA = None
METRIC_NAMES = ('iw_score', 'iw_length')


class Average:
    def __init__(self):
        self.values = []
        self.weights = []

    def add(self, value, weight=1):
        self.values.append(value)
        self.weights.append(weight)

    def mean(self):
        denom = sum(self.weights)
        mean = sum(weight * value for weight, value in zip(self.weights, self.values)) / denom
        std = np.math.sqrt(sum(weight * (value - mean)**2 for weight, value in zip(self.weights, self.values)) / denom)
        return mean, std
    
    def __str__(self):
        mean, std = self.mean()
        return "{} +- {}".format(mean, std)
    
    def __repr__(self):
        return str(self)


def eval(model):
    global DATA
    if DATA is None:
        DATA = np.load(FILE)
    data = DATA
    metrics = [Average() for i in range(2)]
    baselines = [Average() for i in range(2)]
    for session, targets in tqdm(data):
        if targets[1] > 80:
            continue
        for i in range(len(metrics)):
            baselines[i].add(targets[i])
        log_prob = 0
        for batch, candidate_num, baseline_log_proba in session:
            cur_log_prob = model(torch.FloatTensor(batch))[-1].item()
            log_prob += cur_log_prob - baseline_log_proba
            print(np.exp(cur_log_prob), np.exp(baseline_log_proba))
        if log_prob == 0:
            continue
        prob = np.math.exp(log_prob)
        for i in range(len(metrics)):
            metrics[i].add(targets[i], prob)
    return {name: (metric, baseline) for name, metric, baseline in zip(METRIC_NAMES, metrics, baselines)}

def prepare():
    sys.path.append('../py_libs/nlgsearch_simple/')
    import nlgsearch
    APPLIER = nlgsearch.NlgDssmApplier("/mnt/storage/nzinov/index/insight_c3_rus_lister/model")
    NLG = nlgsearch.NlgSearch('/mnt/storage/nzinov/index', 50, 'insight_c3_rus_lister', None, 'base:sns1400:dcl35000,assessors:sns500:dcl8500', num_threads=1, memory_mode='Locked')
    np.save(FILE, prepare_sessions(NLG, APPLIER))

if __name__ == "__main__":
    from experiment import Experiment
    parser = Experiment.create_parser()
    parser.add_argument('--mode', required=True)
    args = parser.parse_args()
    if args.mode == 'prepare':
        prepare()
    elif args.mode == 'eval':
        from q_simulator import Experiment, Model
        class Experiment(Experiment):
            def get_state(self):
                return dict(model=Model())
        experiment = Experiment.from_args(args)
        res = eval(experiment.model)
        print(res)
