import torch.nn as nn
import torch.nn.functional as F
import yt.wrapper as yt
import numpy as np


class LSTMClassifier(nn.Module):
    def __init__(self, input_size=300, num_layers=1, hidden_size=1024, output=53):
        super(LSTMClassifier, self).__init__()

        self.lstm = nn.LSTM(input_size=input_size, num_layers=num_layers, hidden_size=hidden_size, batch_first=True)
        self.linear = nn.Linear(hidden_size * num_layers, output)
        self.softmax = nn.LogSoftmax(1)

    def forward(self, input):
        (output, (h_n, c_n)) = self.lstm(input)
        # Swapaxes of h_n from [num_layers, batch_size, hidden_size] to [batch_size, num_layers * hidden_size]
        h_n = h_n.permute([1, 0, 2])
        h_n = h_n.reshape(h_n.shape[0], -1)

        logits = self.linear(h_n)


        return self.softmax(logits), h_n
