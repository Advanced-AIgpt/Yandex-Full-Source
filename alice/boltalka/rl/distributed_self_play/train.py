import sys
sys.path.append('..')
import itertools
import numpy as np
from random import choice
import torch
from torch import nn
from torch import optim
import torch.nn.functional as F
from tqdm import tqdm
from experiment import ValhallaExperiment as Experiment
import asyncpool
import asyncio
import concurrent.futures
import os
import traceback
import logging
from util import normalize
import zmq
from zmq.asyncio import Context as ZmqContext
from models import CurrentModel
from mlockall import mlockall
import io

mlockall()

logger = logging.getLogger(__name__)
ch = logging.StreamHandler()
logger.addHandler(ch)

MASTER_IP = '127.0.0.1'
MASTER_PORT = 1337
MASTER_MODEL_PORT = 1338

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()

class Batch:
    def __init__(self):
        self.seqs = []

    def add(self, inputs):
        self.seqs.append(inputs)

    def get(self):
        batch_size = len(self.seqs)
        max_len = max(len(el[0]) for el in self.seqs)
        batch_seqs = [
            np.zeros((batch_size, max_len,) + el.shape[1:])
            for el in self.seqs[0]
        ]
        mask = np.zeros((batch_size, max_len, 1))
        for sample_idx in range(len(self.seqs)):
            length = len(self.seqs[sample_idx][0])
            mask[sample_idx, :length] = 1
            for seq_idx in range(len(self.seqs[0])):
                batch_seqs[seq_idx][sample_idx, :length] = self.seqs[sample_idx][seq_idx]
        self.seqs.clear()
        return batch_seqs + [mask]

def decode(messages):
    l = int(messages[0].decode())
    assert l > 0
    return (
        np.frombuffer(messages[1], dtype=np.float32).reshape((l, 300)),
        np.frombuffer(messages[2], dtype=np.float32).reshape((l, 50, 300)),
        np.frombuffer(messages[3], dtype=np.uint8).reshape((l, 1)),
        np.frombuffer(messages[4], dtype=np.float32).reshape((l, 1)),
        np.frombuffer(messages[5], dtype=np.float32).reshape((l, 50, 1)),
        np.frombuffer(messages[6], dtype=np.float32).reshape((l, 1)),
    )

async def train(experiment, args):
    logger.info(f'model: {experiment.fname}')
    model = experiment.model
    model.cuda()
    optimizer = experiment.optimizer
    GAMMA = 0.9
    batch = []
    ctx = ZmqContext()
    ctx.setsockopt(zmq.IPV6, 1)
    queue_socket = ctx.socket(zmq.PULL)
    queue_socket.bind('tcp://{}:{}'.format(args.master_ip, args.master_port))
    #queue_socket.setsockopt(zmq.RCVBUF, 2 * 2**20)
    #queue_socket.setsockopt(zmq.RCVHWM, 100)
    model_socket = ctx.socket(zmq.PUB)
    model_socket.bind('tcp://{}:{}'.format(args.master_ip, args.master_model_port))
    logger.info('starting training')
    batch = Batch()
    BATCH_SIZE = 100
    if args.debug:
        BATCH_SIZE = 1
    PROD_POLICY = torch.zeros(1, 1, 50).cuda()
    PROD_POLICY[:, :, :10] = 0.1
    PROD_POLICY[:, :, 10:] = 1e-3
    PROD_POLICY /= torch.sum(PROD_POLICY)
    PROD_POLICY = torch.log(PROD_POLICY)
    logger.info(PROD_POLICY)
    for _ in tqdm(itertools.count()):
        sample = decode(await queue_socket.recv_multipart())
        batch.add(sample)
        #context = sample[-1]
        #logger.debug(context)
        if experiment.step % BATCH_SIZE == 0:
            context_embeddings, candidate_embeddings, chosen, scores, relevs, generate_probas, mask = batch.get()
            logger.debug(f'generate_probs {generate_probas}')
            states, _ = model.get_state(torch.FloatTensor(context_embeddings).cuda())
            probs = model.get_proba(states, torch.FloatTensor(candidate_embeddings).cuda(), torch.FloatTensor(relevs).cuda())
            logger.debug(f'probs shape {probs.shape}')
            count = np.sum(mask)
            values = model.get_value(states)
            probs_exp = torch.exp(probs)
            mask = torch.FloatTensor(mask).cuda()
            logger.debug(probs.shape)
            logger.debug(chosen.shape)
            neg_entropy = torch.sum(probs_exp * probs * mask.unsqueeze(-1)) / count + np.math.log(50)
            experiment.add_scalar('neg_entropy', neg_entropy.item())
            logger.debug(f'probs {probs.detach().cpu().numpy()}')
            kl = torch.sum(probs_exp.squeeze(-1) * (probs.squeeze(-1) - PROD_POLICY) * mask) / count
            experiment.add_scalar('kl_rev', kl.item())
            probs = torch.gather(probs, dim=2, index=torch.LongTensor(chosen).cuda().unsqueeze(3)).squeeze(2)
            iw = torch.clamp(torch.exp(probs.detach() - torch.FloatTensor(generate_probas).cuda()), -1., 5.)
            logger.debug(f'probs {probs.detach().cpu().numpy()}')
            #scores += (chosen < 10) * 0.5
            for i in reversed(range(len(scores[0]) - 2)):
                scores[:, i] += scores[:, i + 2] * GAMMA
            scores = torch.FloatTensor(scores).cuda()
            offset_scores = scores - values.detach()
            logger.debug(f'scores {scores}')
            logger.debug(f'values {values.detach().cpu().numpy()}')
            logger.debug(f'chosen {chosen}')
            loss = -torch.sum(iw * offset_scores * probs * mask)
            value_loss = torch.sum((values - scores)**2 * mask)
            (loss + value_loss + kl * 50).backward()
            experiment.add_scalar('pseudo_loss', loss.item())
            experiment.add_scalar('length', count / len(context_embeddings))
            experiment.add_scalar('value_loss', value_loss.item() / count)
            optimizer.step()
            optimizer.zero_grad()
        if (experiment.step - 1) % 1000 == 0:
            logger.info('sending model')
            state = model.state_dict()
            for k, v in state.items():
                state[k] = v.cpu()
            buffer = io.BytesIO()
            torch.save(state, buffer)
            await model_socket.send(buffer.getbuffer())
            del state
            del buffer
            logger.info('after send')
        if experiment.step % 1000 == 0:
            experiment.dump_scalars()
            #experiment.writer.add_text('session', '   \n'.join(context), experiment.step)
            experiment.checkpoint()
        experiment.advance()

class Experiment(Experiment):
    model_class = CurrentModel

    def get_state(self):
        model = self.model_class()
        model.cuda()
        optimizer = optim.Adam(model.parameters(), lr=1e-4, weight_decay=0)
        return dict(model=model, optimizer=optimizer)

def main():
    parser = Experiment.create_parser()
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--master-ip')
    parser.add_argument('--master-model-port')
    parser.add_argument('--master-port')
    args = parser.parse_args()
    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)
    experiment = Experiment.from_args(args)
    event_loop = asyncio.get_event_loop()
    try:
        event_loop.run_until_complete(
            train(experiment, args)
        )
    finally:
        event_loop.close()

if __name__ == '__main__':
    main()
