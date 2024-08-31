from nlgsearch_http_source import NlgsearchHttpSource, NlgsearchRanker
from nlgsearchsimple_source import NlgsearchSource
from replier import Replier
import sys
sys.path.append('../py_libs/apply_nlg_dssm/')
import apply_nlg_dssm
import numpy as np
from random import choice
import torch
from torch import nn
from torch import optim
from tensorboardX import SummaryWriter
torch.set_num_threads(1)

class Lazy:
    def __init__(self, initializer):
        self.initializer = initializer
        self.instance = None

    def init(self):
        if self.instance is None:
            self.instance = self.initializer()

    def __getattr__(self, attr):
        self.init()
        return getattr(self.instance, attr)

    def __settattr__(self, attr, val):
        self.init()
        return setattr(self.instance, attr, val)

applier = Lazy(lambda: apply_nlg_dssm.NlgDssmApplier("/mnt/storage/nzinov/arcadia/alice/boltalka/rl/data"))
applier = applier.initializer()
print('Loaded applier')

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()

class Gym:
    def __init__(self):
        self.model = nn.Sequential(
            nn.Linear(600, 300),
            nn.ReLU(),
            nn.Linear(300, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU(),
            nn.Linear(100, 100),
            nn.ReLU(),
            nn.Linear(100, 1),
            #nn.LogSoftmax(dim=0)
        )
        try:
            self.model.load_state_dict(torch.load('/home/nzinov/simulator_1707.model'))
        except OSError:
            print('No model to load')

    def predict(self, context_embedding, reply_embedding):
        data = context_embedding + reply_embedding + list(np.array(context_embedding) * reply_embedding)
        return self.model(torch.FloatTensor(data))

    def get_score(self, context_embedding, reply_embedding):
        return self.predict(context_embedding, reply_embedding).detach().numpy()[0]

    def get_scores(self, context, candidates):
        candidates = [el['text'] for el in candidates]
        embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in applier.get_embeddings([context] * len(candidates), candidates)], axis=1)
        return np.ravel(self.model(torch.FloatTensor(embeddings)).detach().numpy())

    def train(self, context_embedding, selected_reply_embedding, denom, reward):
        denom = torch.FloatTensor([denom])
        self.optimizer.zero_grad()
        loss = -torch.FloatTensor([reward]) * (self.predict(context_embedding, selected_reply_embedding) - torch.log(denom))
        loss.backward()
        self.optimizer.step()

def should_stop(context, reply_embeddings):
    if context[-1] in context[:-1]:
        return True
    if len(context) > 2:
        like = np.dot(reply_embeddings[-3], reply_embeddings[-1])
        if like > 0.97:
            return True
        if np.dot(reply_embeddings[-1], reply_embeddings[-2]) > 0.97 and np.dot(reply_embeddings[-2], reply_embeddings[-3]) > 0.97:
            return True
    return False


def rollout(context, reply_embeddings):
    print("Rollout")
    context = context[:]
    reply_embeddings = reply_embeddings[:]
    length = nlgsearch.search.rollout(context, reply_embeddings)
    print(length)
    return length

def reward(context, reply_embeddings, reply, reply_embedding):
    return np.mean([rollout(context + [reply], reply_embeddings + [reply_embedding]) for i in range(10)])

def simulate():
    writer = SummaryWriter()
    starters = open("/mnt/storage/nzinov/NeuralResponseRanking/data_loader/shuffled.txt")
    gym = Gym()
    print("Start")
    log = []
    for episode in range(10000):
        context = []
        topmost = []
        reply_embeddings = []
        train_queue = []
        while True:
            reply = None
            reply_embedding = None
            if context:
                candidates = [el['text'] for el in replier.get_replies(context[-3:]) if el['text'] not in log[-10:]]
                topmost.append(candidates[0])
                context_embedding = applier.get_context_embedding(context[-3:])
                candidate_embeddings = [applier.get_reply_embedding(candidate) for candidate in candidates]
                scores = [gym.get_score(context_embedding, candidate_embedding) for candidate_embedding in candidate_embeddings]
                #rewards = [reward(context, reply_embeddings, candidate, candidate_embedding) for candidate, candidate_embedding in zip(candidates, candidate_embeddings)]
                denom = np.sum(np.exp(scores))
                idx = np.random.choice(len(candidates), p=softmax(scores))
                reply = candidates[idx]
                #advantage = rewards[idx] - np.mean(rewards)
                reply_embedding = candidate_embeddings[idx]
                train_queue.append((context_embedding, reply_embedding, denom, rollout(context[:], reply_embeddings[:])))
            else:
                reply = next(starters).strip()
                reply_embedding = applier.get_reply_embedding(reply)
                topmost.append('')
            context.append(reply)
            reply_embeddings.append(reply_embedding)
            if should_stop(context, reply_embeddings):
                break
        print(len(context))
        log.append(len(context))
        for i, el in enumerate(train_queue):
            gym.train(*el[:-1], len(context) - i - 1 -el[-1])
        writer.add_scalar('dialog_length', np.mean(log[-20:]), episode)
        if episode % 50 == 0:
            print("\n".join(["{}\n({})".format(*el) for el in zip(context, topmost)]))
            torch.save(gym.model.state_dict(), "model")


if __name__ == '__main__':
    simulate()
