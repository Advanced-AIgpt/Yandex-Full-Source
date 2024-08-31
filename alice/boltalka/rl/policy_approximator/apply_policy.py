import yt.wrapper as yt
import sys
sys.path.append('..')
from data_loader import tqdm_yt_read
sys.path.append('../../py_libs/nlgsearch_simple')
import nlgsearch
from train import Experiment
import torch
import numpy as np

if __name__ == '__main__':
    def softmax(x):
        e_x = np.exp(x - np.max(x))
        return e_x / e_x.sum()

    MODEL = Experiment(name='train-16.10-15:11').model
    NLG = nlgsearch.NlgSearch('/mnt/storage/nzinov/index', 50, 'insight_c3_rus_lister', None, 'base:sns1400:dcl35000,assessors:sns500:dcl8500')
    for row in tqdm_yt_read('//home/voice/dialog/val_bucket_02.07.19'):
        context = list(reversed([row['context_{}'.format(i)] or '' for i in range(3)]))
        context_embedding, candidate_embeddings, candidates = NLG.get_candidates('insight_c3_rus_lister', context)
        batch = np.concatenate((np.tile(context_embedding, (len(candidate_embeddings), 1)), candidate_embeddings), axis=1)
        logits = MODEL.get_score(torch.FloatTensor(batch).cuda()).squeeze().cpu().detach().numpy()
        p = softmax(logits)
        reply = np.random.choice(candidates, p=p)
        print('\t'.join(context), reply.decode('utf-8'), sep='\t')
