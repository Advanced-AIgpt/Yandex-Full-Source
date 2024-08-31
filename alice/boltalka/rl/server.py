import sys
sys.path.append('../telegram_bot/')
from rpc import RpcServer
from policy_approximator.train import Experiment
from q_simulator import Gym
#from rudder import Gym, Experiment

if __name__ == '__main__':
    experiment = Experiment.from_cmdline()
    RpcServer('localhost', 1337, ranker=Gym(experiment)).run()
