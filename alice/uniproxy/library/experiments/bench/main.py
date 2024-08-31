from alice.uniproxy.library.experiments import Experiments
from alice.uniproxy.library.events import Event
from library.python import resource
import json
import time


_macro_list = json.loads(resource.find("/macros.json"))
_experiments_list = json.loads(resource.find("/experiments.json"))
experiments = Experiments(_experiments_list, _macro_list, mutable_shares=True)

# half of experiments becomes per-event
_pe_experiments_list = json.loads(resource.find("/experiments.json"))
for exp in _pe_experiments_list[:int(len(_pe_experiments_list)/2)]:
    exp["per_event"] = True
experiments_pe = Experiments(_pe_experiments_list, _macro_list, mutable_shares=True)


EVENT_BIG = Event(json.loads(resource.find("/event_big.json"))["Event"]["event"])
EVENT_LITTLE = Event(json.loads(resource.find("/event_little.json"))["Event"]["event"])

# -------------------------------------------------------------------------------------------------


class FakeUniSystem:
    def __init__(self):
        self.patcher = None
        self.uaas_flags = {}
        self.uaas_test_ids = []
        self.session_data = {'uuid': '123'}
        self.exps_check = False

    def set_event_patcher(self, patcher):
        self.patcher = patcher

    def log_experiment(self, *args, **kwargs):
        pass

    def patch(self, event):
        self.patcher.patch(event, self.session_data)


def run(func, *args, cycles=1000, **kwargs):
    t0 = time.perf_counter_ns()
    for _ in range(cycles):
        func(*args, **kwargs)
    t1 = time.perf_counter_ns()
    dur = t1 - t0
    return dur / cycles


def measure(func):
    def wrap(*args, cycles=1000, title=func.__name__, **kwargs):
        print(f"Run '{title}' ... ", end="", flush=True)
        dur = run(func, *args, **kwargs)
        print(f" {int(dur/1000)} microsec")
    return wrap


# -------------------------------------------------------------------------------------------------


@measure
def patcher_construction(unisystem):
    experiments.try_use_experiment(unisystem)


@measure
def patcher_construction_pe(unisystem):
    experiments_pe.try_use_experiment(unisystem)


@measure
def patch_big(unisystem):
    unisystem.patcher.patch(EVENT_BIG, unisystem.session_data)


@measure
def patch_little(unisystem):
    unisystem.patcher.patch(EVENT_LITTLE, unisystem.session_data)


def main():
    global CYCLES

    us = FakeUniSystem()

    patcher_construction(us, title="Construct without PE", cycles=1000)
    patcher_construction_pe(us, title="Construct with PE", cycles=1000)

    experiments.try_use_experiment(us)
    patch_little(us, title="Patch little without PE", cycles=1000)
    experiments_pe.try_use_experiment(us)
    patch_little(us, title="Patch little with PE", cycles=1000)

    experiments.try_use_experiment(us)
    patch_big(us, title="Patch big without PE", cycles=200)
    experiments_pe.try_use_experiment(us)
    patch_big(us, title="Patch big with PE", cycles=200)


if __name__ == "__main__":
    main()
