import yt.wrapper as yt
import numpy as np
from tensorboardX import SummaryWriter
import ast

if __name__ == '__main__':
    writer = SummaryWriter('/mnt/storage/nzinov/rl/runs/context_boltalka2')
    data = []
    metadata = []
    for row in yt.read_table(yt.TablePath('//home/voice/nickpon/val_gc_sessions/ctx_len_2_and_embeddings_new_nzinov_model', end_index=10000)):
        data.append(row['context_embedding'])
        metadata.append((row['context_1'] + '||' + row['context_0']).replace('\n', ' '))
    writer.add_embedding(np.array(data), metadata)
