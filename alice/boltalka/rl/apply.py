import pandas as pd
import tqdm
import numpy as np
import sys
import argparse
sys.path.append('../py_libs/nlgsearch_simple/')
import nlgsearch
sys.path.append('distributed_self_play')


APPLIER = nlgsearch.NlgDssmApplier("/mnt/storage/nzinov/rl/index/insight_c3_rus_lister/model")
parser = argparse.ArgumentParser()
parser.add_argument('--model', required=True)
parser.add_argument('--dataset', required=True)
parser.add_argument('--boost-top', action='store_true')
args = parser.parse_args()

model = args.model
forward = None
if model.endswith('onnx'):
    import onnx
    from onnx_tf.backend import prepare
    model = prepare(onnx.load(model))
    forward = lambda x: model.run(x)
else:
    from train import Experiment
    import torch
    experiment = Experiment(name=model)
    model = experiment.model
    forward = lambda x: model.forward(torch.FloatTensor(x).cuda()).detach().cpu().numpy()



def get_scores(contexts, candidates, in_top):
    embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in APPLIER.get_embeddings(contexts, candidates)], axis=1)
    if args.boost_top:
        for i in range(len(embeddings)):
            embeddings[i, 0] = in_top[i]
    return np.ravel(forward(embeddings))

data = pd.read_csv(args.dataset, sep='\t', header=None)

BAD = False

batch = []
group_id = None
c = 0
for _, row in tqdm.tqdm(data.iterrows(), total=len(data)):
    in_top = 0
    if row[0] != group_id:
        c = 0
        group_id = row[0]
    if c < 10:
        in_top = 1
    c += 1

    row = [str(el) for el in list(row)[-4:]]
    batch.append((row[:-1], row[-1], in_top))
    if len(batch) == 2048:
        for el in get_scores([el[0] for el in batch], [el[1] for el in batch], [el[2] for el in batch]):
            print(el)
        batch.clear()
for el in get_scores([el[0] for el in batch], [el[1] for el in batch], [el[2] for el in batch]):
    print(el)
