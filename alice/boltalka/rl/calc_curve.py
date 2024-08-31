"""
Accepts a tsv file with columns as follows:
group_id factor_0 factor_1 target_0 target_1 ...
For each positive alpha ranks rows with the same group_id by (factor_0 + alpha * factor_1) and computes mean target over top 1 samples.
"""
import numpy as np
import sys
TARGETS = ((9, 'max'), (1, 'result'), (2, 'num'), (3, 'num'))
MAP = {"bad": 0, "neutral": 1, "good": 2}
def convert(val, t):
    if t == 'result':
        return [MAP[el] for el in val]
    val = np.array([float(el) for el in val])
    if t == 'num':
        return val
    if t == 'max':
        return (val == np.amax(val)).astype(float)
    if t == 'rank':
        return np.argsort(-val).argsort()
    if t == 'proba':
        data = np.exp(val - np.amax(val))
        return data / np.sum(data)
data = np.loadtxt(sys.stdin, dtype=object, delimiter='\t')
groups = np.unique(data[:, 0])
FACTOR_0 = 4
FACTOR_1 = 9
for i in range(len(data)):
    data[i][FACTOR_0] = -float(data[i][FACTOR_0])
    data[i][FACTOR_1] = float(data[i][FACTOR_1])
groups = [data[data[:, 0] == group] for group in groups]
group_targets = [[convert(group[:, idx], t) for idx, t in TARGETS] for group in groups]
maxes = [None] * len(groups)
queue = []
scores = [0] * len(TARGETS)
for group_idx, group in enumerate(groups):
    maxes[group_idx] = np.argmax(group[:, FACTOR_0])
    for i in range(len(TARGETS)):
        scores[i] += group_targets[group_idx][i][maxes[group_idx]]
    for i in range(len(group)):
        for j in range(len(group)):
            if group[i][FACTOR_1] > group[j][FACTOR_1]:
                equal_alpha = (group[i][FACTOR_0] - group[j][FACTOR_0]) / (group[i][FACTOR_1] - group[j][FACTOR_1]) * -1
                if equal_alpha > 0:
                    queue.append((equal_alpha, group_idx, i, j))
queue.sort()
result = [[0,] + [score / len(groups) for score in scores]]
for el in queue:
    equal_alpha, group_idx, upper, lower = el
    group = groups[group_idx]
    if lower == maxes[group_idx]:
        maxes[group_idx] = upper
        for i in range(len(TARGETS)):
            scores[i] += group_targets[group_idx][i][upper] - group_targets[group_idx][i][lower]
    result.append([equal_alpha,] + [score / len(groups) for score in scores])
for i, el in enumerate(result):
    if i % 10 == 0:
        print(*el)
