from tensorboardX import SummaryWriter
import sys
import os
import datetime
import numpy as np
import torch
from collections import defaultdict
import __main__ as main
import argparse
from util import STATE_DIR, LOG_DIR
import time
import requests
from requests.adapters import HTTPAdapter
from requests.packages.urllib3.util.retry import Retry


def requests_retry_session(
    retries=5,
    backoff_factor=0.3,
    status_forcelist=(500, 502, 504),
    session=None,
):
    session = session or requests.Session()
    retry = Retry(
        total=retries,
        read=retries,
        connect=retries,
        backoff_factor=backoff_factor,
        status_forcelist=status_forcelist,
    )
    adapter = HTTPAdapter(max_retries=retry)
    session.mount('http://', adapter)
    session.mount('https://', adapter)
    return session


SESSION = requests_retry_session()


class Mean:
    def __init__(self):
        self.sum = 0
        self.num = 0
    
    def add(self, value):
        self.sum += value
        self.num += 1
    
    def get(self):
        mean = self.sum / self.num
        return mean


class RemoteSummaryWriter():
    def __init__(self, name):
        self.name = name
    
    def _post(self, _type, tag, value, step):
        params = {'name': self.name,
                  'type': _type,
                  'tag': tag,
                  'value': value,
                  'step': step}
        SESSION.post(url='http://nikola10.search.yandex.net:5756/post', params=params)

    def add_scalar(self, tag, value, step):
        self._post('add_scalar', tag, value, step)

    def add_text(self, tag, value, step):
        self._post('add_text', tag, value, step)


class Experiment:
    def __init__(self, name=None, comment='', path=None, command=None):
        if name is None:
            if command is None:
                command = os.path.basename(main.__file__).split('.')[0]
            time = datetime.datetime.today().strftime('%d.%m-%H:%M')
            if comment:
                comment = '-' + comment
            name = '{}-{}{}'.format(command, time, comment)
        self.name = name
        self.writer = None
        self.scalars = defaultdict(Mean)
        self.step = 1
        self.checkpoint_step = None
        self.stateful = self.get_state()
        if path is None:
            self.fname = os.path.join(STATE_DIR, name)
        else:
            self.fname = path
        if os.path.exists(self.fname):
            self.load()
        self.previous_dump_time = None
    
    def get_writer(self, purge_step):
        return SummaryWriter(os.path.join(LOG_DIR, self.name), purge_step=purge_step)

    def load(self):
        data = torch.load(self.fname, map_location=lambda storage, loc: storage)
        self.name = data['name']
        self.checkpoint_step = data['checkpoint_step']
        for k, obj in self.stateful.items():
            obj.load_state_dict(data['states'][k])
        self.step = self.checkpoint_step + 1
        del data
    
    def checkpoint(self):
        self.checkpoint_step = self.step
        data = dict(
            name=self.name,
            checkpoint_step=self.checkpoint_step,
            step=self.step,
            states={k: obj.state_dict() for k, obj in self.stateful.items()}
        )
        print(f'saving to {self.fname}')
        os.makedirs(os.path.dirname(self.fname), exist_ok=True)
        torch.save(data, self.fname)

    def add_scalar(self, tag, value):
        self.scalars[tag].add(value)
    
    def dump_scalars(self):
        dump_time = time.time()
        if self.previous_dump_time is not None:
            self.add_scalar('rate', dump_time - self.previous_dump_time)
        self.previous_dump_time = dump_time
        if self.writer is None:
            purge_step = None
            if self.checkpoint_step:
                purge_step = self.checkpoint_step + 1
            self.writer = self.get_writer(purge_step)
        for tag, mean in self.scalars.items():
            self.writer.add_scalar(tag, mean.get(), self.step)
        self.scalars.clear()
    
    def loop(self, iterator):
        for el in iterator:
            self.step += 1
            yield el
    
    def advance(self):
        self.step += 1
    
    def __getattr__(self, name):
        if name in self.stateful:
            return self.stateful[name]
        else:
            raise AttributeError()
    
    def get_state(self):
        return {}

    @staticmethod
    def create_parser():
        parser = argparse.ArgumentParser()
        parser.add_argument('--command', default=None)
        parser.add_argument('--path', default=None)
        parser.add_argument('--name', default=None)
        parser.add_argument('--comment', default='')
        return parser

    @classmethod
    def from_args(cls, args):
        if args.name:
            args.name = args.name.split('/')[-1]
        return cls(name=args.name, comment=args.comment, path=args.path, command=args.command)
    
    @classmethod
    def from_cmdline(cls):
        parser = cls.create_parser()
        args = parser.parse_args()
        return cls.from_args(args)


class ValhallaExperiment(Experiment):
    def get_writer(self, purge_step):
        return RemoteSummaryWriter(self.name)