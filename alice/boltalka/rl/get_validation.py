from q_simulator import Model
from gym import Gym
import argparse
import numpy as np
import yt.wrapper as yt
from data_loader import tqdm_yt_read
import torch


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--table', required=True)
    args = parser.parse_args()
    gym = Gym(lambda: Model(inference=True), 'models/simulator_only_len_1208')
    for row in tqdm_yt_read(args.table, format=yt.YsonFormat(encoding=None)):
        embeddings = np.concatenate([np.fromstring(row[key], dtype=np.float32) for key in [b'context_embedding', b'reply_embedding']])
        row[b'rl_score'] = gym.model(torch.FloatTensor(embeddings)).item()
        print(row[b'query_id'], row[b'inv_reranker_score'], row[b'rl_score'], row[b'target_inf'], row[b'result'])
