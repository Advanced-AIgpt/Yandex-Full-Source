import sys
import numpy as np
import torch
from torch import nn
from torch import optim
from experiment import ExperimentFactory
cuda = torch.device('cuda')
from data_loader import EmbeddingDataLoader, SeqDataLoader
from gym import Gym
from tqdm import trange

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()

class Model(nn.Module):
    def __init__(self):
        super().__init__()
        self.inp = nn.Sequential(
             nn.Linear(600, 512),
             nn.ReLU(),
             nn.Dropout(0.5),
             nn.Linear(512, 512),
             nn.ReLU(),
             nn.Dropout(0.5),
             nn.Linear(512, 512),
             nn.ReLU(),
             nn.Dropout(0.5)
        )
        self.lstm = nn.LSTM(512, 512, 3, batch_first=True, dropout=0.5)
        self.outp = nn.Linear(512, 1)

    def forward(self, inp):
        inp = self.inp(inp)
        inp, _ = self.lstm(inp)
        return self.outp(inp)

class SeqModel(nn.Module):
    def __init__(self, dict_size=20000):
        super().__init__()
        self.emb = nn.Embedding(dict_size, 512, padding_idx=0)
        self.lstm = nn.LSTM(512, 512, 3, batch_first=True)
        self.turn_lstm = nn.LSTM(512, 1024, 1, batch_first=True)
        self.outp = nn.Linear(1024, 1)

    def forward(self, inp):
        inp = self.emb(inp)
        shape = inp.shape
        inp = inp.reshape((shape[0], -1, 512))
        inp, _ = self.lstm(inp)
        inp = inp.view(shape)
        inp, _ = self.turn_lstm(inp[:, :, -1, :].squeeze())
        return self.outp(inp)

APPLIER = None

class Gym(Gym):
    def get_scores(self, context, candidates):
        global APPLIER
        if APPLIER is None:
            sys.path.append('../py_libs/apply_nlg_dssm_module')
            import apply_nlg_dssm
            APPLIER = apply_nlg_dssm.NlgDssmApplier("/mnt/storage/nzinov/index/insight_c3_rus_lister/model")
        CONTEXT = 3
        replies = context
        candidates = [el['text'] for el in candidates]
        contexts, replies = [[replies[i - CONTEXT:i] for i in range(1, len(replies) + 1, 2)], replies[1::2]]
        prev_embeddings = []
        if len(contexts) > 1:
            prev_embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in APPLIER.get_embeddings(contexts[:-1], replies)], axis=1).tolist()
        last_embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in APPLIER.get_embeddings([contexts[-1]] * len(candidates), candidates)], axis=1).tolist()
        embeddings = [prev_embeddings + [embedding] for embedding in last_embeddings]
        K = 10
        N = 50
        data = torch.cat([torch.FloatTensor(embeddings) for i in range(N)]).cuda()
        res = self.model.forward(data)[:, -1, 0].detach().cpu().numpy().reshape((N, -1))
        return np.partition(res, K, axis=0)[K].tolist()


class SeqGym(Gym):
    def __init__(self):
        self.model_path = '/home/nzinov/rudder_like_seq.model'
        self.data_loader = SeqDataLoader('//home/voice/nzinov/gc_sessions', 32, '/mnt/storage/nzinov/NeuralResponseRanking/data_loader/s.txt')
        self.model = SeqModel()
        try:
            self.model.load_state_dict(torch.load(self.model_path))
        except OSError:
            print('No model to load')
        self.optimizer = optim.Adam(self.model.parameters(), lr=1e-4, weight_decay=0)

    def get_scores(self, context, candidates):
        candidates = [el['text'] for el in candidates]
        context = [self.data_loader._process_turn(el) for el in context]
        candidates = [self.data_loader._process_turn(el) for el in candidates]
        candidate_contexts = [context + [reply] for reply in candidates]
        data = self.data_loader._process_batch(candidate_contexts, [1] * len(candidate_contexts))[0]
        return self.model.forward(torch.LongTensor(data))[:, -1, 0].detach().cpu().numpy()

exps = [
    dict(comment=' rudder',
         loader=lambda: EmbeddingDataLoader(),
         gym=Gym),
    dict(comment=' rudder seq',
         loader=lambda: SeqDataLoader('//home/voice/nzinov/gc_sessions', 32, '/mnt/storage/nzinov/NeuralResponseRanking/data_loader/s.txt'),
         gym=SeqGym)
]

def train(experiment):
    exp = exps[0]
    model = experiment.model
    VAL_SIZE = 32
    EVAL_EVERY = 16
    SAVE_EVERY = 2048
    loader = EmbeddingDataLoader()
    for epoch in trange(100):
        for episode, batch in enumerate(loader.tqdm_batches()):
            batch, batch_target, mask = batch
            loss = (torch.FloatTensor(mask).cuda() * (torch.FloatTensor(batch_target).cuda() - model(torch.FloatTensor(batch).cuda()))**2).sum()
            loss_value = loss.item() / np.sum(mask)
            if episode <= VAL_SIZE:
                experiment.add_scalar('val_loss', loss_value)
                if episode == VAL_SIZE:
                    experiment.dump_scalars()
            if episode > VAL_SIZE:
                loss.backward()
                experiment.add_scalar('loss', loss_value)
                experiment.optimizer.step()
                experiment.optimizer.zero_grad()
                if experiment.step % EVAL_EVERY == 0:
                    experiment.dump_scalars()
                if experiment.step % SAVE_EVERY == 0:
                    experiment.checkpoint()
            experiment.advance()


class ExperimentFactory(ExperimentFactory):
    def get_state(self):
        model = Model()
        model.to(device=cuda)
        optimizer = optim.Adam(model.parameters(), lr=1e-4, weight_decay=1)
        return dict(model=model, optimizer=optimizer)

Experiment = ExperimentFactory()


if __name__ == '__main__':
    train(Experiment.from_cmdline())
