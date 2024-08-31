import sys
sys.path.append('..')
from experiment import Experiment
from data_loader import tqdm_yt_read
from tqdm import trange
import torch
import yt.wrapper as yt
from torch import nn
import numpy as np

yt.config["read_parallel"]["enable"] = True

class Model(nn.Module):
    def __init__(self, inference=False):
        super().__init__()
        self.body = nn.Sequential(
            nn.Linear(600, 300),
            nn.ReLU(),
            nn.Linear(300, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU())
        self.policy = nn.Linear(100, 1)
        self.log_softmax = nn.LogSoftmax(dim=-1)

    def get_score(self, input):
        tmp = self.body(input)
        return self.policy(tmp).squeeze()

    def forward(self, input):
        return self.log_softmax(self.get_score(input))

class Experiment(Experiment):
    def get_state(self):
        model = Model().cuda()
        optimizer = torch.optim.Adam(model.parameters(), lr=1e-5)
        return dict(model=model, optimizer=optimizer)

def train():
    experiment = Experiment.from_cmdline()
    model = experiment.model
    optimizer = experiment.optimizer
    BATCH_SIZE = 2048
    matrix = np.ones(50)
    matrix[0] = 500.0
    matrix /= matrix.sum()
    matrix = torch.FloatTensor(matrix[None, :]).cuda()
    for epoch in trange(100):
        batch = []
        for i, row in enumerate(tqdm_yt_read('//home/voice/nzinov/policy_approximator/dataset', format=yt.YsonFormat(encoding=None), unordered=True)):
            context_embedding = np.fromstring(row[b'context_embedding'], dtype=np.float32).reshape((-1, 300))
            reply_embedding = np.fromstring(row[b'reply_embedding'], dtype=np.float32).reshape((-1, 300))
            candidate_embeddings = np.fromstring(row[b'candidate_embeddings'], dtype=np.float32).reshape((-1, 300))
            candidate_embeddings = np.concatenate((reply_embedding, candidate_embeddings))
            data = np.concatenate((context_embedding.repeat(len(candidate_embeddings), 0), candidate_embeddings), axis=1)
            batch.append(data)
            if len(batch) >= BATCH_SIZE:
                batch = torch.FloatTensor(batch).cuda()
                loss = -(model.forward(batch) * matrix).sum()
                if  i <= 10 * BATCH_SIZE:
                    experiment.add_scalar('val_loss', loss.item() / BATCH_SIZE)
                else:
                    loss.backward()
                    optimizer.step()
                    experiment.add_scalar('loss', loss.item() / BATCH_SIZE)
                    if experiment.step % 30 == 0:
                        experiment.dump_scalars()
                        experiment.checkpoint()
                experiment.advance()
                batch = []

if __name__ == '__main__':
    train()