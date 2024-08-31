import sys
import numpy as np
cuda = None
if True:
    import torch
    from torch import nn
    from torch import optim
    from tensorboardX import SummaryWriter
    from tqdm import tqdm, trange
    cuda = torch.device('cuda')
import yt.wrapper as yt
sys.path.append('../py_libs/apply_nlg_dssm_module/')
import apply_nlg_dssm
applier = apply_nlg_dssm.NlgDssmApplier("/mnt/storage/nzinov/index/insight_c3_rus_lister/model")

def softmax(x):
    e_x = np.exp(x - np.max(x))
    return e_x / e_x.sum()

class Model(nn.Module):
    def __init__(self):
        super().__init__()
        self.inp = nn.Sequential(
             nn.Linear(600, 1024),
             nn.ReLU(),
             nn.Linear(1024, 1024),
             nn.ReLU(),
        )
        self.lstm = nn.LSTM(1024, 1024, 1, batch_first=True)
        self.outp = nn.Linear(1024, 1)

    def forward(self, inp):
        inp = self.inp(inp)
        inp, _ = self.lstm(inp)
        return self.outp(inp)

class Gym:
    def __init__(self):
        self.model = Model()
        try:
            self.model.load_state_dict(torch.load('/home/nzinov/rlmodel'))
        except OSError:
            print('No model to load')
        self.optimizer = optim.Adam(self.model.parameters(), lr=1e-4)

    def get_scores(self, context, candidates):
        CONTEXT = 3
        replies = context
        contexts, replies = [[replies[i - CONTEXT:i] for i in range(1, len(replies) + 1, 2)], replies[1::2]]
        candidates = [el['text'] for el in candidates]
        prev_embeddings = []
        if len(contexts) > 1:
            prev_embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in applier.get_embeddings(contexts[:-1], replies)], axis=1).tolist()
        last_embeddings = np.concatenate([np.array(el).reshape((-1, 300)) for el in applier.get_embeddings([contexts[-1]] * len(candidates), candidates)], axis=1).tolist()
        embeddings = [prev_embeddings + [embedding] for embedding in last_embeddings]
        return self.model.forward(torch.FloatTensor(embeddings))[:, -1, 0].detach().numpy()

    def predict(self, context_embedding, reply_embedding):
        data = np.concatenate(np.broadcast_arrays(context_embedding, reply_embedding), axis=2)
        return self.model(torch.FloatTensor(data))

CONTEXT = 3

def tqdm_read_table(table_path):
    size = yt.get(table_path + '/@row_count')
    return tqdm(yt.read_table(table_path), total=size)


def run_map_local(mapper, input_table, output_table):
    def gen():
        for row in tqdm_read_table(input_table):
            yield mapper(row)
    yt.write_table(output_table, gen())

def prepare_sessions():
    def mapper(row):
        replies = row['session'].split('\t')
        contexts, replies = [[replies[i - CONTEXT:i] for i in range(1, len(replies), 2)], replies[1::2]]
        embeddings = np.concatenate([np.array(part).reshape((-1, 300)) for part in applier.get_embeddings(contexts, replies)], axis=1)
        return {'embeddings': embeddings.tostring()}
    yt.run_map(mapper, "//home/voice/alzaharov/bexp/scoring/sessions-jan-mar", '//home/voice/nzinov/q_learning_0307')


def pad(sequences, target):
    max_len = max(len(el) for el in sequences)
    data = np.zeros(*((len(sequences), max_len) + sequences[0][0].shape))
    target_data = np.zeros((len(sequences), max_len, 1))
    mask = np.zeros_like(target_data)
    for i in range(len(sequences)):
        target_data[i][:len(sequences[i]), 0] = target[i]
        data[i][:len(sequences[i])] = sequences[i]
        mask[i][:len(sequences[i])] = 1
    return data, target_data, mask


def train():
    writer = SummaryWriter()
    gym = Gym()
    gym.model.to(device=cuda)
    gamma = 0.99
    BATCH_SIZE = 128
    for epoch in trange(100):
        losses = []
        table_path = "//home/voice/alzaharov/bexp/scoring/sessions-jan-mar"
        size = yt.get(table_path + '/@row_count')
        batch = []
        batch_target = []
        for episode, row in tqdm(enumerate(yt.read_table(table_path)), total=size):
            episode = episode + size * epoch
            replies = row['session'].split('\t')
            contexts, replies = [[replies[i - CONTEXT:i] for i in range(1, len(replies), 2)], replies[1::2]]
            target = [0]
            for i in range(len(replies) - 1):
                target.append(gamma * target[-1] + 1)
            target = list(reversed(target))
            embeddings = np.concatenate([np.array(part).reshape((-1, 300)) for part in applier.get_embeddings(contexts, replies)], axis=1)
            batch.append(embeddings)
            batch_target.append(target)
            if episode % BATCH_SIZE == 0:
                batch, batch_target, mask = pad(batch, batch_target)
                loss = (torch.FloatTensor(mask).cuda() * (torch.FloatTensor(batch_target).cuda() - gym.model(torch.FloatTensor(batch).cuda()))**2).sum() / 600
                loss.backward()
                losses.append(loss.item() / len(target) / 600)
                gym.optimizer.step()
                gym.optimizer.zero_grad()
                batch = []
                batch_target = []
            #writer.add_scalar('dialog_length', np.mean(log[-20:]), episode)
            if (episode + 1) % 1000 == 0:
                writer.add_scalar('loss', np.mean(losses), episode)
                losses = []
            #writer.add_text('Text', '\n\n'.join(context), episode)
            if episode % 1000 == 0:
                torch.save(gym.model.state_dict(), "/home/nzinov/rlmodel")


def main():
    #train()
    prepare_sessions()


if __name__ == '__main__':
    main()
