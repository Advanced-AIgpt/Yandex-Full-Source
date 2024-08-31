import torch
from torch import nn
from torch import optim
import sys
from util import DATA_DIR
sys.path.append('../../py_libs/nlgsearch_simple/')
import nlgsearch
import argparse
import itertools
import numpy as np
from random import choice
import asyncpool
import asyncio
import concurrent.futures
import os
import subprocess
import traceback
import logging
from models import CurrentModel
import zmq
from zmq.asyncio import Context as ZmqContext
from mlockall import mlockall
import io

mlockall()

APPLIER = None
MODEL = 'insight_c3_rus_lister'

ctx = ZmqContext.instance()
ctx.setsockopt(zmq.IPV6, 1)

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()


def should_stop(context, reply_embeddings):
    #return 'да' in context
    if context[-1] in context[:-1]:
        pass
        return True
    if len(context) > 3:
        like = np.dot(reply_embeddings[-3], reply_embeddings[-1])
        if like > 0.97:
            return True
        if np.dot(reply_embeddings[-1],
                  reply_embeddings[-2]) > 0.97 and np.dot(
                      reply_embeddings[-2], reply_embeddings[-3]) > 0.97:
            return True
    return False


class BatchPredictor:
    def __init__(self, func, batch_size=256):
        self.func = func
        self.batch_size = batch_size
        self.inputs = []
        self.futures = []

    def __call__(self, *input):
        loop = asyncio.get_event_loop()
        if len(self.inputs) >= self.batch_size:
            inputs = [
                torch.stack([el[i] for el in self.inputs])
                for i in range(len(self.inputs[0]))
            ]
            futures = self.futures[:]
            self.inputs = []
            self.futures = []
            results = self.func.get_state_proba(*inputs)
            results = [[row.squeeze(0) for row in res.detach().split(1)] for res in results]
            for i, future in enumerate(futures):
                future.set_result(tuple(el[i] for el in results))
        future = loop.create_future()
        self.inputs.append(input)
        self.futures.append(future)
        return future


class Session:
    def __init__(self, start):
        self.context = [start.strip()]
        self.reply_embeddings = []
        self.context_embeddings = []
        self.candidate_embeddings_lists = []
        self.chosen = []
        self.proba = []
        self.history = torch.zeros(1, 256)
        self.history_c = torch.zeros(1, 256)


class Simulator:
    def __init__(self, args):
        self.args = args
        self.model = CurrentModel()
        self.result_queue = asyncio.Queue(maxsize=512)
        self.batcher = BatchPredictor(self.model)
        INDEX_PATH = os.path.join(DATA_DIR, 'index')
        if not os.path.exists(INDEX_PATH):
            print('downloading index')
            subprocess.check_call(['sky', 'get', 'rbtorrent:6f7d61bdb03cb639513452962b921675bbb1f844'])
            print('downloaded')
        self.nlg = nlgsearch.NlgSearch(INDEX_PATH,
                                       50,
                                       'insight_c3_rus_lister,factor_dssm_0_index',
                                       None,
                                       'base:sns1400:dcl35000,assessors:sns500:dcl8500',
                                       ranker_model_name='catboost:sf9',
                                       num_threads=60,
                                       memory_mode='Locked',
                                       seq2seq_uri='http://nikola10.search.yandex.net:5757/generative')
        self.starters = open(os.path.join(DATA_DIR, "starters.txt")).readlines()
        self.pool = concurrent.futures.ThreadPoolExecutor(50)

    async def gen_session(self, start):
        start = start.strip()
        proba_list = []
        context = []
        used_reply_embeddings = []
        context_embeddings = []
        candidate_embeddings_lists = []
        relevs_list = []
        context.append(start)
        used_reply_embeddings.append(None)
        loop = asyncio.get_event_loop()
        chosen = []
        with torch.no_grad():
            history = torch.zeros(1, 256, dtype=torch.float)
            history_c = torch.zeros(1, 256, dtype=torch.float)
            for turn in range(200):
                turn_context = context[-3:]
                context_embedding, candidate_embeddings, candidates, relevs = await loop.run_in_executor(
                    self.pool, self.nlg.get_candidates, MODEL, turn_context)
                context_embedding = torch.as_tensor(context_embedding, dtype=torch.float).unsqueeze(0)
                candidate_embeddings = torch.as_tensor(candidate_embeddings, dtype=torch.float).reshape((1, -1, 300))
                relevs = torch.as_tensor(relevs, dtype=torch.float).unsqueeze(0).unsqueeze(-1)
                fut = self.batcher(
                    context_embedding,
                    candidate_embeddings,
                    relevs,
                    history, history_c)
                state, history, history_c, proba = await fut
                del fut
                proba = proba.detach().squeeze().cpu().numpy()
                selected_reply = np.random.choice(candidate_embeddings.shape[1], p=np.exp(proba))
                proba_list.append(proba[selected_reply])
                context_embeddings.append(context_embedding)
                candidate_embeddings_lists.append(candidate_embeddings)
                relevs_list.append(relevs)
                chosen.append(selected_reply)
                context.append(candidates[selected_reply].decode('utf8'))
                used_reply_embeddings.append(candidate_embeddings[0, selected_reply])
                if should_stop(context, used_reply_embeddings):
                    break
            scores = [1.] * len(context_embeddings)
            if len(scores) < 200:
                scores[-1] = -10.
                if len(scores) > 1:
                    scores[-2] = -10.
            await self.result_queue.put([str(len(context_embeddings)).encode(),
                                         np.stack(context_embeddings),
                                         np.stack(candidate_embeddings_lists),
                                         np.array(chosen, dtype=np.uint8).reshape((-1, 1)),
                                         np.array(scores, dtype=np.float32).reshape((-1, 1)),
                                         np.stack(relevs_list),
                                         np.array(proba_list, dtype=np.float32).reshape((-1, 1))
                                        ])

    async def _generator(self, starts):
        async with asyncpool.AsyncPool(asyncio.get_event_loop(),
                                       num_workers=1024,
                                       name="Generator",
                                       logger=logging.getLogger("Generator"),
                                       worker_co=self.gen_session) as pool:
            print('generating')
            for el in starts:
                await pool.push(el)

    async def serve(self):
        asyncio.ensure_future(self._generator(itertools.cycle(self.starters)))
        asyncio.ensure_future(self.loop_model_update())
        sample_socket = ctx.socket(zmq.PUSH)
        sample_socket.connect('tcp://{}:{}'.format(self.args.master_ip, self.args.master_port))
        sample_socket.setsockopt(zmq.SNDBUF, 2 * 2**20)
        sample_socket.setsockopt(zmq.SNDHWM, 100)
        while True:
            await sample_socket.send_multipart(await self.result_queue.get(), copy=False)

    async def loop_model_update(self):
        s = ctx.socket(zmq.SUB)
        s.setsockopt(zmq.SUBSCRIBE, b'')
        s.setsockopt(zmq.CONFLATE, 1)
        s.connect('tcp://{}:{}'.format(self.args.master_ip, self.args.master_model_port))
        while True:
            print('wating for model')
            buffer = io.BytesIO(await s.recv())
            self.model.load_state_dict(torch.load(buffer))
            del buffer
            print('loaded model')


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--master-ip')
    parser.add_argument('--master-model-port')
    parser.add_argument('--master-port')
    args = parser.parse_args()
    with torch.no_grad():
        event_loop = asyncio.get_event_loop()
        try:
            event_loop.run_until_complete(
                Simulator(args).serve()
            )
        finally:
            event_loop.close()
