import sys
sys.path.append('../../rl/')
from util import DATA_DIR
sys.path.append('../../py_libs/nlgsearch_simple/')

import itertools
import numpy as np
from random import choice
import torch
from torch import nn
from torch import optim
import tqdm
from experiment import Experiment
import asyncio
import concurrent.futures
import os
import traceback
import logging
from lstm_dssm_model import *



def train_vh(experiment):
    model = experiment.model
    optimizer = experiment.optimizer
    model.cuda()
    samples = load_dataset("//home/voice/alzaharov/nirvana/dde97b40-d859-4521-be97-ddcbeda57ca4/output__okwqwB-zRwapaoSbmueE-A")
    batch_size = 32
    val_samples = samples[-2048:]
    samples = samples[: -2048]
    best_val_loss = None
    for epoch in tqdm.tqdm(range(130)):
        for i in range(0, len(samples), batch_size):
            loss = optimization_step(model, optimizer, samples[i:i+batch_size]).data.cpu().numpy()
            experiment.add_scalar('loss', loss)
            if batch_size * experiment.step % 32768 == 0:
                val_loss = 0
                counter = 0.0
                for i in range(0, len(val_samples), batch_size):
                    counter += 1
                    val_loss += calculate_loss(model, val_samples[i : i + batch_size] , False).data.cpu().numpy()
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
        model = create_model()
        model.cuda()
        optimizer = optim.Adam(model.parameters(), lr=1e-5)
        return dict(model=model, optimizer=optimizer)

def main():
    experiment = MyExperiment.from_cmdline()
    train_vh(experiment)

if __name__ == '__main__':
    main()
