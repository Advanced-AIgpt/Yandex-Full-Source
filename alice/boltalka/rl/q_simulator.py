import sys
from util import DATA_DIR
sys.path.append('../py_libs/nlgsearch_simple/')
import nlgsearch
import itertools
import numpy as np
from random import choice
import torch
from torch import nn
from torch import optim
from tqdm import tqdm
from experiment import Experiment
from data_loader import TextDataLoader
import off_policy_eval
from gym import Gym
import asyncpool
import asyncio
import concurrent.futures
import os
import traceback
import logging
from util import normalize

APPLIER = None
MODEL = 'insight_c3_rus_lister'
#from rudder import Gym as RudderGym
#rudder_gym = RudderGym()
#RUDDER = rudder_gym.model

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()


def should_stop(context, reply_embeddings):
    if context[-1] in context[:-1]:
        pass
        return True
    if len(context) > 3:
        like = np.dot(reply_embeddings[-3], reply_embeddings[-1])
        if like > 0.97:
            return True
        if np.dot(reply_embeddings[-1], reply_embeddings[-2]) > 0.97 and np.dot(reply_embeddings[-2], reply_embeddings[-3]) > 0.97:
            return True
    return False

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
        self.policy = nn.Sequential(nn.Linear(100, 1))
        self.log_softmax = nn.LogSoftmax(dim=-2)
        self.value_model = nn.Sequential(
            nn.Linear(300, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU(),
            nn.Linear(100, 1))

    def get_score(self, input):
        tmp = self.body(input)
        return self.policy(tmp)

    def forward(self, input):
        return self.log_softmax(self.get_score(input))


class RnnModel(nn.Module):
    def __init__(self, inference=False):
        super().__init__()
        self.inputer = nn.Sequential(
            nn.Linear(300, 256),
            nn.ReLU())
        self.context_lstm = nn.LSTM(256, 256, 1, batch_first=True)
        self.policy = nn.Sequential(
            nn.Linear(300 + 256, 256),
            nn.ReLU(),
            nn.Linear(256, 256),
            nn.ReLU(),
            nn.Linear(256, 128),
            nn.ReLU(),
            nn.Linear(128, 1))
        self.log_softmax = nn.LogSoftmax(dim=-2)
        self.value_model = nn.Linear(256, 1)

    def get_state(self, context, history=None):
        context = self.inputer(context)
        state, history = self.context_lstm(context, history)
        return state, history

    def get_score(self, state, reply):
        state = torch.cat([state.unsqueeze(-2).expand(-1, -1, reply.shape[2], -1), reply], dim=-1)
        return self.policy(state)
    
    def get_value(self, state):
        return self.value_model(state)

    def forward(self, state, reply):
        score = self.get_score(state, reply)
        return self.log_softmax(score)
    
    def get_state_proba(self, context, reply, history, history_c):
        history = history.transpose(0, 1)
        history_c = history_c.transpose(0, 1)
        state, (history, history_c) = self.get_state(context, (history, history_c))
        history = history.transpose(0, 1)
        history_c = history_c.transpose(0, 1)
        return state, history, history_c, self.forward(state, reply)
    

class Gym(Gym):
    def get_scores(self, context, candidates):
        global APPLIER
        if APPLIER is None:
            APPLIER = nlgsearch.NlgDssmApplier("/mnt/storage/nzinov/index/insight_c3_rus_lister/model")
        candidates = [normalize(el['text']) for el in candidates]
        embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in APPLIER.get_embeddings([context] * len(candidates), candidates)], axis=1)
        return np.ravel(self.model.get_score(torch.FloatTensor(embeddings).cuda()).detach().cpu().numpy()).tolist()


class BatchPredictor:
    def __init__(self, func, batch_size=256):
        self.func = func
        self.batch_size = batch_size
        self.inputs = []
        self.futures = []

    def __call__(self, *input):
        loop = asyncio.get_event_loop()
        if len(self.inputs) >= self.batch_size:
            inputs = [torch.stack([el[i] for el in self.inputs]) for i in range(len(self.inputs[0]))]
            futures = self.futures[:]
            self.inputs = []
            self.futures = []
            results = self.func.get_state_proba(*inputs)
            for i, future in enumerate(futures):
                future.set_result(tuple(el[i] for el in results))
        future = loop.create_future()
        self.inputs.append(input)
        self.futures.append(future)
        return future


class Simulator:
    def __init__(self, nlg, model, pool):
        self.nlg = nlg
        self.model = model
        self.pool = pool
        self.result_queue = asyncio.Queue(maxsize=1024)
        self.batcher = BatchPredictor(self.model)

    async def gen_session(self, start):
        start = start.strip()
        context = []
        reply_embeddings = []
        context_embeddings = []
        candidate_embeddings_lists = []
        context.append(start)
        reply_embeddings.append(None)
        loop = asyncio.get_event_loop()
        chosen = []
        history = torch.zeros(1, 256).cuda()
        history_c = torch.zeros(1, 256).cuda()
        for turn in range(400):
            turn_context = context[-3:]
            context_embedding, candidate_embeddings, candidates = await loop.run_in_executor(self.pool, self.nlg.get_candidates, MODEL, turn_context)
            fut = self.batcher(torch.FloatTensor([context_embedding]).cuda(), torch.FloatTensor([candidate_embeddings]).cuda(), history, history_c)
            state, history, history_c, proba = await fut
            del fut
            proba = proba.detach().squeeze().cpu().numpy()
            selected_reply = np.random.choice(len(candidate_embeddings), p=np.exp(proba))
            context_embeddings.append(context_embedding)
            candidate_embeddings_lists.append(candidate_embeddings)
            chosen.append(selected_reply)
            context.append(candidates[selected_reply].decode('utf8'))
            reply_embeddings.append(candidate_embeddings[selected_reply])
            if should_stop(context, reply_embeddings):
                break
        scores = [1] * len(context_embeddings)
        if len(scores) < 401:
            scores[-1] = -5
        await self.result_queue.put((context_embeddings, candidate_embeddings_lists, chosen, scores, context))

    async def _generator(self, starts):
        async with asyncpool.AsyncPool(asyncio.get_event_loop(), num_workers=1024, name="Generator", logger=logging.getLogger("Generator"), worker_co=self.gen_session) as pool:
            for el in starts:
                await pool.push(el)

    async def generate(self, starts):
        asyncio.ensure_future(self._generator(starts))
        while True:
            yield await self.result_queue.get()

async def simulate(experiment):
    NLG = nlgsearch.NlgSearch('/mnt/storage/nzinov/index', 50, 'insight_c3_rus_lister', None, 'base:sns1400:dcl35000,assessors:sns500:dcl8500', num_threads=60, memory_mode='Locked')
    #applier = nlgsearch.NlgDssmApplier(DATA_DIR + "/index/insight_c3_rus_lister/model")
    #off_policy_eval.prepare_sessions(NLG, applier)
    #FACTOR = nlgsearch.NlgDssmApplier(DATA_DIR + "/index/factor_dssm_0_index/model")
    model = experiment.model
    optimizer = experiment.optimizer
    model.cuda()
    starters = open(DATA_DIR + "/starters.txt").readlines()
    GAMMA = 0.95
    pool = concurrent.futures.ThreadPoolExecutor(50)
    sim = Simulator(NLG, model, pool)
    with tqdm(total=100000, smoothing=0.1) as pbar:
        batch = []
        async for row in sim.generate(itertools.cycle(starters)):
            context_embeddings, candidate_embeddings, chosen, scores, context = row
            states, _ = model.get_state(torch.FloatTensor([context_embeddings]).cuda())
            probs = model.forward(states, torch.FloatTensor([candidate_embeddings]).cuda()).squeeze(0)
            probs = probs[np.arange(len(probs)), chosen]
            values = model.get_value(states).squeeze()
            for i in reversed(range(len(scores) - 2)):
                scores[i] += scores[i + 2] * GAMMA
            scores = torch.FloatTensor(scores).cuda()
            offset_scores = scores - values.detach()
            loss = -torch.sum(offset_scores * probs)
            value_loss = torch.sum((values - scores)**2)
            (loss + value_loss).backward()
            experiment.add_scalar('length', len(context_embeddings))
            experiment.add_scalar('value_loss', value_loss.item() / len(context_embeddings))
            if experiment.step % 10 == 0:
                optimizer.step()
                optimizer.zero_grad()
            if experiment.step % 1000 == 0:
                experiment.dump_scalars()
                experiment.checkpoint()
            experiment.advance()
            pbar.update(1)

class Experiment(Experiment):
    model_class = RnnModel

    def get_state(self):
        model = self.model_class()
        model.cuda()
        optimizer = optim.Adam(model.parameters(), lr=1e-3)
        return dict(model=model, optimizer=optimizer)

def main():
    experiment = Experiment.from_cmdline()
    event_loop = asyncio.get_event_loop()
    try:
        event_loop.run_until_complete(
            simulate(experiment)
        )
    finally:
        event_loop.close()

if __name__ == '__main__':
    main()