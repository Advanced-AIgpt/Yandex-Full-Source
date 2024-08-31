import torch
from torch import nn


class Model(nn.Module):
    def __init__(self, inference=False):
        super().__init__()
        self.body = nn.Sequential(nn.Linear(601, 300), nn.ReLU(),
                                  nn.Linear(300, 100), nn.ReLU(),
                                  nn.Linear(100, 100), nn.ReLU(),
                                  nn.Linear(100, 100), nn.ReLU())
        self.policy = nn.Sequential(nn.Linear(100, 1))
        self.log_softmax = nn.LogSoftmax(dim=-2)
        self.value_model = nn.Sequential(nn.Linear(300, 100), nn.ReLU(),
                                         nn.Linear(100, 100), nn.ReLU(),
                                         nn.Linear(100, 100), nn.ReLU(),
                                         nn.Linear(100, 1))

    def forward(self, input):
        return self.policy(self.body(input))

    def get_state(self, context, history=None):
        return context, history

    def get_score(self, state, reply, relev):
        state = torch.cat(
            [state.unsqueeze(-2).expand(-1, -1, reply.shape[2], -1), reply, relev],
            dim=-1)
        return self.policy(self.body(state))

    def get_value(self, state):
        return self.value_model(state)

    def get_proba(self, state, reply, relev):
        score = self.get_score(state, reply, relev)
        return self.log_softmax(score)

    def get_state_proba(self, context, reply, relev, history, history_c):
        state, _ = self.get_state(context, None)
        return state, history, history_c, self.get_proba(state, reply, relev)


class RnnModel(nn.Module):
    def __init__(self, inference=False):
        super().__init__()
        self.inputer = nn.Sequential(nn.Linear(300, 256), nn.ReLU())
        self.context_lstm = nn.LSTM(256, 256, 1, batch_first=True)
        self.policy = nn.Sequential(nn.Linear(300 + 256, 256), nn.ReLU(),
                                    nn.Linear(256, 256), nn.ReLU(),
                                    nn.Linear(256, 128), nn.ReLU(),
                                    nn.Linear(128, 1))
        self.log_softmax = nn.LogSoftmax(dim=-2)
        self.value_model = nn.Linear(256, 1)

    def get_state(self, context, history=None):
        context = self.inputer(context)
        state, history = self.context_lstm(context, history)
        return state, history

    def get_score(self, state, reply):
        state = torch.cat(
            [state.unsqueeze(-2).expand(-1, -1, reply.shape[2], -1), reply],
            dim=-1)
        state[:, :, :10, 0] = 1.
        state[:, :, 10:, 0] = 0.
        return self.policy(state)

    def get_value(self, state):
        return self.value_model(state)

    def forward(self, state, reply):
        score = self.get_score(state, reply)
        return self.log_softmax(score)

    def get_state_proba(self, context, reply, history, history_c):
        history = history.transpose(0, 1)
        history_c = history_c.transpose(0, 1)
        state, (history, history_c) = self.get_state(context,
                                                     (history, history_c))
        history = history.transpose(0, 1)
        history_c = history_c.transpose(0, 1)
        return state, history, history_c, self.forward(state, reply)


CurrentModel = Model