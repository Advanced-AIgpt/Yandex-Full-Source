import sys
from q_simulator import Experiment
import torch

model = sys.argv[1]
experiment = Experiment(name=model)
for name, val in vars(experiment).items():
    print(name, val)