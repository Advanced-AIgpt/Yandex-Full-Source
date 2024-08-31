import sys
sys.path.append('../../rl/')
sys.path.append('../../py_libs/nlgsearch_simple/')

from dataset import *
from model import *
import traceback
import os
import concurrent.futures
import asyncio
from experiment import Experiment
import tqdm
from torch import optim
from torch import nn
import torch
from random import choice
import numpy as np
import itertools
from util import DATA_DIR


def loss_f(true, predicted):
    return -(true * predicted).sum(1).mean()


def calculate_loss(model, batch_samples, with_prefixes=True):
    if with_prefixes:
        embeds, labels = make_batch_from_samples_with_prefexes(batch_samples)
    else:
        embeds, labels = make_batch_from_samples(batch_samples)
    embeds = embeds.cuda()
    labels = labels.cuda()

    result = model(embeds)

    loss = loss_f(labels, result)
    return loss


def optimization_step(model, optimizer, batch_samples):
    loss = calculate_loss(model, batch_samples)
    model.zero_grad()
    loss.backward()
    optimizer.step()
    return loss


def load_dataset(table):
    dataset = FeedbackSessionsDataset(table)
    iterator = iter(dataset)
    samples = [x for x in tqdm.tqdm(iterator)]
    return samples


def train_vh(experiment):
    batch_size = 32
    train_samples = load_dataset(
        '//home/voice/alzaharov/interests_lstm/embeded_train')
    val_samples = load_dataset(
        '//home/voice/alzaharov/interests_lstm/embeded_validation')

    best_val_loss = None
    for epoch in tqdm.tqdm(range(20)):
        for i in range(0, len(train_samples), batch_size):
            loss = optimization_step(
                model, optimizer, train_samples[i:i + batch_size]).data.cpu().numpy()
            experiment.add_scalar('loss', loss)
            if batch_size * experiment.step % 32768 == 0:
                val_loss = 0
                counter = 0.0
                for i in range(0, len(val_samples), batch_size):
                    counter += 1
                    val_loss += calculate_loss(
                        model, val_samples[i: i + batch_size], False).data.cpu().numpy()
                val_loss /= counter
                experiment.add_scalar('val_loss', val_loss)
                if not best_val_loss or val_loss < best_val_loss:
                    best_val_loss = val_loss
                    experiment.checkpoint('-best')
                experiment.dump_scalars()
                experiment.checkpoint()
            experiment.advance()
    experiment.checkpoint()


class MyExperiment(Experiment):

    def get_state(self):
        model = LSTMClassifier()
        if torch.cuda.is_available():
            model.cuda()
        optimizer = optim.Adam(model.parameters(), lr=1e-4)
        return dict(model=model, optimizer=optimizer)


def main():
    experiment = MyExperiment.from_cmdline()
    train_vh(experiment)


if __name__ == '__main__':
    main()
