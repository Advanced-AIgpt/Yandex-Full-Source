import itertools
import yt.wrapper as yt
import torch
from torch import nn
import torch.optim as optim
from experiment import ValhallaExperiment as Experiment
from data_loader import EmbeddingDataLoader
import logging

classes = [b'other', b'laugh', b'love', b'happy', b'wink', b'smirk', b'cry', b'sweat', b'think', b'thumb', b'cool', b'facepalm']

logger = logging.getLogger(__name__)
ch = logging.StreamHandler()
logger.addHandler(ch)


def train(experiment, args):
    criterion = nn.CrossEntropyLoss()
    model = experiment.model
    model.cuda()
    optimizer = experiment.optimizer
    batch = []
    BATCH_SIZE = 100
    def parse_target(target):
        return classes.index(target)

    for epoch in range(8):
        for i, (batch, target) in enumerate(EmbeddingDataLoader('//home/voice/nzinov/emoji_embeddings', parse_target=parse_target).tqdm_batches()):
            logits = model.forward(torch.FloatTensor(batch).cuda())
            target = torch.LongTensor(target).cuda()
            loss = criterion(logits, target)
            if i < 10:
                accuracy = torch.mean((torch.argmax(logits, dim=-1) == target).float())
                experiment.add_scalar('val_loss', loss.item())
                experiment.add_scalar('val_accuracy', accuracy.item())
            else:
                loss.backward()
                experiment.add_scalar('train_loss', loss.item())
            optimizer.step()
            optimizer.zero_grad()
            experiment.advance()
        experiment.dump_scalars()
        experiment.checkpoint()

def apply(experiment, args):
    model = experiment.model
    model.cuda()
    batch = []
    BATCH_SIZE = 100
    for i, (batch, target) in enumerate(EmbeddingDataLoader('//home/voice/krom/assessors_index_28012020/context_and_reply').tqdm_batches()):
        if i == 100:
            pass
            #break
        logits = model.forward(torch.FloatTensor(batch).cuda())
        predict = torch.argmax(logits, dim=-1)
        for el in predict.cpu().detach().numpy().ravel():
            print(classes[el].decode())

class Experiment(Experiment):
    def get_state(self):
        model = nn.Sequential(
            nn.Linear(600, 300),
            nn.ReLU(),
            nn.Linear(300, 300),
            nn.ReLU(),
            nn.Linear(300, 300),
            nn.ReLU(),
            nn.Linear(300, 300),
            nn.ReLU(),
            nn.Linear(300, 12),
        )
        model.cuda()
        optimizer = optim.Adam(model.parameters(), lr=1e-3, weight_decay=1e-4)
        return dict(model=model, optimizer=optimizer)

def main():
    parser = Experiment.create_parser()
    parser.add_argument('--debug', action='store_true')
    parser.add_argument('--mode', required=True)
    args = parser.parse_args()
    if args.debug:
        logger.setLevel(logging.DEBUG)
    else:
        logger.setLevel(logging.INFO)
    experiment = Experiment.from_args(args)
    if args.mode == 'train':
        train(experiment, args)
    else:
        apply(experiment, args)

if __name__ == '__main__':
    main()

