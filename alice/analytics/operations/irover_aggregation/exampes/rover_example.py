#!usr/bin/python
# -*- encoding: utf-8 -*-

from utils.resources import ClusterReference
from utils.rover import RoverVotingScheme
from utils.word_transition_network import TextHyp
import random

rover = RoverVotingScheme(
    "q",
    [TextHyp("q", "test1", "aa bb cc"), TextHyp("q", "test2", "aa cc ee"), TextHyp("q", "test3", "ba bb")],
    # ClusterReference("../linguistics/cluster_references/ru-RU/cluster_references.json")
)
print(" ".join(value for value, _ in rover.get_result() if value != ""))

rover = RoverVotingScheme(
    "q",
    [TextHyp("q", "test1", "эйси диси"), TextHyp("q", "test2", "ac dc"), TextHyp("q", "test3", ["эйси", "диси"])],
    # ClusterReference("../linguistics/cluster_references/ru-RU/cluster_references.json")
)
print(" ".join(value for value, _ in rover.get_result() if value != ""))

# inp = [list("FARMS.")] * 13 + [list("famms")] * 31 + [list("farms")] * 89 + [list("farms.")] * 130
inp = ["aa bb cc".split()] + ["aa cc ee".split()] + ["ba bb".split()]
# inp.sort(key=lambda x: random.random())
rover = RoverVotingScheme("q", [TextHyp("q", i, text) for i, text in enumerate(inp)])
print(rover)

print("".join(value for value, _ in rover.get_result() if value != ""))

from hashlib import md5
[TextHyp("q", i, text) for i, text in enumerate(inp)].sort(key=lambda x: md5((x[2][0] + str(x[1]) + " and some salt").encode()).hexdigest())